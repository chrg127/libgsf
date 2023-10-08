#include "libgsf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("no input file\n");
        return 1;
    }

    GsfEmu *emu;
    if (gsf_new(&emu, 44100) != 0) {
        printf("couldn't create emulator\n");
        return 1;
    }

    // set file reader function here if you want

    if (gsf_load_file(emu, argv[1]) != 0) {
        printf("couldn't load file inside emulator\n");
        return 1;
    }

    while (!gsf_track_ended(emu)) {
        short samples[16];
        gsf_play(emu, samples, 16);
        /* do something with samples */
    }
    gsf_delete(emu);

    return 0;
}
