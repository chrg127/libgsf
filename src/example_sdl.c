#include "libgsf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <SDL.h>
#include <SDL_audio.h>

// note that using SDL's audio libraries with a small buffer size
// can result in garbage being played alongside the music.
// if you wanna test it, simply set the value below to 16
const int NUM_SAMPLES = 1024;
const int NUM_CHANNELS = 2;
const int BUF_SIZE = NUM_CHANNELS * NUM_SAMPLES;

void sdl_callback(void *userdata, unsigned char *stream, int length)
{
    GsfEmu *emu = (GsfEmu *) userdata;
    if (!gsf_track_ended(emu)) {
        short samples[BUF_SIZE];
        gsf_play(emu, samples, BUF_SIZE);
        memcpy(stream, samples, sizeof(samples));
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("no input file\n");
        return 1;
    }

    GsfEmu *emu;
    if (gsf_new(&emu, 44100, 0) != 0) {
        printf("couldn't create emulator\n");
        return 1;
    }

    if (gsf_load_file(emu, argv[1]) != 0) {
        printf("couldn't load file inside emulator\n");
        return 1;
    }

    SDL_Init(SDL_INIT_AUDIO);
    SDL_AudioSpec spec;
    spec.freq     = 44100;
    spec.format   = AUDIO_S16SYS;
    spec.channels = NUM_CHANNELS;
    spec.samples  = NUM_SAMPLES;
    spec.userdata = emu;
    spec.callback = sdl_callback;
    int dev = SDL_OpenAudioDevice(NULL, 0, &spec, &spec, 0);

    SDL_PauseAudioDevice(dev, 0);

    for (bool running = true; running; ) {
        for (SDL_Event ev; SDL_PollEvent(&ev); ) {
            switch (ev.type) {
            case SDL_QUIT:
                running = false;
                break;
            }
        }
    }

    SDL_CloseAudioDevice(dev);
    gsf_delete(emu);

    return 0;
}
