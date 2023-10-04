#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <libgen.h>
#include <ao/ao.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include "VBA/GBA.h"
#include "VBA/Sound.h"
#include "VBA/Util.h"
#include "VBA/psftag.h"

#define HA_VERSION_STR "0.11"
#define VERSION_STR "0.07"
#define BOLD() printf("%c[36m", 27);
#define NORMAL() printf("%c[0m", 27);
#define EMU_COUNT 250000

int emulating = 0;
bool debugger = false;
bool systemSoundOn = false;

extern "C" {
int defvolume=1000;
int relvolume=1000;
int TrackLength=0;
int FadeLength=0;
int IgnoreTrackLength, DefaultLength=150000;
int playforever=0;
int TrailingSilence=1000;
int DetectSilence=0, silencedetected=0, silencelength=5;
int cpupercent=0, sndSamplesPerSec, sndNumChannels;
int sndBitsPerSample=16;
int deflen=120,deffade=4;
}

extern int soundBufferLen;
extern char soundEcho;
extern char soundLowPass;
extern char soundReverse;
double decode_pos_ms; // current decoding position, in milliseconds
int seek_needed; // if != -1, it is the point that the decode thread should seek to, in ms.

static int g_playing = 0;
static int g_must_exit = 0;
static ao_device *snd_ao;
static int fileoutput=0;

extern "C" int LengthFromString(const char * timestring);
extern "C" int VolumeFromString(const char * volumestring);

extern "C" void end_of_track()
{
    g_playing = 0;
}

extern "C" void writeSound(void)
{
    int tmp;
    int ret = soundBufferLen;
    ao_play(snd_ao, (char*)soundFinalWave, ret);
    decode_pos_ms += (ret/(2*sndNumChannels) * 1000)/(float)sndSamplesPerSec;
}

void systemSoundShutdown() { }
void systemSoundPause() { }
void systemSoundReset() { }
void systemSoundResume() { }

void systemWriteDataToSoundBuffer()
{
    writeSound();
}

bool systemCanChangeSoundQuality()
{
    return true;
}

bool IsTagPresent(unsigned char Test[5])
{
    return (*((unsigned*)&Test[0]) == 0x4741545b && Test[4] == 0x5D);
}

bool IsValidGSF(unsigned char Test[4])
{
    return (*((unsigned*)&Test[0]) == 0x22465350);
}

void setupSound(void)
{
    sndNumChannels = 2;
    switch (soundQuality) {
    case 2:
        sndSamplesPerSec = 22050;
        soundBufferLen = 736 * 2;
        // soundBufferTotalLen = 7360*2;
        break;
    case 4:
        sndSamplesPerSec = 11025;
        soundBufferLen = 368 * 2;
        // soundBufferTotalLen = 3680*2;
        break;
    default:
        soundQuality = 1;
        sndSamplesPerSec = 44100;
        // soundBufferLen = 1470*2;
        soundBufferLen = 576 * 2 * 2;
        // soundBufferTotalLen = 14700*2;
        // soundBufferTotalLen = 576*2*10*2;
    }
    sndBitsPerSample = 16;
    systemSoundOn = true;
}

int GSFRun(char* filename)
{
    static int soundInitialized = false;

    if (rom != NULL) {
        CPUCleanUp();
        emulating = false;
    }

    IMAGE_TYPE type = utilFindType(filename);
    if (type == IMAGE_UNKNOWN) {
        fprintf(stderr, "Unsupported\n");
        return false;
    }

    int size = CPULoadRom(filename);
    if (!size)
        return false;

    if (soundInitialized) {
        soundReset();
    } else {
        if (!soundOffFlag)
            soundInit();
        soundInitialized = true;
    }

    if (type == IMAGE_GBA) {
        // CPUInit((char *)(LPCTSTR)theApp.biosFileName, theApp.useBiosFile ? true : false);
        CPUInit((char*)NULL, false); // don't use bios file
        CPUReset();
    }

    emulating = true;
    return true;
}

void GSFClose(void)
{
    if (rom != NULL /*|| gbRom != NULL*/) {
        soundPause();
        CPUCleanUp();
    }
    emulating = 0;
}

bool EmulationLoop(void)
{
    if (emulating /*&& !paused*/) {
        for (int i = 0; i < 2; i++) {
            CPULoop(EMU_COUNT);
            return true;
        }
    }
    return false;
}

static void signal_handler(int sig)
{
    g_playing = 0;
    g_must_exit = 1;
}

static void shuffle_list(char *filelist[], int num_files)
{
    int i, n;
    char *tmp;
    srand((int)time(NULL));
    for (i=0; i<num_files; i++)
    {
        tmp = filelist[i];
        n = (int)((double)num_files*rand()/(RAND_MAX+1.0));
        filelist[i] = filelist[n];
        filelist[n] = tmp;
    }
}

int main(int argc, char **argv)
{
    int r, tmp, fi, random=0;
    char Buffer[1024];
    char length_str[256], fade_str[256], volume[256], title_str[256];
    char tmp_str[256];

    soundLowPass = 0;
    soundEcho = 0;
    soundQuality = 0;
    DetectSilence=1;
    silencelength=5;
    IgnoreTrackLength=0;
    DefaultLength=150000;
    TrailingSilence=1000;
    playforever=0;

    while((r=getopt(argc, argv, "hlsrieWL:t:"))>=0)
    {
        char *e;
        switch(r)
        {
            case 'h':
                printf("playgsf version %s (based on Highly Advanced version %s)\n\n",
                        VERSION_STR, HA_VERSION_STR);
                printf("Usage: ./playgsf [options] files...\n\n");
                printf("  -l        Enable low pass filer\n");
                printf("  -s        Detect silence\n");
                printf("  -L        Set silence length in seconds (for detection). Default 5\n");
                printf("  -t        Set default track length in milliseconds. Default 150000 ms\n");
                printf("  -i        Ignore track length (use default length)\n");
                printf("  -e        Endless play\n");
                printf("  -r        Play files in random order\n");
                printf("  -W        output to 'output.wav' rather than soundcard\n");
                printf("  -h        Displays what you are reading right now\n");
                return 0;
                break;
            case 'i':
                IgnoreTrackLength = 1;
                break;
            case 'l':
                soundLowPass = 1;
                break;
            case 's':
                DetectSilence = 1;
                break;
            case 'L':
                silencelength = strtol(optarg, &e, 0);
                if (e==optarg) {
                    fprintf(stderr, "Bad value\n");
                    return 1;
                }
                break;
            case 'e':
                playforever = 1;
                break;
            case 't':
                DefaultLength = strtol(optarg, &e, 0);
                if (e==optarg) {
                    fprintf(stderr, "Bad value\n");
                    return 1;
                }
                break;
            case 'r':
                random = 1;
                break;
            case 'W':
                fileoutput = 1;
                break;
            case '?':
                fprintf(stderr, "Unknown argument. try -h\n");
                return 1;
                break;
        }
    }

    if (argc-optind<=0) {
        printf("No files specified! For help, try -h\n");
        return 1;
    }


    if (random) { shuffle_list(&argv[optind], argc-optind); }

    printf("playgsf version %s (based on Highly Advanced version %s)\n\n",
                VERSION_STR, HA_VERSION_STR);

    signal(SIGINT, signal_handler);

    fi = optind;
    while (!g_must_exit && fi < argc)
    {
        decode_pos_ms = 0;
        seek_needed = -1;
        TrailingSilence=1000;

        r = GSFRun(argv[fi]);
        if (!r) {
            fi++;
            continue;
        }

        g_playing = 1;

        void *tag = psftag_create();
        psftag_readfromfile(tag, argv[fi]);

        BOLD(); printf("Filename: "); NORMAL();
        printf("%s\n", basename(argv[fi]));
        BOLD(); printf("Channels: "); NORMAL();
        printf("%d\n", sndNumChannels);
        BOLD(); printf("Sample rate: "); NORMAL();
        printf("%d\n", sndSamplesPerSec);

        if (!psftag_getvar(tag, "title", title_str, sizeof(title_str)-1)) {
            BOLD(); printf("Title: "); NORMAL();
            printf("%s\n", title_str);
        }

        if (!psftag_getvar(tag, "artist", tmp_str, sizeof(tmp_str)-1)) {
            BOLD(); printf("Artist: "); NORMAL();
            printf("%s\n", tmp_str);
        }

        if (!psftag_getvar(tag, "game", tmp_str, sizeof(tmp_str)-1)) {
            BOLD(); printf("Game: "); NORMAL();
            printf("%s\n", tmp_str);
        }

        if (!psftag_getvar(tag, "year", tmp_str, sizeof(tmp_str)-1)) {
            BOLD(); printf("Year: "); NORMAL();
            printf("%s\n", tmp_str);
        }

        if (!psftag_getvar(tag, "copyright", tmp_str, sizeof(tmp_str)-1)) {
            BOLD(); printf("Copyright: "); NORMAL();
            printf("%s\n", tmp_str);
        }

        if (!psftag_getvar(tag, "gsfby", tmp_str, sizeof(tmp_str)-1)) {
            BOLD(); printf("GSF By: "); NORMAL();
            printf("%s\n", tmp_str);
        }

        if (!psftag_getvar(tag, "tagger", tmp_str, sizeof(tmp_str)-1)) {
            BOLD(); printf("Tagger: "); NORMAL();
            printf("%s\n", tmp_str);
        }

        if (!psftag_getvar(tag, "comment", tmp_str, sizeof(tmp_str)-1)) {
            BOLD(); printf("Comment: "); NORMAL();
            printf("%s\n", tmp_str);
        }

        if (!psftag_getvar(tag, "fade", fade_str, sizeof(fade_str)-1)) {
            FadeLength = LengthFromString(fade_str);
            BOLD(); printf("Fade: "); NORMAL();
            printf("%s (%d ms)\n", fade_str, FadeLength);
        }

        if (!psftag_getvar(tag, "length", length_str, sizeof(length_str)-1)) {
            TrackLength = LengthFromString(length_str) + FadeLength;
            BOLD(); printf("Length: "); NORMAL();
            printf("%s (%d ms) ", length_str, TrackLength);
            if (IgnoreTrackLength) {
                printf("(ignored)");
                TrackLength = DefaultLength;
            }
            printf("\n");
        } else {
            TrackLength = DefaultLength;
        }

        psftag_delete(tag);

        /* Must be done after GSFrun so sndNumchannels and
         * sndSamplesPerSec are set to valid values */
        ao_initialize();
        ao_sample_format format_ao = {
          16, sndSamplesPerSec, sndNumChannels, AO_FMT_LITTLE
        };
        if(fileoutput) {
          snd_ao = ao_open_file(ao_driver_id("wav"), "output.wav", 1, &format_ao, NULL);
        } else {
          snd_ao = ao_open_live(ao_default_driver_id(), &format_ao, NULL);
        }

        while(g_playing)
        {
            int remaining = TrackLength - (int)decode_pos_ms;
            if (remaining<0) {
                // this happens during silence period
                remaining = 0;
            }
            EmulationLoop();

            BOLD(); printf("Time: "); NORMAL();
            printf("%02d:%02d.%02d ",
                    (int)(decode_pos_ms/1000.0)/60,
                    (int)(decode_pos_ms/1000.0)%60,
                    (int)(decode_pos_ms/10.0)%100);
            if (!playforever) {
                printf("[");
                printf("%02d:%02d.%02d", remaining/1000/60, (remaining/1000)%60, (remaining/10%100));
                printf("] of ");
                printf("%02d:%02d.%02d ", TrackLength/1000/60, (TrackLength/1000)%60, (TrackLength/10%100));
            }
            BOLD(); printf("  GBA Cpu: "); NORMAL();
            printf("%02d%% ", cpupercent);
            printf("     \r");

            fflush(stdout);
        }
        printf("\n--\n");
        ao_close(snd_ao);
        fi++;
    }

    ao_shutdown();
    GSFClose();
    return 0;
}
