#pragma once

#include "gsf.h"

#include <limits>

extern GsfAllocators allocators;

namespace detail {

void *malloc(size_t size, void *) { return ::malloc(size); }
void free(void *p, size_t, void *) { ::free(p); }

} // namespace detail

template <typename T, typename... Args>
inline T *allocate(const GsfAllocators &allocators, size_t nmemb, Args&&... args)
{
    auto *mem = allocators.malloc(nmemb * sizeof(T), allocators.userdata);
    if (!mem)
        return nullptr;
    auto *obj = new (mem) T(args...);
    return obj;
}

template <typename T>
struct GsfAllocator {
    GsfAllocators allocators = { nullptr, nullptr, nullptr };

    using value_type      = T;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

    constexpr GsfAllocator() noexcept = default;
    constexpr explicit GsfAllocator(const GsfAllocators &allocators) noexcept
        : allocators{allocators}
    { }

    constexpr GsfAllocator(const GsfAllocator &a) noexcept
    {
        this->allocators.malloc   = a.allocators.malloc;
        this->allocators.free     = a.allocators.free;
        this->allocators.userdata = a.allocators.userdata;
    }

    template <class U>
    constexpr GsfAllocator(const GsfAllocator<U> &a) noexcept
    {
        this->allocators.malloc   = a.allocators.malloc;
        this->allocators.free     = a.allocators.free;
        this->allocators.userdata = a.allocators.userdata;
    }

    constexpr ~GsfAllocator() {}

    T *allocate(size_type count)
    {
        return static_cast<T *>(allocators.malloc(count * sizeof(T), allocators.userdata));
    }

    void deallocate(T *p, size_type size)
    {
        allocators.free(p, size, allocators.userdata);
    }

    bool operator==(const GsfAllocator<T> &other) const
    {
        return allocators.malloc   == other.allocators.malloc
            && allocators.free     == other.allocators.free
            && allocators.userdata == other.allocators.userdata;
    }

    bool operator!=(const GsfAllocator<T> &other) const  { return !operator==(other); }
};
