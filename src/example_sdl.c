#include "libgsf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <SDL.h>
#include <SDL_audio.h>

#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>

struct termios ots = {};

void term_init()
{
    struct termios ts = {};
    if (tcgetattr(STDIN_FILENO, &ts) == -1)
        return;
    ots = ts;
    ts.c_lflag &= ~(ICANON | ECHO | ECHONL);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &ts);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

void term_end()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &ots);
}

bool get_input(char *c)
{
    return read(STDIN_FILENO, c, 1) == 1;
}

// note that using SDL's audio libraries with a small buffer size
// can result in garbage being played alongside the music.
// if you wanna test it, simply set the value below to 16
const int NUM_SAMPLES = 1024;
const int NUM_CHANNELS = 2;
const int BUF_SIZE = NUM_CHANNELS * NUM_SAMPLES;

void sdl_callback(void *userdata, unsigned char *stream, int length)
{
    GsfEmu *emu = (GsfEmu *) userdata;
    memset(stream, 0, length);
    if (!gsf_track_ended(emu)) {
        short samples[BUF_SIZE];
        gsf_play(emu, samples, BUF_SIZE);
        memcpy(stream, samples, sizeof(samples));
    }
    printf("\r%d samples, %d millis, %d seconds",
        gsf_tell_samples(emu),
        gsf_tell(emu),
        gsf_tell(emu) / 1000);
    fflush(stdout);
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

    GsfTags *tags;
    gsf_get_tags(emu, &tags);
    printf(
        "title: %s\n"
        "artist: %s\n"
        "game: %s\n"
        "year: %d\n"
        "genre: %s\n"
        "comment: %s\n"
        "copyright: %s\n"
        "gsfby: %s\n"
        "volume: %f\n"
        "fade: %d\n",
        tags->title, tags->artist, tags->game,
        tags->year, tags->genre, tags->comment,
        tags->copyright, tags->gsfby,
        tags->volume, tags->fade
    );
    printf("length: %d\n", gsf_length(emu));
    gsf_free_tags(tags);

    term_init();

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
        char c = 1;
        if (get_input(&c)) {
            switch (c) {
            case 'l':
                SDL_LockAudioDevice(dev);
                gsf_seek(emu, 1000);
                SDL_UnlockAudioDevice(dev);
                break;
            }
        }
    }

    SDL_CloseAudioDevice(dev);
    gsf_delete(emu);

    term_end();

    return 0;
}
