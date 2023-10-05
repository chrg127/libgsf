#include "libgsf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

bool read_file(const char *filename, unsigned char **buf, long *size)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
        return false;
    fseek(file, 0l, SEEK_END);
    *size = ftell(file);
    rewind(file);
    *buf = calloc(*size, sizeof(unsigned char));
    size_t bytes_read = fread(*buf, sizeof(char), *size, file);
    if (bytes_read < (size_t)*size) {
        free(*buf);
        return false;
    }
    fclose(file);
    return true;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("no input file\n");
        return 1;
    }

    unsigned char *buf;
    long size;
    if (!read_file(argv[1], &buf, &size)) {
        printf("couldn't read file %s\n", argv[1]);
        return 1;
    }

    GsfEmu *emu;
    if (gsf_new(&emu, 44100) != 0) {
        printf("couldn't create emulator\n");
        return 1;
    }

    if (gsf_load_data(emu, buf, size) != 0) {
        printf("couldn't load data inside emulator\n");
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
