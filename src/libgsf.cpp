#include "libgsf.h"

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <span>
#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <utility>
#include <algorithm>
#include <unordered_map>
#include <strings.h>
#include <zlib.h>
#include <tl/expected.hpp>
#include "allocation.hpp"

/* utilities */

using u8 = unsigned char;
using u32 = uint32_t;

template <typename T>
using Vector = std::vector<T, GsfAllocator<T>>;

using String = std::basic_string<char, std::char_traits<char>, GsfAllocator<char>>;

bool casecmp(const char *a, const char *b, size_t n)
{
    return strncasecmp(a, b, n) == 0;
}

struct CaseCmpStruct {
    constexpr bool operator()(const String &lhs, const String &rhs) const
    {
        return lhs.size() != rhs.size()
            && casecmp(lhs.data(), rhs.data(), lhs.size());
    }
};

using TagMap = std::unordered_map<String, String, std::hash<String>, CaseCmpStruct>;

inline bool is_space(char c) { return c == ' ' || c == '\t' || c == '\r'; }

template <typename T = std::string>
inline void split(const T &s, char delim, auto &&fn)
{
    for (std::size_t i = 0, p = 0; i != s.size(); i = p+1) {
        p = s.find(delim, i);
        fn(s.substr(i, (p == s.npos ? s.size() : p) - i));
        if (p == s.npos)
            break;
    }
}

template <typename From = std::string, typename To = std::string>
inline To trim(const From &s)
{
    auto i = std::find_if_not(s.begin(),  s.end(),  is_space);
    auto j = std::find_if_not(s.rbegin(), s.rend(), is_space).base();
    return {i, j};
}

inline std::string_view trim_view(std::string_view s) { return trim<std::string_view, std::string_view>(s); }

template <typename T>
u32 read4(T ptr)
{
    return ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
}



/* allocation */

GsfAllocators allocators = {
    malloc, realloc, free
};

GSF_API void gsf_set_allocators(
    void *(*malloc_fn)(size_t),
    void *(*realloc_fn)(void *, size_t),
    void (*free_fn)(void *)
)
{
    allocators.malloc_fn = malloc_fn;
    allocators.realloc_fn = realloc_fn;
    allocators.free_fn = free_fn;
}



/* gsf parsing */

struct Rom {
    u32 entry_point;
    u32 offset;
    Vector<u8> data;
};

struct GSFFile {
    std::span<u8> reserved;
    Rom rom;
    TagMap tags;
};

tl::expected<Rom, int> uncompress_rom(std::span<u8> data, u32 crc)
{
    if (crc != crc32(crc32(0l, nullptr, 0), data.data(), data.size()))
        return tl::unexpected(0);
    // uncompress first 12 bytes first, which tells us the entry point, the offset and the size of the rom
    std::array<u8, 12> tmp;
    unsigned long size = 12;
    if (uncompress(tmp.data(), &size, data.data(), data.size()) != Z_BUF_ERROR)
        return tl::unexpected(0);
    // re-uncompress, now with the real size;
    size = read4(&tmp[8]) + 12;
    auto uncompressed = Vector<u8>(size, 0);
    if (uncompress(uncompressed.data(), &size, data.data(), data.size()) != Z_OK)
        return tl::unexpected(0);
    return Rom {
        .entry_point = read4(&tmp[0]),
        .offset      = read4(&tmp[4]),
        .data        = std::move(uncompressed)
    };
}

TagMap parse_tags(std::string_view tags)
{
    TagMap result;
    split(tags, '\n', [&] (std::string_view tag) {
        auto equals = tag.find('=');
        auto first = trim_view(tag.substr(0, equals));
        auto second = trim_view(tag.substr(equals + 1, tag.size()));
        /* currently lacking multiline variables. */
        result[String(first)] = String(second);
    });
    return result;
}

tl::expected<GSFFile, int> parse(std::span<u8> data, int libnum = 1)
{
    if (data.size() < 0x10 || data.size() > 0x4000000)
        return tl::unexpected(0);
    if (data[0] != 'P' || data[1] != 'S' || data[2] != 'F' || data[3] != 0x22)
        return tl::unexpected(0);
    int cursor = 4;
    auto readb = [&](size_t bytes) { auto p = &data[cursor]; cursor += bytes; return std::span{p, bytes}; };
    u32 reserved_length = read4(readb(4));
    u32 program_length  = read4(readb(4));
    u32 crc             = read4(readb(4));
    if (reserved_length + program_length + 16 > data.size())
        return tl::unexpected(0);
    auto reserved = readb(reserved_length);
    auto rom = program_length > 0 ? uncompress_rom(readb(program_length), crc) : Rom{};
    if (!rom)
        return tl::unexpected(rom.error());
    auto tags = casecmp((const char *)readb(5).data(), "[TAG]", 5)
              ? readb(std::min<size_t>(data.size() - cursor, 50000u))
              : std::span<u8>{};
    return GSFFile {
        .reserved = reserved,
        .rom = std::move(rom.value()),
        .tags = parse_tags(std::string_view((char *) tags.data(), tags.size())),
    };
}



/* public API functions */

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
    auto result = parse(std::span{data, size});
    if (!result)
        return 1;
    GSFFile gsf = std::move(result.value());
    printf("rom size = %d\n", gsf.rom.data.size());
    printf("entry point = %0X\n", gsf.rom.entry_point);
    printf("offset = %0X\n", gsf.rom.offset);
    for (auto &[k, v] : gsf.tags) {
        printf("tag: %s = %s\n", k.c_str(), v.c_str());
    }
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
