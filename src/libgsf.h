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
typedef int (*GsfReadFn)(void *userdata, const char *filename, unsigned char **buf, long *size);
typedef void (*GsfDeleteFileDataFn)(unsigned char *buf);

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
GSF_API int gsf_load_file_custom(GsfEmu *emu, const char *filename,
    void *userdata, GsfReadFn read_fn, GsfDeleteFileDataFn delete_fn);
GSF_API bool gsf_loaded(const GsfEmu *emu);
GSF_API void gsf_play(GsfEmu *emu, short *out, long size);
GSF_API bool gsf_track_ended(const GsfEmu *emu);
GSF_API int gsf_get_tags(const GsfEmu *emu, GsfTags **out);
GSF_API void gsf_free_tags(GsfTags *tags);
GSF_API long gsf_tell(const GsfEmu *emu);
GSF_API long gsf_tell_samples(const GsfEmu *emu);
GSF_API void gsf_seek(GsfEmu *emu, long millis);
GSF_API void gsf_seek_samples(GsfEmu *emu, long samples);
GSF_API void gsf_set_default_length(GsfEmu *emu, long length);
GSF_API long gsf_default_length(const GsfEmu *emu, long length);
GSF_API void gsf_set_infinite(GsfEmu *emu, bool infinite);

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

#ifdef __cplusplus
}
#endif

#endif
