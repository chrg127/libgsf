#include "gsf.h"

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <limits>
#include <span>
#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <utility>
#include <algorithm>
#include <unordered_map>
#include <filesystem>
#include <optional>
#include <bit>
#include <strings.h>
#include <zlib.h>
#include <tl/expected.hpp>
#include <mgba/gba/core.h>
#include <mgba/core/core.h>
#include <mgba/core/blip_buf.h>
#include <mgba/core/log.h>
#include <mgba-util/vfs.h>
#include "allocation.hpp"
#include "string.hpp"



/* allocation */

namespace detail {

void *malloc(size_t size, void *) {
    return ::malloc(size);
}
void free(void *p, void *) { ::free(p); }

} // namespace detail

GsfAllocators allocators = { detail::malloc, detail::free, nullptr};

int default_read_file(void *, const char *filename,
    unsigned char **buf, long *size)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
        return 1;
    fseek(file, 0l, SEEK_END);
    *size = ftell(file);
    rewind(file);
    *buf = gsf_allocate<unsigned char>(*size);
    size_t bytes_read = fread(*buf, sizeof(char), *size, file);
    if (bytes_read < (size_t)*size) {
        allocators.free(*buf, allocators.userdata);
        return 1;
    }
    fclose(file);
    return 0;
}

void default_delete_file_data(unsigned char *buf)
{
    allocators.free(buf, allocators.userdata);
}

void *read_file_userdata;



/* utilities */

namespace fs = std::filesystem;

using u8 = unsigned char;
using u32 = uint32_t;
template <typename T> using Vector = std::vector<T, GsfAllocator<T>>;
using String = std::basic_string<char, std::char_traits<char>, GsfAllocator<char>>;
template <typename T> using Result = tl::expected<T, int>;

struct InsensitiveCompare {
    bool operator()(const String &lhs, const String &rhs) const
    {
        return lhs.size() == rhs.size()
            && strncasecmp(lhs.data(), rhs.data(), lhs.size()) == 0;
    }
};

// GCC version < 10 doesn't handle std::hash<std::basic_string> with custom allocators
struct StringHash {
    std::size_t operator()(const String &s) const noexcept {
        return std::hash<std::string_view>{}(s);
    }
};

using TagMap = std::unordered_map<String, String, StringHash, InsensitiveCompare, GsfAllocator<std::pair<const String, String>>>;

template <typename T>
u32 read4(T ptr)
{
    return ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
}

template <typename T, typename Deleter = std::default_delete<T>>
struct ManagedBuffer {
    std::unique_ptr<T[], Deleter> ptr;
    std::size_t size;
    std::span<      T> to_span()       { return std::span<T>(ptr.get(), size); }
    std::span<const T> to_span() const { return std::span<T>(ptr.get(), size); }
};

Result<ManagedBuffer<u8, GsfDeleteFileDataFn>> read_file(fs::path filepath,
    void *, GsfReadFn read_fn, GsfDeleteFileDataFn delete_fn)
{
    u8 *buf;
    long size;
    auto err = read_fn(read_file_userdata, filepath.string().c_str(), &buf, &size);
    if (err != 0)
        return tl::unexpected(err);
    return ManagedBuffer {
        .ptr = std::unique_ptr<u8[], GsfDeleteFileDataFn>{buf, delete_fn},
        .size = std::size_t(size),
    };
}

std::optional<int> parse_duration(std::string_view s)
{
    if (s.size() == 0)
        return std::nullopt;
    std::array<int, 4> numbers = { 0, 0, 0, 0 };
    std::array<int, 4> multipliers = { 60 * 60 * 1000, 60 * 1000, 1000, 1 };
    bool decimal = false;
    auto parsenum = [&] (auto start, int index) {
        auto i = start;
        while (string::is_digit(s[i]))
            i--;
        numbers[index--] = string::to_number(s.substr(i+1, start - i)).value();
        return i;
    };
    int i = parsenum(s.size() - 1, 3);
    if (s[i] == '.' || s[i] == ',') {
        i = parsenum(i-1, 2);
        decimal = true;
    }
    if (decimal && s[i] == ':')
        i = parsenum(i-1, 1);
    if (decimal && s[i] == ':')
        i = parsenum(i-1, 0);
    if (i > 0)
        return std::nullopt;
    int n = 0;
    for (int i = 0; i < 4; i++)
        n += numbers[i] * multipliers[i];
    return n;
}

constexpr long samples_to_millis(long samples, int sample_rate, int channels)
{
    auto rate = sample_rate * channels;
    auto secs = samples / rate;
    auto frac = samples - secs * rate; // samples % rate;
    return secs * 1000 + frac * 1000 / rate;
}

constexpr long millis_to_samples(long millis, int sample_rate, int channels)
{
    auto secs = millis / 1000;
    auto frac = millis - secs * 1000; // millis % 1000;
    return (secs * sample_rate + frac * sample_rate / 1000) * channels;
}



/* gsf parsing */

constexpr int MAX_LIBS = 11;

struct Rom {
    u32 entry_point;
    u32 offset;
    Vector<u8> data;
};

struct GSFFile {
    // std::span<u8> reserved;
    Rom rom;
    TagMap tags;

    void impose(const GSFFile &f)
    {
        std::copy(f.rom.data.begin(), f.rom.data.end(),
                    rom.data.begin() + (f.rom.offset & 0x01FFFFFF));
    }
};

Result<Rom> uncompress_rom(std::span<u8> data, u32 crc)
{
    if (crc != crc32(crc32(0l, nullptr, 0), data.data(), data.size()))
        return tl::unexpected(0);
    // uncompress first 12 bytes first, which tells us the entry point,
    // the offset and the size of the rom
    std::array<u8, 12> tmp;
    unsigned long size = 12;
    if (uncompress(tmp.data(), &size, data.data(), data.size()) != Z_BUF_ERROR)
        return tl::unexpected(0);
    // re-uncompress, now with the real size;
    size = read4(&tmp[8]) + 12;
    auto uncompressed = Vector<u8>(size, 0);
    if (uncompress(uncompressed.data(), &size, data.data(), data.size()) != Z_OK)
        return tl::unexpected(0);
    uncompressed.erase(uncompressed.begin(), uncompressed.begin() + 12);
    return Rom {
        .entry_point = read4(&tmp[0]),
        .offset      = read4(&tmp[4]),
        .data        = std::move(uncompressed)
    };
}

TagMap parse_tags(std::string_view tags)
{
    TagMap result;
    string::split(tags, '\n', [&] (std::string_view tag) {
        auto equals = tag.find('=');
        auto first = string::trim_view(tag.substr(0, equals));
        auto second = string::trim_view(tag.substr(equals + 1, tag.size()));
        // currently lacking multiline variables
        result[String(first)] = String(second);
    });
    return result;
}

Result<GSFFile> parse(std::span<u8> data)
{
    if (data.size() < 0x10 || data.size() > 0x4000000)
        return tl::unexpected(0);
    if (data[0] != 'P' || data[1] != 'S' || data[2] != 'F' || data[3] != 0x22)
        return tl::unexpected(0);
    size_t cursor = 4;
    auto readb = [&](size_t bytes) { auto p = &data[cursor]; cursor += bytes; return std::span{p, bytes}; };
    u32 reserved_length = read4(readb(4));
    u32 program_length  = read4(readb(4));
    u32 crc             = read4(readb(4));
    if (reserved_length + program_length + 16 > data.size())
        return tl::unexpected(0);
    // auto reserved = readb(reserved_length);
    readb(reserved_length);
    auto rom = program_length > 0 ? uncompress_rom(readb(program_length), crc) : Rom{};
    if (!rom)
        return tl::unexpected(rom.error());
    auto tags = cursor < data.size() - 5 && std::memcmp(readb(5).data(), "[TAG]", 5) == 0
              ? readb(std::min<size_t>(data.size() - cursor, 50000u))
              : std::span<u8>{};
    return GSFFile {
        // .reserved = reserved,
        .rom = std::move(rom.value()),
        .tags = parse_tags(std::string_view((char *) tags.data(), tags.size())),
    };
}

std::optional<String> find_lib(std::span<GSFFile> files, int n)
{
    auto key = String("_lib") + string::from_number<String>(n);
    for (auto &f : files)
        if (auto p = f.tags.find(key); p != f.tags.end())
            return p->second;
    return std::nullopt;
}

Result<GSFFile> load_file(fs::path filepath,
    void *userdata, GsfReadFn read_fn, GsfDeleteFileDataFn delete_fn)
{
    auto parsebuf = [](ManagedBuffer<u8, GsfDeleteFileDataFn> buf) { return parse(buf.to_span()); };
    std::array<GSFFile, MAX_LIBS> files;
    if (auto r = read_file(filepath, userdata, read_fn, delete_fn)
                .and_then(parsebuf)
                .map([&] (GSFFile &&f) { files[0] = std::move(f); }); !r)
        return tl::unexpected(r.error());
    if (auto tag = files[0].tags.find("_lib"); tag != files[0].tags.end()) {
        if (auto r = read_file(filepath.parent_path() / tag->second, userdata, read_fn, delete_fn)
                    .and_then(parsebuf)
                    .map([&] (GSFFile &&f) {
                        files[1] = std::move(f);
                        files[1].impose(files[0]);
                        std::swap(files[1].rom.data, files[0].rom.data);
                    }); !r)
            return tl::unexpected(r.error());
        for (auto i = 2; i < MAX_LIBS; i++) {
            if (auto libname = find_lib(std::span{files.begin(), files.begin() + i}, i); libname) {
                if (auto r = read_file(filepath.parent_path() / libname.value(), userdata, read_fn, delete_fn)
                            .and_then(parsebuf)
                            .map([&] (GSFFile &&f) {
                                files[i] = std::move(f);
                                files[0].impose(files[i]);
                            }); !r)
                    return tl::unexpected(r.error());
            }
        }
    }
    return files[0];
}



// actual implementation of emulator and various other stuff
// using mGBA as a base

constexpr auto NUM_SAMPLES = 2048;
constexpr auto NUM_CHANNELS = 2;
constexpr auto BUF_SIZE = NUM_CHANNELS * NUM_SAMPLES;

void post_audio_buffer(mAVStream *stream, blip_t *left, blip_t *right);

class AVStream : public mAVStream {
    GSF_IMPLEMENT_ALLOCATORS

public:
    short samples[BUF_SIZE];
    long read = 0;

    AVStream() : mAVStream()
    {
        this->postAudioBuffer = post_audio_buffer;
    }

    void take(std::span<short> out)
    {
        auto index = BUF_SIZE - read;
        std::copy_n(std::begin(samples) + index, out.size(), out.begin());
        read -= out.size();
    }

    void clear(long n) { read -= n; }
};

void post_audio_buffer(mAVStream *stream, blip_t *left, blip_t *right)
{
    auto *self = (AVStream *) stream;
    /*auto samples_read1 =*/ blip_read_samples(left,  self->samples,   NUM_SAMPLES, true);
    /*auto samples_read2 =*/ blip_read_samples(right, self->samples+1, NUM_SAMPLES, true);
    self->read += BUF_SIZE;
}

class GsfEmu {
    GSF_IMPLEMENT_ALLOCATORS

    mCore *core;
    AVStream av;
    int samplerate;
    int flags;
    TagMap tags;
    long num_samples = 0;
    long max_samples = 0;
    int default_len  = 0;
    bool loaded      = false;
    bool infinite    = false;

public:
    explicit GsfEmu(mCore *core, int sample_rate, int flags)
        : core{core}, samplerate{sample_rate}, flags{flags} { }

    static Result<GsfEmu *> create(int sample_rate, int flags)
    {
        auto core = GBACoreCreate();
        core->init(core);
        mCoreInitConfig(core, nullptr);
        core->setAudioBufferSize(core, NUM_SAMPLES);
        auto clock_rate = core->frequency(core);
        for (auto i = 0; i < NUM_CHANNELS; i++)
            blip_set_rates(core->getAudioChannel(core, i), clock_rate, sample_rate);
        mCoreOptions opts = {};
        opts.skipBios = true;
        opts.useBios = false;
        opts.sampleRate = static_cast<unsigned>(sample_rate);
        mCoreConfigLoadDefaults(&core->config, &opts);
        auto *emu = new GsfEmu(core, sample_rate, flags);
        core->setAVStream(core, &emu->av);
        return emu;
    }

    ~GsfEmu()
    {
        core->deinit(core);
        core = nullptr;
    }

    int load(std::span<u8> data, TagMap &&tags)
    {
        auto *vmem = VFileMemChunk(data.data(), data.size());
        core->loadROM(core, vmem);
        core->reset(core);
        this->tags = std::move(tags);
        auto length = parse_duration(get_tag("length")).value_or(default_len);
        max_samples = millis_to_samples(length, samplerate, num_channels());
        loaded = true;
        return 0;
    }

    void play(short *out, long size)
    {
        memset(out, 0, size * sizeof(short));
        for (auto took = 0; took < size && !ended(); ) {
            while (av.read == 0)
                core->runLoop(core);
            auto to_take = std::min(size - took, av.read);
            av.take(std::span{out + took, static_cast<size_t>(to_take)});
            took += to_take;
            num_samples += to_take;
        }
    }

    void skip(long n)
    {
        for (auto took = 0; took < n && !ended(); ) {
            while (av.read == 0)
                core->runLoop(core);
            auto to_take = std::min(n - took, av.read);
            av.clear(to_take);
            took += to_take;
            num_samples += to_take;
        }
    }

    std::string_view get_tag(const String &s) const
    {
        if (auto it = tags.find(s); it != tags.end())
            return it->second;
        return "";
    }

    void set_default_length(long length)
    {
        default_len = length;
        if (loaded && max_samples == 0) {
            max_samples = millis_to_samples(default_len, samplerate, num_channels());
        }
    }

    void set_infinite(bool value) { infinite = value; }

    long tell()           const { return num_samples; }
    int sample_rate()     const { return samplerate; }
    long length()         const { return samples_to_millis(max_samples, samplerate, num_channels()); }
    long default_length() const { return default_len; }
    bool ended()          const { return infinite ? false : num_samples >= max_samples; }
    bool loaded_file()    const { return loaded; }
    int num_channels()    const { return NUM_CHANNELS; }
};



// mGBA by default spits out a lot of log stuff. This and the call to
// mLogSetDefaultLogger() in gsf_new makes sure to disable all that.
void mgba_empty_log(struct mLogger *, int, enum mLogLevel, const char *, va_list) { }

struct EmptyLogger : mLogger {
    EmptyLogger() { this->log = mgba_empty_log; }
} empty_logger;



/* public API functions */

GSF_API unsigned int gsf_get_version(void)
{
    return GSF_VERSION;
}

GSF_API bool gsf_is_compatible_dll(void)
{
    unsigned major = gsf_get_version() >> 16;
    return major == GSF_VERSION_MAJOR;
}

GSF_API int gsf_new(GsfEmu **out, int sample_rate, int flags)
{
    auto emu = GsfEmu::create(sample_rate, flags);
    if (!emu)
        return emu.error();
    *out = emu.value();
    mLogSetDefaultLogger(&empty_logger);
    return 0;
}

GSF_API void gsf_delete(GsfEmu *emu)
{
    delete emu;
}

GSF_API int gsf_load_file_custom(GsfEmu *emu, const char *filename,
    void *userdata, GsfReadFn read_fn, GsfDeleteFileDataFn delete_fn)
{
    auto f = load_file(fs::path{filename}, userdata, read_fn, delete_fn);
    if (!f)
        return f.error();
    emu->load(f.value().rom.data, std::move(f.value().tags));
    return 0;
}

GSF_API int gsf_load_file(GsfEmu *emu, const char *filename)
{
    return gsf_load_file_custom(emu, filename, nullptr, default_read_file,
        default_delete_file_data);
}

GSF_API bool gsf_loaded(const GsfEmu *emu)
{
    return emu->loaded_file();
}

GSF_API void gsf_play(GsfEmu *emu, short *out, long size)
{
    emu->play(out, size);
}

GSF_API bool gsf_track_ended(const GsfEmu *emu)
{
    return emu->ended();
}

GSF_API int gsf_get_tags(const GsfEmu *emu, GsfTags **out)
{
    GsfTags *tags   = gsf_allocate<GsfTags>();
    tags->title     = emu->get_tag("title").data();
    tags->artist    = emu->get_tag("artist").data();
    tags->game      = emu->get_tag("game").data();
    tags->year      = string::to_number(emu->get_tag("year")).value_or(0);
    tags->genre     = emu->get_tag("genre").data();
    tags->comment   = emu->get_tag("comment").data();
    tags->copyright = emu->get_tag("copyright").data();
    tags->gsfby     = emu->get_tag("gsfby").data();
    tags->volume    = string::to_number<double>(emu->get_tag("volume")).value_or(0.0);
    tags->fade      = parse_duration(emu->get_tag("fade")).value_or(0);
    *out = tags;
    return 0;
}

GSF_API void gsf_free_tags(GsfTags *tags)
{
    allocators.free(tags, allocators.userdata);
}

GSF_API long gsf_length(GsfEmu *emu)
{
    return samples_to_millis(emu->length(), emu->sample_rate(), emu->num_channels());
}

GSF_API long gsf_tell(const GsfEmu *emu)
{
    return samples_to_millis(emu->tell(), emu->sample_rate(), 2);
}

GSF_API long gsf_tell_samples(const GsfEmu *emu)
{
    return emu->tell();
}

GSF_API void gsf_seek(GsfEmu *emu, long millis)
{
    gsf_seek_samples(emu, millis_to_samples(millis, emu->sample_rate(), 2));
}

GSF_API void gsf_seek_samples(GsfEmu *emu, long samples)
{
    emu->skip(samples);
}

GSF_API void gsf_set_default_length(GsfEmu *emu, long length)
{
    emu->set_default_length(length);
}

GSF_API long gsf_default_length(const GsfEmu *emu)
{
    return emu->default_length();
}

GSF_API void gsf_set_infinite(GsfEmu *emu, bool infinite)
{
    emu->set_infinite(infinite);
}

GSF_API void gsf_set_allocators(GsfAllocators *allocators)
{
    ::allocators = *allocators;
}
