/* SPDX-License-Identifier: GPL-2.0-only */
/* Main-thread producer: no callback locks, game pointers, filesystem or
   allocations in synthesis/mixing. SDL owns conversion and asynchronous IO. */
#include "sdl_audio.h"
#include "engine/ID_SD.h"
#include "engine/ID_CA.h"
#include "engine/WL_PROFILE.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
static SDL_AudioStream *stream;
static uint64_t generation;
static int queue_bytes=48000*4*80/1000;
static char error[256];
bool Audio_Active(void) { return stream!=NULL; }
#ifdef WOLF3D_DISPLAY_TEST
int Audio_TestQueuedBytes(void) { return stream?SDL_GetAudioStreamQueued(stream):-1; }
void Audio_TestQueueMS(unsigned ms) { queue_bytes=(int)(48000*4*(ms>=8 && ms<=250?ms:80)/1000);if(stream) SDL_ClearAudioStream(stream); }
unsigned Audio_TestTargetMS(void) { return (unsigned)queue_bytes/192; }
#endif
const char *Audio_Error(void) { return error; }
void Audio_Shutdown(void)
{
    if(stream) { SDL_DestroyAudioStream(stream);stream=NULL; }
    SD_Shutdown();SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
static SDL_AudioStream *OpenOutput(const SDL_AudioSpec *spec)
{
    SDL_AudioStream *output=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,spec,NULL,NULL);
#ifdef __linux__
    /* An ALSA-only SDL SDK can run on a PulseAudio/PipeWire desktop without
       an ALSA "default" PCM. Try its standard pulse plugin, respecting any
       explicit SDL device hint. This changes no system configuration. */
    const char *driver=SDL_GetCurrentAudioDriver();
    if(!output && driver && !strcmp(driver,"alsa") &&
       !SDL_GetHint(SDL_HINT_AUDIO_ALSA_DEFAULT_DEVICE) &&
       !SDL_GetHint(SDL_HINT_AUDIO_ALSA_DEFAULT_PLAYBACK_DEVICE)) {
        if(SDL_SetHint(SDL_HINT_AUDIO_ALSA_DEFAULT_PLAYBACK_DEVICE,"pulse")) {
            output=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,spec,NULL,NULL);
            SDL_ResetHint(SDL_HINT_AUDIO_ALSA_DEFAULT_PLAYBACK_DEVICE);
        }
    }
#endif
    return output;
}
bool Audio_Startup(void)
{
    Audio_Shutdown();error[0]=0;
    if(!SD_Startup(48000)) { (void)snprintf(error,sizeof(error),"%s",CA_Error());return false; }
    SDL_AudioSpec spec={SDL_AUDIO_S16,2,48000};
    if(!SDL_InitSubSystem(SDL_INIT_AUDIO) ||
       !(stream=OpenOutput(&spec))) {
        (void)snprintf(error,sizeof(error),"%s",SDL_GetError());Audio_Shutdown();return false;
    }
    SDL_AudioSpec device;int period;
    if(!SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(stream),&device,&period) || device.freq<=0 || period<=0) {
        (void)snprintf(error,sizeof(error),"Cannot query audio device period");Audio_Shutdown();return false;
    }
    /* Two device periods plus scheduling headroom, with an 80 ms floor.
       The old 24 ms target barely covered one 21.3 ms ALSA period. */
    uint64_t ms=((uint64_t)(unsigned)period*2000+(unsigned)device.freq-1)/(unsigned)device.freq+8;
    if(ms<80) ms=80;
    if(ms>250) { (void)snprintf(error,sizeof(error),"Audio device buffer exceeds supported latency");Audio_Shutdown();return false; }
    queue_bytes=(int)(48000*4*ms/1000);
    generation=SD_OutputGeneration();Audio_Pump();
    if(!stream || !SDL_ResumeAudioStreamDevice(stream)) {
        if(stream) (void)snprintf(error,sizeof(error),"%s",SDL_GetError());
        Audio_Shutdown();return false;
    }
    return true;
}
void Audio_Pump(void)
{
    if(!stream) return;
#ifdef WOLF3D_BENCHMARK
    bool changed=generation!=SD_OutputGeneration();
#endif
    if(generation!=SD_OutputGeneration()) {
        if(!SDL_ClearAudioStream(stream)) {
            (void)snprintf(error,sizeof(error),"%s",SDL_GetError());
            fprintf(stderr,"SDL3 audio stopped: %s; continuing silently.\n",error);
            Audio_Shutdown();return;
        }
        generation=SD_OutputGeneration();
    }
    int queued=SDL_GetAudioStreamQueued(stream);
#ifdef WOLF3D_BENCHMARK
    if(queued==0 && !changed) ++WL_Profile.empty_queues;
#endif
    /* Buffer in sample frames, never render-frame timestamps. The producer
       is also serviced inside expensive presentation loops. Arbitrarily long
       OS/driver stalls can still exhaust this deliberately bounded queue. */
    for(unsigned blocks=0;queued>=0 && queued<queue_bytes && blocks<32;++blocks) {
        int16_t pcm[384*2];SD_Render(pcm,384);
        if(!SDL_PutAudioStreamData(stream,pcm,(int)sizeof(pcm))) { queued=-1;break; }
        queued=SDL_GetAudioStreamQueued(stream);
    }
    if(queued<0) {
        (void)snprintf(error,sizeof(error),"%s",SDL_GetError());
        fprintf(stderr,"SDL3 audio stopped: %s; continuing silently.\n",error);Audio_Shutdown();
    }
}
