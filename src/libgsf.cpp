#include "libgsf.h"

unsigned int gsf_get_version(void)
{
    return GSF_VERSION;
}

bool gsf_is_compatible_dll(void)
{
    unsigned major = gsf_get_version() >> 16;
    return major == GSF_VERSION_MAJOR;
}

struct GsfEmu {

};

GSF_API int gsf_new(GsfEmu **out, int frequency)
{
    return 0;
}

GSF_API int gsf_load_data(GsfEmu *emu, unsigned char *data, size_t size)
{
    return 0;
}

GSF_API void gsf_play(GsfEmu *emu, short *out, size_t size)
{

}

GSF_API bool gsf_track_ended(GsfEmu *emu)
{
    return true;
}

GSF_API void gsf_delete(GsfEmu *emu)
{

}
