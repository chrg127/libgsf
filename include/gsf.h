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

#define GSF_VERSION_MAJOR 0
#define GSF_VERSION_MINOR 1
#define GSF_VERSION ((GSF_VERSION_MAJOR << 16) | GSF_VERSION_MINOR)

/*
 * A type representing an emulator capable of playing GSF files.
 * It's usually the first parameter to library functions.
 */
typedef struct GsfEmu GsfEmu;

/* read/delete functions for gsf_load_file_custom, see below. */
// typedef int (*GsfReadFn)(void *userdata, const char *filename, unsigned char **buf, long *size);
// typedef void (*GsfDeleteFileDataFn)(unsigned char *buf);

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
    void *(*malloc)(size_t size, void *userdata);
    void  (*free)(void *ptr, size_t size, void *userdata);
    void *userdata;
} GsfAllocators;

/* Errors returned by the library. */
typedef enum GsfErrorCode {
    GSF_INVALID_FILE_SIZE = 1,
    GSF_ALLOCATION_FAILED,
    GSF_INVALID_HEADER,
    GSF_INVALID_SECTION_LENGTH,
    GSF_INVALID_CRC,
    GSF_UNCOMPRESS_ERROR,
} GsfErrorCode;

/*
 * How error handling for this library works:
 * This struct is returned by any function that could potentially error-out.
 * `code` represent an error code returned by the function. If it's 0, it's
 * guaranteed that the function returned success, whatever the value of `from`
 * is.
 * `from` represent where we got the error code. Fortunately, there are only
 * two values for it: 0, meaning it's a system error, and 1, meaning it's a
 * library-specific error.
 * If you just want to check for success, just check if <expr>.code == 0.
 */
typedef struct GsfError {
    int code;
    int from;
} GsfError;

static const GsfError GSF_NO_ERROR = {
    .code = 0,
    .from = 0,
};

/* Result of reading a file, see below. */
typedef struct GsfReadResult {
    unsigned char *buf;
    long size;
    GsfError err;
} GsfReadResult;

/*
 * This struct is used on load_file to customize how reading should be done.
 * The `read` function reads a file with the given `filename` and returns a
 * buffer (pointer+size) and an error that tells us if reading was successful.
 * The error should usually be get from the OS.
 * The `delete_data` function delete the file data allocated by `read`.
 */
typedef struct GsfReader {
    GsfReadResult (*read)(const char *filename, void *userdata, const GsfAllocators *allocators);
    void (*delete_data)(unsigned char *buf, long size, void *userdata, const GsfAllocators *allocators);
    void *userdata;
} GsfReader;

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
GSF_API GsfError gsf_new(GsfEmu **out, int sample_rate, int flags);

/* Deletes an emulator object. */
GSF_API void gsf_delete(GsfEmu *emu);

/*
 * Same as above, but takes a parameter `allocators` that the functions will use
 * to allocate memory.
 */
GSF_API GsfError gsf_new_with_allocators(GsfEmu **out, int sample_rate, int flags,
    GsfAllocators *allocators);
GSF_API void gsf_delete_with_allocators(GsfEmu *emu, GsfAllocators *allocators);

/*
 * Loads a file and any corresponding library files inside an emulator.
 * `filename` is assumed to be a valid file path.
 */
GSF_API GsfError gsf_load_file(GsfEmu *emu, const char *filename);

/*
 * Same as above, but with additional parameters `reader` and `allocators`
 * to specify how to read a file and how to allocate memory.
 */
GSF_API GsfError gsf_load_file_with_reader(GsfEmu *emu, const char *filename,
    GsfReader *reader);
GSF_API GsfError gsf_load_file_with_allocators(GsfEmu *emu,
    const char *filename, GsfAllocators *allocators);
GSF_API GsfError gsf_load_file_with_reader_allocators(GsfEmu *emu,
    const char *filename, GsfReader *reader, GsfAllocators *allocators);

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
GSF_API GsfError gsf_get_tags(const GsfEmu *emu, GsfTags **out);

/* Frees tags taken from `gsf_get_tags`. */
GSF_API void gsf_free_tags(GsfTags *tags);

/* Same pair of functions as the above two, but with allocator API. */
GSF_API GsfError gsf_get_tags_with_allocators(const GsfEmu *emu, GsfTags **out,
    GsfAllocators *allocators);
GSF_API void gsf_free_tags_with_allocators(GsfTags *tags, GsfAllocators *allocators);

/*
 * Gets the length of the file, regardless of whether gsf_infinite
 * has been set or not.
 * First functions returns it in milliseconds, second one returns it in number
 * of samples.
 * A length of -1 may returned, which means that a length tag is present, but
 * is in a bad format. Refer to psf.txt for details on a correctly formatted
 * length.
 */
GSF_API long gsf_length(GsfEmu *emu);
GSF_API long gsf_length_samples(GsfEmu *emu);

/*
 * Returns the playback position of the file. The first function return the
 * position in millisecond, the second in samples.
 */
GSF_API long gsf_tell(const GsfEmu *emu);
GSF_API long gsf_tell_samples(const GsfEmu *emu);

/*
 * Sets the currently playing file to a specified position in milliseconds or
 * in samples. Note that seeking backwards can be slow.
 */
GSF_API void gsf_seek(GsfEmu *emu, long millis);
GSF_API void gsf_seek_samples(GsfEmu *emu, long samples);

/*
 * Gets and sets the default length when parsing a file.
 * Ideally, this length should be set before loading a file (it won't modify
 * the current file's length. The default length is only used if the file tags
 * contain no length information, otherwise it will use the tags.
 */
GSF_API long gsf_default_length(const GsfEmu *emu);
GSF_API void gsf_set_default_length(GsfEmu *emu, long length);

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

/* Returns the sample rate set at creation. */
GSF_API int gsf_sample_rate(GsfEmu *emu);

/*
 * Returns the number of channels. Usually 2 (for stereo),
 * unless GSF_MULTI was used.
 */
GSF_API int gsf_num_channels(GsfEmu *emu);

#ifdef __cplusplus
}
#endif

#endif
