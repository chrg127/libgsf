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
        #if defined(GSF_BUILD_SHARED) /* build dll */
            #define GSF_API __declspec(dllexport)
        #elif !defined(GSF_BUILD_STATIC) /* use dll */
            #define GSF_API __declspec(dllimport)
        #else   /* static library */
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

unsigned int gsf_get_version(void);
bool gsf_is_compatible_dll(void);

typedef struct GsfEmu GsfEmu;

GSF_API int gsf_new(GsfEmu **out, int frequency);
GSF_API int gsf_load_data(GsfEmu *emu, unsigned char *data, size_t size);
GSF_API void gsf_play(GsfEmu *emu, short *out, size_t size);
GSF_API bool gsf_track_ended(GsfEmu *emu);
GSF_API void gsf_delete(GsfEmu *emu);

#ifdef __cplusplus
}
#endif

#endif
