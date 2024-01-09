/* This is the C interface for libgsf. It's also usable from C++. */

#ifndef GSF_H_INCLUDED
#define GSF_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* export markers */
#ifndef GSF_API
    #ifdef _WIN32
        #if defined(GSF_BUILD_SHARED)       /* build dll */
            #define GSF_API __declspec(dllexport)
        #elif !defined(GSF_BUILD_STATIC)    /* use dll */
            #define GSF_API __declspec(dllimport)
        #else                               /* static library */
            #define GSF_API
        #endif
    #else
        #if __GNUC__ >= 4
            #define GSF_API __attribute__((visibility("default")))
        #else
            #define GSF_API
        #endif
    #endif
#endif

#define GSF_VERSION_MAJOR 1
#define GSF_VERSION_MINOR 1
#define GSF_VERSION ((GSF_VERSION_MAJOR << 16) | GSF_VERSION_MINOR)

/*
 * A type representing an emulator capable of playing GSF files.
 * It's usually the first parameter to library functions.
 */
typedef struct GsfEmu GsfEmu;

/* read/delete functions for gsf_load_file_custom, see below. */
typedef int (*GsfReadFn)(void *userdata, const char *filename, unsigned char **buf, long *size);
typedef void (*GsfDeleteFileDataFn)(unsigned char *buf);

/* Flags passed to gsf_new, see below. */
typedef enum GsfFlags {
    GSF_INFO_ONLY   = 1 << 1,
    GSF_MULTI       = 1 << 2,
} GsfFlags;

/* A type representing the tags inside a GSF file. Returned by gsf_get_tags, see below. */
typedef struct GsfTags {
    const char *title;
    const char *artist;
    const char *game;
    int year;
    const char *genre;
    const char *comment;
    const char *copyright;
    const char *gsfby;
    /*
     * The following two tags (volume and fade) are completely ignored by the library,
     * but we include them for the sake of completeness.
     */
    double volume;
    int fade;
} GsfTags;

/* Custom allocator support, see below. */
typedef struct GsfAllocators {
    void *(*malloc)(size_t, void *userdata);
    void  (*free)(void *, void *userdata);
    void *userdata;
} GsfAllocators;


typedef enum GsfError {
    GSF_INVALID_FILE_SIZE = 1,
    GSF_INVALID_HEADER,
    GSF_INVALID_SECTION_LENGTH,
    GSF_INVALID_CRC,
    GSF_UNCOMPRESS_ERROR,
} GsfError;

/*
 * These two functions get and check the library version, respectively.
 * They can be used to test if you've got any installation errors.
 */
GSF_API unsigned int gsf_get_version(void);
GSF_API bool gsf_is_compatible_version(void);

/*
 * Creates a new emulator with a specific `frequency` and `flags`.
 * The following flags are defined:
 * - GSF_INFO_ONLY: creates an emulator used only for retrieving tags.
 * - GSF_MULTI: creates an emulator that will output audio on multiple
 *   channels instead of a single one.
 * (NOTE: these flags are still unsupported!)
 */
GSF_API int gsf_new(GsfEmu **out, int frequency, int flags);

/* Deletes an emulator object. */
GSF_API void gsf_delete(GsfEmu *emu);

/*
 * Loads a file and any corresponding library files inside an emulator.
 * `filename` is assumed to be a valid file path.
 */
GSF_API int gsf_load_file(GsfEmu *emu, const char *filename);

/*
 * Same as above, but with custom functions for reading and deleting file data.
 * read_fn should read all of file's data and return it inside the parameters buf and size.
 * It can return any OS error codes it finds.
 * delete_fn should delete the memory allocated by read_fn.
 */
GSF_API int gsf_load_file_custom(GsfEmu *emu, const char *filename,
    void *userdata, GsfReadFn read_fn, GsfDeleteFileDataFn delete_fn);

/* Checks if any files are loaded inside an emulator. */
GSF_API bool gsf_loaded(const GsfEmu *emu);

/*
 * Generates `size` 16-bit signed stereo samples inside out from an emulator.
 * Generates them in one single channel (I.E. no need to do any mixing).
 */
GSF_API void gsf_play(GsfEmu *emu, short *out, long size);

/* Checks if an emulator has finished playing a loaded file. */
GSF_API bool gsf_ended(const GsfEmu *emu);

/* Returns the tags found from a loaded GSF file. */
GSF_API int gsf_get_tags(const GsfEmu *emu, GsfTags **out);

/* Frees tags taken from `gsf_get_tags`. */
GSF_API void gsf_free_tags(GsfTags *tags);

/*
 * Gets the length of the file, regardless of whether gsf_infinite
 * has been set or not.
 */
GSF_API long gsf_length(GsfEmu *emu);

/* Number of milliseconds played since the beginning of the file. */
GSF_API long gsf_tell(const GsfEmu *emu);

/* Same as above, but in samples. */
GSF_API long gsf_tell_samples(const GsfEmu *emu);

/*
 * Sets the currently playing file to a specified position in milliseconds.
 * Can only seek forward.
 */
GSF_API void gsf_seek(GsfEmu *emu, long millis);

/* Same as above, but in samples. */
GSF_API void gsf_seek_samples(GsfEmu *emu, long samples);

/*
 * Sets the default length when parsing a file. Ideally should be set before
 * loading a file (it won't modify the current file's length.
 * The default length is only used if the file tags contain no length
 * information, otherwise it will use the tags.
 */
GSF_API void gsf_set_default_length(GsfEmu *emu, long length);

/* Returns the default length set. Defaults to 0. */
GSF_API long gsf_default_length(const GsfEmu *emu);

/*
 * Sets whether playing should be infinite for the currently playing file.
 * If set to `false` while playing, and the currently playing file is
 * theoretically already at the end, then playback will stop and `gsf_ended`
 * will immediately return `true`.
 * If set to `true` while playing, playback may be resumed.
 */
GSF_API void gsf_set_infinite(GsfEmu *emu, bool infinite);

/*
 * Part of the allocation API.
 * This functions sets two allocators for use inside the library. The `malloc`
 * function allocates memory; then `free` functions deallocates it. An additional
 * parameter `userdata` is passed to these functions for custom use.
 * NOTE: this API is still a WIP, since not everything will pass through these
 * two functions.
 */
GSF_API void gsf_set_allocators(GsfAllocators *allocators);

#ifdef __cplusplus
}
#endif

#endif
