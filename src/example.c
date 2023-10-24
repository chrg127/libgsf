#include "libgsf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

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

    // set file reader function here if you want

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
        "length: %d\n"
        "fade: %d\n",
        tags->title, tags->artist, tags->game,
        tags->year, tags->genre, tags->comment,
        tags->copyright, tags->gsfby,
        tags->volume, tags->length, tags->fade
    );
    gsf_free_tags(tags);

    while (!gsf_track_ended(emu)) {
        short samples[16];
        gsf_play(emu, samples, 16);
        printf("\r%d samples, %d millis %d seconds", gsf_tell_samples(emu), gsf_tell(emu), gsf_tell(emu) / 1000);
        fflush(stdout);
        usleep(1000);
    }

    gsf_delete(emu);

    return 0;
}
