#pragma once

#include "gsf.h"

#include <limits>

struct GsfAllocators {
    void *(*malloc_fn)(size_t);
    void *(*realloc_fn)(void *, size_t);
    void (*free_fn)(void *);
};

extern GsfAllocators allocators;

template <typename T>
inline T *gsf_allocate(size_t size = 1)
{
    return static_cast<T *>(allocators.malloc_fn(size * sizeof(T)));
}

inline void gsf_free(void *ptr) { allocators.free_fn(ptr); }

#define GSF_IMPLEMENTS_ALLOCATORS                                                   \
public:                                                                             \
    void *operator new  (size_t size)       { return allocators.malloc_fn(size); }  \
    void *operator new[](size_t size)       { return allocators.malloc_fn(size); }  \
    void *operator new  (size_t, void *ptr) { return ptr; }                         \
    void *operator new[](size_t, void *ptr) { return ptr; }                         \
    void  operator delete  (void *ptr)      { gsf_free(ptr); }                      \
    void  operator delete[](void *ptr)      { gsf_free(ptr); }                      \
    void  operator delete  (void *, void *) {}                                      \
    void  operator delete[](void *, void *) {}                                      \
private:

template <class T> struct GsfAllocator {
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef T& reference;
    typedef const T &const_reference;
    typedef T value_type;

    template <class U> struct rebind { typedef GsfAllocator<U> other; };

    GsfAllocator() throw() {}
    GsfAllocator(const GsfAllocator &) throw() {}
    template <class U> GsfAllocator(const GsfAllocator<U> &) throw() {}
    ~GsfAllocator() throw() {}
    pointer address(reference x) const { return &x; }
    const_pointer address(const_reference x) const { return &x; }
    pointer allocate(size_type s, void const * = 0) { return s ? gsf_allocate<T>(s) : 0; }
    void deallocate(pointer p, size_type) { gsf_free(p); }
    size_type max_size() const throw() { return std::numeric_limits<size_t>::max() / sizeof(T); }
    void construct(pointer p, const T& val) { new (reinterpret_cast<void *>(p)) T(val); }
    void destroy(pointer p) { p->~T(); }
    bool operator==(const GsfAllocator<T> &other) const { return true; }
    bool operator!=(const GsfAllocator<T> &other) const { return false; }
};
