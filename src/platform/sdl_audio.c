/* SPDX-License-Identifier: GPL-2.0-only */
/* Main-thread producer: no callback locks, game pointers, filesystem or
   allocations in synthesis/mixing. SDL owns conversion and asynchronous IO. */
#include "sdl_audio.h"
#include "engine/ID_SD.h"
#include "engine/ID_CA.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
static SDL_AudioStream *stream;
static char error[256];
bool Audio_Active(void) { return stream!=NULL; }
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
       !(stream=OpenOutput(&spec)) ||
       !SDL_ResumeAudioStreamDevice(stream)) {
        (void)snprintf(error,sizeof(error),"%s",SDL_GetError());Audio_Shutdown();return false;
    }
    return true;
}
void Audio_Pump(void)
{
    if(!stream) return;
    int queued=SDL_GetAudioStreamQueued(stream);
    /* Bound work and latency to 32 ms. A stalled renderer can underrun but
       cannot change the rational music clock, simulation or gameplay RNG. */
    while(queued>=0 && queued<48000*4*24/1000) {
        int16_t pcm[384*2];SD_Render(pcm,384);
        if(!SDL_PutAudioStreamData(stream,pcm,(int)sizeof(pcm))) { queued=-1;break; }
        queued+=(int)sizeof(pcm);
    }
    if(queued<0) {
        (void)snprintf(error,sizeof(error),"%s",SDL_GetError());
        fprintf(stderr,"SDL3 audio stopped: %s; continuing silently.\n",error);Audio_Shutdown();
    }
}
