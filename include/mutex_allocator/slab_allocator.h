#pragma once

#include "single_allocator.h"
#include <memory>
#include <vector>
#include <thread>

#include <cmath>
#include <cstring>

template <typename T>
struct SlabAllocator {
    typedef T value_type;

    // Allows for slots of size 2 << MAX_ALLOCATORS
    static int const MAX_ALLOCATORS = 13;
    SingleAllocator* allocators[MAX_ALLOCATORS] = {0};
    std::mutex* mux_allocators;

    // used for debugging
    void viewState() {
        for (auto& salloc : allocators) {
            if (salloc != nullptr) {
                salloc->viewState();
            }
        }
    }

    SlabAllocator()
    {
        //std::cout << "Calling SlabAllocator() for " << (void*) this 
        //    << ": " << __PRETTY_FUNCTION__ << std::endl;
        mux_allocators = new std::mutex();
    }

    ~SlabAllocator()
    {
        //std::cout << "Calling ~SlabAllocator() on " << (void*) this
        //    << ": " << __PRETTY_FUNCTION__ << std::endl;
        //for (int i = 0; i < MAX_ALLOCATORS; ++i) {
        //    if (allocators[i] != nullptr)
        //        delete allocators[i];
        //}
        //delete mux_allocators;
    }

    template <class U>
    SlabAllocator(const SlabAllocator<U>& rhs)
    {
        //throw std::runtime_error(__PRETTY_FUNCTION__);
        //std::cout << "Calling SlabAllocator(const SlabAllocator<U>& rhs)" << std::endl;
        mux_allocators = rhs.mux_allocators;
        memcpy(allocators, rhs.allocators, sizeof(SingleAllocator*) * MAX_ALLOCATORS);
    }

    template <class U>
    SlabAllocator<T>& operator=(const SlabAllocator<U>& rhs)
    {
        //throw std::runtime_error(__PRETTY_FUNCTION__);
        //std::cout << "Calling SlabAllocator(const SlabAllocator<U>& rhs)" << std::endl;
        mux_allocators = rhs.mux_allocators;
        memcpy(allocators, rhs.allocators, sizeof(SingleAllocator*) * MAX_ALLOCATORS);
    }

    T* allocate(std::size_t n)
    {
        //std::cout << "Trying to allocate " << n * sizeof(T) << " bytes" << std::endl;

        // Find the nearest power of 2 >= (n*sizeof(T))
        size_t rounded_size = 1;
        int exponent = 0;
        while (rounded_size < n * sizeof(T)) {
            rounded_size <<= 1;
            ++exponent;
        }

        //std::cout << "Rounded up to " << rounded_size << " bytes" << std::endl;

        // If size of object to be allocated is larger than what we have
        // SingleAllocators for, just malloc it
        if (exponent >= MAX_ALLOCATORS) {
            return static_cast<T*>(malloc(n * sizeof(T)));
        }

        // Create a new SingleAllocator the first time this particular
        // rounded_size is needed.
        if (allocators[exponent] == nullptr) {
            std::lock_guard<std::mutex> lock(*mux_allocators);
            if (allocators[exponent] == nullptr) {
                allocators[exponent] = new SingleAllocator(rounded_size);
            }
        }

        SingleAllocator *salloc = allocators[exponent];
        T* p = static_cast<T*>(salloc->allocate());

        return p;
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        // Find the nearest power of 2 >= (n*sizeof(T))
        size_t rounded_size = 1;
        int exponent = 0;
        while (rounded_size < n * sizeof(T)) {
            rounded_size <<= 1;
            ++exponent;
        }

        if (exponent >= MAX_ALLOCATORS) {
            free(p);
        } else {
            // Find the correct allocator for this size and use it do allocation
            SingleAllocator*& salloc = allocators[exponent];
            salloc->deallocate(p);
        }
    }

};

template <class T, class U>
bool operator==(const SlabAllocator<T>& lhs, const SlabAllocator<U>& rhs)
{
    return true;
}

template <class T, class U>
bool operator!=(const SlabAllocator<T>& lhs, const SlabAllocator<U>& rhs)
{
    return false;
}

