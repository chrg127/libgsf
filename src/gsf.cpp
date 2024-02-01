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
#include <zlib.h>
#include <tl/expected.hpp>
#include <mgba/gba/core.h>
#include <mgba/core/core.h>
#include <mgba/core/blip_buf.h>
#include <mgba/core/log.h>
#include <mgba-util/vfs.h>
#include "allocation.hpp"
#include "string.hpp"



/* utilities */

namespace fs = std::filesystem;

using u8 = unsigned char;
using u32 = uint32_t;
template <typename T> using Vector = std::vector<T, GsfAllocator<T>>;
using String = std::basic_string<char, std::char_traits<char>, GsfAllocator<char>>;
template <typename T> using Result = tl::expected<T, GsfError>;

struct InsensitiveCompare {
    bool operator()(const String &lhs, const String &rhs) const
    {
        return string::iequals(lhs, rhs);
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

GsfError make_err(GsfErrorCode code) { return { .code = code, .from = 1 }; }



/* reading files */

template <typename T, typename Deleter = std::default_delete<T>>
struct ManagedBuffer {
    std::unique_ptr<T[], Deleter> ptr;
    std::size_t size;
    std::span<      T> to_span()       { return std::span<T>(ptr.get(), size); }
    std::span<const T> to_span() const { return std::span<T>(ptr.get(), size); }
};

struct Deleter {
    GsfReader reader;
    const GsfAllocators allocators;
    long size;
    void operator()(unsigned char *buf) { reader.delete_data(buf, size, reader.userdata, &allocators); }
};

GsfReadResult default_read_file(const char *filename, void *, const GsfAllocators *allocators)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
        return { .buf = nullptr, .size = 0, .err = { .code = 1, .from = 0 } };
    fseek(file, 0l, SEEK_END);
    auto size = ftell(file);
    rewind(file);
    unsigned char *buf = allocate<unsigned char>(*allocators, size);
    if (!buf)
        return { .buf = nullptr, .size = 0, .err = make_err(GSF_ALLOCATION_FAILED) };
    size_t bytes_read = fread(buf, sizeof(char), size, file);
    if (bytes_read < (size_t)size) {
        allocators->free(buf, size, allocators->userdata);
        return { .buf = nullptr, .size = 0, .err = { .code = 1, .from = 0 } };
    }
    fclose(file);
    return { .buf = buf, .size = size, .err = { .code = 0, .from = 0 } };
}

void default_delete_data(unsigned char *buf, long size, void *, const GsfAllocators *allocators)
{
    allocators->free(buf, size, allocators->userdata);
}

Result<ManagedBuffer<u8, Deleter>> read_file(fs::path filepath,
    const GsfReader &reader, const GsfAllocators &allocators)
{
    auto res = reader.read(filepath.string().c_str(), reader.userdata, &allocators);
    if (res.err.code != 0)
        return tl::unexpected(res.err);
    auto deleter = Deleter { .reader = reader, .allocators = allocators, .size = res.size };
    return ManagedBuffer {
        .ptr = std::unique_ptr<u8[], Deleter>{res.buf, deleter},
        .size = std::size_t(res.size),
    };
}



/* gsf parsing */

constexpr int MAX_LIBS = 11;

struct Rom {
    u32 entry_point;
    u32 offset;
    Vector<u8> data;

    explicit Rom(const GsfAllocators &allocators)
        : entry_point{0}, offset{0}, data(GsfAllocator<u8>(allocators))
    { }
    Rom(u32 entry_point, u32 offset, Vector<u8> data)
        : entry_point{entry_point}, offset{offset}, data{std::move(data)}
    { }
};

struct GSFFile {
    // std::span<u8> reserved;
    Rom rom;
    TagMap tags;

    GSFFile(const GsfAllocators &allocators)
        : rom{allocators}, tags(GsfAllocator<std::pair<const String, String>>(allocators))
    { }
    GSFFile(Rom &&rom, TagMap &&tags)
        : rom{std::move(rom)}, tags{std::move(tags)}
    { }

    void impose(const GSFFile &f)
    {
        std::copy(f.rom.data.begin(), f.rom.data.end(),
                    rom.data.begin() + (f.rom.offset & 0x01FFFFFF));
    }
};

Result<Rom> uncompress_rom(std::span<u8> data, u32 crc, const GsfAllocators &allocators)
{
    if (crc != crc32(crc32(0l, nullptr, 0), data.data(), data.size()))
        return tl::unexpected(make_err(GSF_INVALID_CRC));
    // uncompress first 12 bytes first, which tells us the entry point,
    // the offset and the size of the rom
    std::array<u8, 12> tmp;
    unsigned long size = 12;
    if (uncompress(tmp.data(), &size, data.data(), data.size()) != Z_BUF_ERROR)
        return tl::unexpected(make_err(GSF_UNCOMPRESS_ERROR));
    // re-uncompress, now with the real size;
    size = read4(&tmp[8]) + 12;
    auto uncompressed = Vector<u8>(size, 0, GsfAllocator<u8>(allocators));
    if (uncompress(uncompressed.data(), &size, data.data(), data.size()) != Z_OK)
        return tl::unexpected(make_err(GSF_UNCOMPRESS_ERROR));
    uncompressed.erase(uncompressed.begin(), uncompressed.begin() + 12);
    return Rom {
        read4(&tmp[0]),
        read4(&tmp[4]),
        std::move(uncompressed)
    };
}

TagMap parse_tags(std::string_view tags, const GsfAllocators &allocators)
{
    auto allocator = GsfAllocator<char>(allocators);
    auto result = TagMap(GsfAllocator<std::pair<const String, String>>(allocators));
    string::split(tags, '\n', [&] (std::string_view tag) {
        auto equals = tag.find('=');
        auto first = string::trim_view(tag.substr(0, equals));
        auto second = string::trim_view(tag.substr(equals + 1, tag.size()));
        // currently lacking multiline variables
        result[String(first, allocator)] = String(second, allocator);
    });
    return result;
}

Result<GSFFile> parse(std::span<u8> data, const GsfAllocators &allocators)
{
    if (data.size() < 0x10 || data.size() > 0x4000000)
        return tl::unexpected(make_err(GSF_INVALID_FILE_SIZE));
    if (data[0] != 'P' || data[1] != 'S' || data[2] != 'F' || data[3] != 0x22)
        return tl::unexpected(make_err(GSF_INVALID_HEADER));
    size_t cursor = 4;
    auto readb = [&](size_t bytes) { auto p = &data[cursor]; cursor += bytes; return std::span{p, bytes}; };
    u32 reserved_length = read4(readb(4));
    u32 program_length  = read4(readb(4));
    u32 crc             = read4(readb(4));
    if (reserved_length + program_length + 16 > data.size())
        return tl::unexpected(make_err(GSF_INVALID_SECTION_LENGTH));
    // auto reserved = readb(reserved_length);
    readb(reserved_length);
    auto rom = program_length > 0 ? uncompress_rom(readb(program_length), crc, allocators) : Rom{allocators};
    if (!rom)
        return tl::unexpected(rom.error());
    auto tags = cursor < data.size() - 5 && std::memcmp(readb(5).data(), "[TAG]", 5) == 0
              ? readb(std::min<size_t>(data.size() - cursor, 50000u))
              : std::span<u8>{};
    return GSFFile {
        // .reserved = reserved,
        std::move(rom.value()),
        parse_tags(std::string_view((char *) tags.data(), tags.size()), allocators),
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

Result<GSFFile> load_file(fs::path filepath, const GsfReader &reader, const GsfAllocators &allocators)
{
    auto parsebuf = [&](ManagedBuffer<u8, Deleter> buf) { return parse(buf.to_span(), allocators); };
    auto files = Vector<GSFFile>(MAX_LIBS, GSFFile{allocators}, GsfAllocator<GSFFile>(allocators));
    if (auto r = read_file(filepath, reader, allocators)
                .and_then(parsebuf)
                .map([&] (GSFFile &&f) { files[0] = std::move(f); }); !r)
        return tl::unexpected(r.error());
    if (auto tag = files[0].tags.find("_lib"); tag != files[0].tags.end()) {
        if (auto r = read_file(filepath.parent_path() / tag->second, reader, allocators)
                    .and_then(parsebuf)
                    .map([&] (GSFFile &&f) {
                        files[1] = std::move(f);
                        files[1].impose(files[0]);
                        std::swap(files[1].rom.data, files[0].rom.data);
                    }); !r)
            return tl::unexpected(r.error());
        for (auto i = 2; i < MAX_LIBS; i++) {
            if (auto libname = find_lib(std::span{files.begin(), files.begin() + i}, i); libname) {
                if (auto r = read_file(filepath.parent_path() / libname.value(), reader, allocators)
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

struct AVStream : public mAVStream {
    short samples[BUF_SIZE] = {};
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
    mCore *core;
    int samplerate;
    int flags;
    TagMap tags;
    AVStream av;
    long num_samples = 0;
    long max_samples = 0;
    int default_len  = 0;
    bool loaded      = false;
    bool infinite    = false;

public:
    explicit GsfEmu(mCore *core, int sample_rate, int flags, const GsfAllocators &allocators)
        : core{core}, samplerate{sample_rate}, flags{flags},
          tags(GsfAllocator<std::pair<const String, String>>(allocators))
    { }

    static Result<GsfEmu *> create(int sample_rate, int flags, const GsfAllocators &allocators)
    {
        auto core = GBACoreCreate();
        if (!core->init(core))
            return tl::unexpected(make_err(GSF_ALLOCATION_FAILED));
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
        auto *emu = allocate<GsfEmu>(allocators, 1, core, sample_rate, flags, allocators);
        if (!emu) {
            core->deinit(core);
            return tl::unexpected(make_err(GSF_ALLOCATION_FAILED));
        }
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
            av.take(std::span<short>{ out + took, static_cast<size_t>(to_take) });
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
    long length_samples() const { return max_samples; }
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

GSF_API bool gsf_is_compatible_version(void)
{
    unsigned major = gsf_get_version() >> 16;
    return major == GSF_VERSION_MAJOR;
}

GSF_API GsfError gsf_new(GsfEmu **out, int sample_rate, int flags)
{
    auto alloc = GsfAllocators { detail::malloc, detail::free, nullptr };
    return gsf_new_with_allocators(out, sample_rate, flags, &alloc);
}

GSF_API GsfError gsf_new_with_allocators(GsfEmu **out, int sample_rate, int flags, GsfAllocators *allocators)
{
    auto emu = GsfEmu::create(sample_rate, flags, *allocators);
    if (!emu)
        return emu.error();
    *out = emu.value();
    mLogSetDefaultLogger(&empty_logger);
    return GSF_NO_ERROR;
}

GSF_API void gsf_delete(GsfEmu *emu)
{
    auto alloc = GsfAllocators { detail::malloc, detail::free, nullptr };
    gsf_delete_with_allocators(emu, &alloc);
}

GSF_API void gsf_delete_with_allocators(GsfEmu *emu, GsfAllocators *allocators)
{
    allocators->free(emu, sizeof(GsfEmu), allocators->userdata);
}

GSF_API GsfError gsf_load_file(GsfEmu *emu, const char *filename)
{
    auto reader = GsfReader { default_read_file, default_delete_data, nullptr };
    return gsf_load_file_with_reader(emu, filename, &reader);
}

GSF_API GsfError gsf_load_file_with_reader(GsfEmu *emu, const char *filename,
    GsfReader *reader)
{
    auto alloc = GsfAllocators { detail::malloc, detail::free, nullptr };
    return gsf_load_file_with_reader_allocators(emu, filename, reader, &alloc);
}

GSF_API GsfError gsf_load_file_with_allocators(GsfEmu *emu,
    const char *filename, GsfAllocators *allocators)
{
    auto reader = GsfReader { default_read_file, default_delete_data, nullptr };
    return gsf_load_file_with_reader_allocators(emu, filename, &reader, allocators);
}

GSF_API GsfError gsf_load_file_with_reader_allocators(GsfEmu *emu,
    const char *filename, GsfReader *reader, GsfAllocators *allocators)
{
    auto f = load_file(fs::path{filename}, *reader, *allocators);
    if (!f)
        return f.error();
    emu->load(f.value().rom.data, std::move(f.value().tags));
    return GSF_NO_ERROR;
}

GSF_API bool gsf_loaded(const GsfEmu *emu)
{
    return emu->loaded_file();
}

GSF_API void gsf_play(GsfEmu *emu, short *out, long size)
{
    emu->play(out, size);
}

GSF_API bool gsf_ended(const GsfEmu *emu)
{
    return emu->ended();
}

GSF_API GsfError gsf_get_tags(const GsfEmu *emu, GsfTags **out)
{
    auto alloc = GsfAllocators { detail::malloc, detail::free, nullptr };
    return gsf_get_tags_with_allocators(emu, out, &alloc);
}

GSF_API void gsf_free_tags(GsfTags *tags)
{
    auto alloc = GsfAllocators { detail::malloc, detail::free, nullptr };
    return gsf_free_tags_with_allocators(tags, &alloc);
}

GSF_API GsfError gsf_get_tags_with_allocators(const GsfEmu *emu, GsfTags **out,
    GsfAllocators *allocators)
{
    auto *tags      = allocate<GsfTags>(*allocators, 1);
    if (!tags)
        return make_err(GSF_ALLOCATION_FAILED);
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
    return GSF_NO_ERROR;
}

GSF_API void gsf_free_tags_with_allocators(GsfTags *tags, GsfAllocators *allocators)
{
    allocators->free(tags, sizeof(GsfTags), allocators->userdata);
}

GSF_API long gsf_length(GsfEmu *emu)
{
    return samples_to_millis(emu->length_samples(), emu->sample_rate(), emu->num_channels());
}

GSF_API long gsf_length_samples(GsfEmu *emu)
{
    return emu->length_samples();
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

GSF_API int gsf_sample_rate(GsfEmu *emu)
{
    return emu->sample_rate();
}

GSF_API int gsf_num_channels(GsfEmu *emu)
{
    return emu->num_channels();
}
