#ifndef SLAB_ALLOCATOR_H
#define SLAB_ALLOCATOR_H

#include "single_allocator.h"
#include <memory>
#include <vector>
#include <cmath>
#include <thread>

/**
 * Internal state for a slab allocator.
 * This is what will need to be copied when a slab allocator itself is copied.
 */
struct slab_allocator_internal {
    // Allows for slots of size 2 << MAX_ALLOCATORS
    static int const MAX_ALLOCATORS = 40;
    SingleAllocator* allocators[MAX_ALLOCATORS] = {0};
    std::mutex mux_allocators;

    ~slab_allocator_internal() {
        for (int i = 0; i < MAX_ALLOCATORS; ++i) {
            delete allocators[i];
        }
    }
};

/**
 * Compute ceil(log_2(n))
 */
int log2_int_ceil(int n) {
    int rounded_size = 1;
    int exponent = 0;
    while (rounded_size < n) {
        rounded_size <<= 1;
        ++exponent;
    }
    return exponent;
}

template <typename T>
struct SlabAllocator {
    typedef T value_type;

    using internal = slab_allocator_internal;
    std::shared_ptr<internal> internal_state;

    // used for debugging
    void viewState() {
        for (auto& salloc : internal_state->allocators) {
            if (salloc != nullptr) {
                salloc->viewState();
            }
        }
    }

    // Default constructor
    SlabAllocator() : internal_state(new internal())
    {
    }

    // Destructor
    ~SlabAllocator()
    {
    }

    // Override implicit default copy constructor
    constexpr SlabAllocator(const SlabAllocator& rhs) noexcept
    : internal_state(rhs.internal_state)
    {
    }

    // Template copy constructor
    template <class U>
    constexpr SlabAllocator(const SlabAllocator<U>& rhs) noexcept
    : internal_state(rhs.internal_state)
    {
    }

    [[nodiscard]]
    T* allocate(std::size_t n)
    {
        int exponent = log2_int_ceil(n * sizeof(T));

        // Throw error if desired size to allocate is larger than supported
        if (exponent >= internal_state->MAX_ALLOCATORS) {
            return (T*) malloc(n * sizeof(T));
        }

        // Create a new SingleAllocator the first time this particular
        // rounded_size is needed.
        // TODO: Replace lock with atomic bool
        if (internal_state->allocators[exponent] == nullptr) {
            std::lock_guard<std::mutex> lock(internal_state->mux_allocators);
            if (internal_state->allocators[exponent] == nullptr) {
                internal_state->allocators[exponent] = new SingleAllocator(2 << exponent);
            }
        }

        // Find the correct allocator for this size and use it do allocation
        SingleAllocator* salloc = internal_state->allocators[exponent];
        T* p = static_cast<T*>(salloc->allocate());

        return p;
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        int exponent = log2_int_ceil(n * sizeof(T));

        if (exponent >= internal_state->MAX_ALLOCATORS) {
            free(p);
        } else {
            // Find the correct allocator for this size and use it do allocation
            SingleAllocator*& salloc = internal_state->allocators[exponent];
            salloc->deallocate(p);
        }
    }
};

template <class T, class U>
bool operator==(const SlabAllocator<T>& lhs, const SlabAllocator<U>& rhs)
{
    return lhs.internal_state == rhs.internal_state;
}

template <class T, class U>
bool operator!=(const SlabAllocator<T>& lhs, const SlabAllocator<U>& rhs)
{
    return !(lhs == rhs);
}


#endif /* ifndef SLAB_ALLOCATOR_H */
