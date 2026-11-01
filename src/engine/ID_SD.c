/* SPDX-License-Identifier: GPL-2.0-only */
/* Native port of ID_SD.C/ID_SD_A.ASM: IMF, AL effects and SB Pro priorities.
   No DOS interrupts/devices: a rational sample clock services the original
   700/140 Hz schedules. The chip is shared, including channel-zero writes. */
#include "ID_SD.h"
#include "ID_FILE.h"
#include "ID_CA.h"
#include "../../third_party/emu8950/emu8950.h"
#include <string.h>
#define AUDIO_CHUNKS 288U
#define MUSIC_BASE 261U
#define DIGI_COUNT 46U
static memptr header,archive,swap;
static size_t header_size,archive_size,swap_size;
static struct { const uint8_t *data; size_t size; } chunks[AUDIO_CHUNKS],digi[DIGI_COUNT];
static memptr samples[DIGI_COUNT];
static const int digimap[LASTSOUND]={
    [HALTSND]=1,[DOGBARKSND]=2,[CLOSEDOORSND]=3,[OPENDOORSND]=4,
    [ATKMACHINEGUNSND]=5,[ATKPISTOLSND]=6,[ATKGATLINGSND]=7,[SCHUTZADSND]=8,
    [GUTENTAGSND]=9,[MUTTISND]=10,[BOSSFIRESND]=11,[SSFIRESND]=12,
    [DEATHSCREAM1SND]=13,[DEATHSCREAM2SND]=14,[DEATHSCREAM3SND]=14,
    [TAKEDAMAGESND]=15,[PUSHWALLSND]=16,[DOGDEATHSND]=17,[AHHHGSND]=18,
    [DIESND]=19,[EVASND]=20,[LEBENSND]=21,[NAZIFIRESND]=22,[SLURPIESND]=23,
    [TOT_HUNDSND]=24,[MEINGOTTSND]=25,[SCHABBSHASND]=26,[HITLERHASND]=27,
    [SPIONSND]=28,[NEINSOVASSND]=29,[DOGATTACKSND]=30,[LEVELDONESND]=31,
    [MECHSTEPSND]=32,[YEAHSND]=33,[SCHEISTSND]=34,[DEATHSCREAM4SND]=35,
    [DEATHSCREAM5SND]=36,[DONNERSND]=37,[EINESND]=38,[ERLAUBENSND]=39,
    [DEATHSCREAM6SND]=40,[DEATHSCREAM7SND]=41,[DEATHSCREAM8SND]=42,
    [DEATHSCREAM9SND]=43,[KEINSND]=44,[MEINSND]=45,[ROSESND]=46
}; /* zero = no mapping; other entries = original sample index + 1. */
static OPL *chip;
static unsigned output_rate,music_volume=70,effects_volume=85;
static bool music_enabled=true,effects_enabled=true,digitized_mode;

static sd_status status;
void SD_SetDigitizedMode(bool enabled)
{
    digitized_mode=enabled;status.digitized=-1;status.digi_priority=0;
}
bool SD_DigitizedMode(void) { return digitized_mode; }
static uint64_t phase,music_time,next_event,digi_phase,chip_phase;
static int32_t chip_previous[2],chip_next[2];
static size_t music_pos,fx_pos,fx_length;
static unsigned fx_divider,fx_block;
static const uint8_t *fx_data;
static void Write(unsigned reg,unsigned value)
{
    status.registers[reg]=(uint8_t)value;++status.writes;
    OPL_writeReg(chip,reg,(uint8_t)value);
}
static void Gains(void)
{
    if (!chip) return;
    for (unsigned ch=0;ch<14;++ch) {
        float volume=(float)(ch==0 ? (effects_enabled?effects_volume:0) : (music_enabled?music_volume:0))/100.0f;
        float pan[2]={volume,volume};OPL_setPanFine(chip,ch,pan);
    }
}
void SD_SetVolumes(unsigned music,unsigned effects)
{
    music_volume=music>100?100:music;effects_volume=effects>100?100:effects;Gains();
}
void SD_GetSettings(unsigned *music,unsigned *effects,bool *music_on,bool *effects_on)
{
    if (music) *music=music_volume;
    if (effects) *effects=effects_volume;
    if (music_on) *music_on=music_enabled;
    if (effects_on) *effects_on=effects_enabled;
}
bool SD_Ready(void) { return chip!=NULL; }
void SD_GetStatus(sd_status *out) { if(out) *out=status; }
void SD_StopSound(void)
{
    fx_data=NULL;fx_pos=fx_length=0;
    status.sound=status.digitized=-1;status.sound_priority=status.digi_priority=0;
    if(chip) Write(0xb0,0);
}
static void MusicOff(void)
{
    if(!chip) return;
    Write(0xbd,0);
    for(unsigned i=1;i<=10;++i) Write(0xb0+i,0);
}
void SD_StartMusic(int track)
{
    if(!chip) return;
    MusicOff();status.music=track>=0 && track<27?track:-1;
    music_pos=0;music_time=next_event=0;
}
void SD_SetEnabled(bool music,bool effects)
{
    bool restart=!music_enabled && music;
    if(music_enabled && !music) MusicOff();
    music_enabled=music;effects_enabled=effects;
    if(restart) SD_StartMusic(status.music);
    if(!effects) SD_StopSound();
    Gains();
}
void SD_Shutdown(void)
{
    if(chip) { OPL_delete(chip);chip=NULL; }
    for(unsigned i=0;i<DIGI_COUNT;++i) if(samples[i]) (void)MM_FreePtr(&samples[i]);
    if(header) (void)MM_FreePtr(&header);
    if(archive) (void)MM_FreePtr(&archive);
    if(swap) (void)MM_FreePtr(&swap);
    memset(chunks,0,sizeof(chunks));memset(digi,0,sizeof(digi));
    memset(&status,0,sizeof(status));status.music=status.sound=status.digitized=-1;
    fx_data=NULL;fx_pos=fx_length=0;output_rate=0;
}
static bool LoadAudio(void)
{
    if(!CA_ReadDataFile("AUDIOHED.WL6",&header,&header_size) ||
       !CA_ReadDataFile("AUDIOT.WL6",&archive,&archive_size)) return false;
    /* Some releases carry extra trailing offsets; require the WL6 288 chunks. */
    if(header_size<4*(AUDIO_CHUNKS+1) || header_size%4) return CA_Fail("Malformed AUDIOHED.WL6 offset table");
    const uint8_t *h=header,*a=archive;
    for(unsigned i=0;i<AUDIO_CHUNKS;++i) {
        size_t start=CA_LE32(h+i*4),end=CA_LE32(h+(i+1)*4);
        if(start>end || end>archive_size) return CA_Fail("Invalid AUDIOT.WL6 chunk %u bounds",i);
        chunks[i].data=a+start;chunks[i].size=end-start;
    }
    for(unsigned i=0;i<LASTSOUND;++i) {
        const uint8_t *s=chunks[87+i].data;size_t n=chunks[87+i].size;
        if(n<23 || CA_LE32(s)>n-23 || !CA_LE32(s) || !(s[12]|s[13]))
            return CA_Fail("Malformed AdLib sound %u",i);
        s=chunks[i].data;n=chunks[i].size;
        if(n<6 || CA_LE32(s)>n-6) return CA_Fail("Malformed PC sound %u",i);
    }
    for(unsigned i=MUSIC_BASE;i<AUDIO_CHUNKS;++i) {
        const uint8_t *s=chunks[i].data;size_t n=chunks[i].size;
        if(n<6 || !CA_LE16(s) || CA_LE16(s)%4 || CA_LE16(s)>n-2)
            return CA_Fail("Malformed WL6 IMF music %u",i-MUSIC_BASE);
        /* Original MusicGroup is length-prefixed; it is not generic IMF type 0. */
    }
    return true;
}
static bool LoadDigi(void)
{
    if(!CA_ReadDataFile("VSWAP.WL6",&swap,&swap_size)) return false;
    const uint8_t *s=swap;unsigned count,start;
    if(swap_size<6) goto bad;
    count=CA_LE16(s);start=CA_LE16(s+4);
    size_t directory=6+(size_t)count*6,previous=directory;
    if(count<2 || count>4096 || start>=count-1 || CA_LE16(s+2)>start || directory>swap_size) goto bad;
    for(unsigned p=0;p<count;++p) {
        size_t offset=CA_LE32(s+6+p*4),length=CA_LE16(s+6+count*4+p*2);
        if(!offset && !length) continue;
        if(!offset || !length || offset<previous || offset>swap_size || length>swap_size-offset) goto bad;
        previous=offset+length;
    }
    size_t tableoff=CA_LE32(s+6+(count-1)*4),tablelen=CA_LE16(s+6+count*4+(count-1)*2);
    if(!tableoff || tablelen<DIGI_COUNT*4) goto bad;
    for(unsigned i=0;i<DIGI_COUNT;++i) {
        const uint8_t *entry=s+tableoff+i*4;
        unsigned page=start+CA_LE16(entry);size_t left=CA_LE16(entry+2),at=0;
        if(!left || page>=count-1 || !MM_GetPtr(&samples[i],left)) goto bad;
        digi[i].data=samples[i];digi[i].size=left;
        while(left) {
            if(page>=count-1) goto bad;
            size_t offset=CA_LE32(s+6+page*4),length=CA_LE16(s+6+count*4+page*2);
            size_t copy=left>4096?4096:left;
            if(!offset || length<copy) goto bad;
            memcpy((uint8_t *)samples[i]+at,s+offset,copy);at+=copy;left-=copy;++page;
        }
    }
    (void)MM_FreePtr(&swap);return true;
bad: return CA_Fail("Malformed VSWAP.WL6 digitized directory/sample bounds");
}
bool SD_Startup(unsigned rate)
{
    SD_Shutdown();
    if(rate<8000 || rate>192000) return CA_Fail("Unsupported audio sample rate");
    if(!LoadAudio() || !LoadDigi()) { SD_Shutdown();return false; }
    /* Use the native chip rate. Resample below with integer phase; avoids
       allocations in the synthesiser's optional rate converter. */
    chip=OPL_new(3579545,49716);
    if(!chip) { SD_Shutdown();return CA_Fail("Cannot allocate OPL2 synthesiser"); }
    OPL_setChipType(chip,2);OPL_reset(chip);Write(1,0x20);Write(8,0);
    output_rate=rate;phase=rate;chip_phase=rate;fx_divider=0;digi_phase=0;music_pos=0;
    memset(chip_previous,0,sizeof(chip_previous));memset(chip_next,0,sizeof(chip_next));
    Gains();return true;
}
void SD_SetPosition(unsigned left,unsigned right)
{
    status.left=left>15?15:left;status.right=right>15?15:right;
}
bool SD_IsDigitizedSound(soundnames sound) { return digitized_mode && (unsigned)sound<LASTSOUND && digimap[sound]!=0; }
bool SD_PlaySound(soundnames sound,unsigned left,unsigned right)
{
    if(!chip || !effects_enabled || (unsigned)sound>=LASTSOUND) return false;
    const uint8_t *s=chunks[87+(unsigned)sound].data;unsigned priority=CA_LE16(s+4);
    int mapped=digitized_mode?digimap[sound]-1:-1;
    if(mapped>=0) {
        if(priority<status.digi_priority) return false;
        status.digitized=mapped;status.digi_priority=priority;
        digi_phase=0;SD_SetPosition(left,right);return true;
    }
    if(priority<status.sound_priority) return false;
    Write(0xb0,0);
    /* SDL_AlSetFXInst: clear then program both operators; connection is
       deliberately zero in Wolf's original MUSE-compatible effect driver. */
    static const unsigned regs[10]={0x20,0x23,0x40,0x43,0x60,0x63,0x80,0x83,0xe0,0xe3};
    for(unsigned i=0;i<10;++i) Write(regs[i],0);
    Write(0xc0,0);
    for(unsigned i=0;i<10;++i) Write(regs[i],s[6+i]);
    Write(0xc0,0);
    fx_data=s+23;fx_length=CA_LE32(s);fx_pos=0;fx_block=((unsigned)s[22]&7U)*4U|0x20U;
    status.sound=(int)sound;status.sound_priority=priority;return true;
}
static void Tick(void)
{
    ++status.ticks;
    /* The original ISR services effects before IMF on each fifth tick. */
    if(++fx_divider==5) {
        fx_divider=0;
        if(fx_data) {
            unsigned frequency=fx_data[fx_pos++];
            if(frequency) Write(0xa0,frequency);
            Write(0xb0,frequency?fx_block:0);
            if(fx_pos==fx_length) { fx_data=NULL;Write(0xb0,0);status.sound=-1;status.sound_priority=0; }
        }
    }
    if(status.music>=0 && music_enabled) {
        const uint8_t *song=chunks[MUSIC_BASE+(unsigned)status.music].data;
        size_t length=CA_LE16(song);
        while(music_pos<length && next_event<=music_time) {
            const uint8_t *event=song+2+music_pos;
            Write(event[0],event[1]);next_event=music_time+CA_LE16(event+2);music_pos+=4;
        }
        ++music_time;
        /* DOS restarts immediately after the final register write: the final
           event's delay is discarded, including across chip stateful loops. */
        if(music_pos==length) { music_pos=0;music_time=next_event=0;++status.loops; }
    }
}
void SD_Render(int16_t *stereo,size_t frames)
{
    if(!stereo) return;
    if(!chip) { memset(stereo,0,frames*2*sizeof(*stereo));return; }
    for(size_t i=0;i<frames;++i) {
        /* First service occurs at t=0; exact rational boundaries thereafter. */
        while(phase>=output_rate) { phase-=output_rate;Tick(); }
        while(chip_phase>=output_rate) {
            chip_phase-=output_rate;memcpy(chip_previous,chip_next,sizeof(chip_next));
            OPL_calcStereo(chip,chip_next);
        }
        int32_t l=chip_previous[0]+(int32_t)((int64_t)(chip_next[0]-chip_previous[0])*(int64_t)chip_phase/(int64_t)output_rate);
        int32_t r=chip_previous[1]+(int32_t)((int64_t)(chip_next[1]-chip_previous[1])*(int64_t)chip_phase/(int64_t)output_rate);
        phase+=700;chip_phase+=49716;
        if(status.digitized>=0) {
            unsigned d=(unsigned)status.digitized;size_t at=(size_t)(digi_phase/(UINT64_C(142)*output_rate));
            if(at>=digi[d].size) { status.digitized=-1;status.digi_priority=0; }
            else {
                int32_t sample=((int32_t)digi[d].data[at]-128)*256;
                unsigned gain=effects_enabled?effects_volume:0;
                l+=(int32_t)((int64_t)sample*gain*(15-status.left)/1500);
                r+=(int32_t)((int64_t)sample*gain*(15-status.right)/1500);
                /* Original SB constant: 256 - floor(1000000/7000) = 114.
                   Therefore DAC rate is 1000000/142, not exactly 7000 Hz. */
                digi_phase+=1000000;
            }
        }
        stereo[i*2]=(int16_t)(l>32767?32767:l< -32768?-32768:l);
        stereo[i*2+1]=(int16_t)(r>32767?32767:r< -32768?-32768:r);
        ++status.frames;
    }
}
int SD_LevelMusic(unsigned episode,unsigned floor)
{
    static const int songs[60]={3,11,9,12,3,11,9,12,2,0, 8,18,17,4,8,18,4,17,2,1,
        6,20,22,21,6,20,22,21,19,26, 3,11,9,12,3,11,9,12,2,0,
        8,18,17,4,8,18,4,17,2,1, 6,20,22,21,6,20,22,21,19,15};
    return episode<6 && floor<10?songs[episode*10+floor]:-1;
}
