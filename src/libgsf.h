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

typedef struct GsfEmu GsfEmu;

typedef enum GsfFlags {
    GSF_INFO_ONLY   = 1 << 1,
    GSF_MULTI       = 1 << 2,
} GsfFlags;

typedef struct GsfTags {
    const char *title;
    const char *artist;
    const char *game;
    int year;
    const char *genre;
    const char *comment;
    const char *copyright;
    const char *gsfby;
    double volume;
    int length;
    int fade;
} GsfTags;

GSF_API unsigned int gsf_get_version(void);
GSF_API bool gsf_is_compatible_dll(void);
GSF_API int gsf_new(GsfEmu **out, int frequency, int flags);
GSF_API void gsf_delete(GsfEmu *emu);
GSF_API int gsf_load_file(GsfEmu *emu, const char *filename);
GSF_API void gsf_play(GsfEmu *emu, short *out, size_t size);
GSF_API bool gsf_track_ended(GsfEmu *emu);
GSF_API int gsf_get_tags(GsfEmu *emu, GsfTags **out);
GSF_API void gsf_free_tags(GsfTags *tags);
GSF_API size_t gsf_tell(GsfEmu *emu);
GSF_API size_t gsf_tell_samples(GsfEmu *emu);

// gsf_seek
// gsf_seek_samples
// gsf_set_fade
// gsf_set_tempo
// gsf_channel_count -- probably should be a constant
// gsf_channel_name
// gsf_mute_channel
// gsf_set_channel_volume

/* memory allocation customization */
GSF_API void gsf_set_allocators(
    void *(*malloc_fn)(size_t),
    void *(*realloc_fn)(void *, size_t),
    void (*free_fn)(void *)
);

/* Setups functions used to read files. */
typedef int (*GsfReadFn)(void *userdata, const char *filename, unsigned char **buf, long *size);
typedef void (*GsfDeleteFileDataFn)(unsigned char *buf);

GSF_API void gsf_set_file_reader(
    void *userdata,
    GsfReadFn read_fn,
    GsfDeleteFileDataFn delete_fn
);

#ifdef __cplusplus
}
#endif

#endif
