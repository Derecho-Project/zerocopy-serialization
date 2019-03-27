#pragma once

#include "slab.h"

// Computes 2^p
constexpr size_t pow2(size_t p) {
  return 2UL << p;
}

// Compute ceil(log_2(n))
constexpr size_t log2_int_ceil(size_t n) {
  size_t rounded_size = 1;
  size_t exponent = 0;
  while (rounded_size < n) {
    rounded_size <<= 1;
    ++exponent;
  }
  return exponent;
}

template<size_t s>
struct slab_array : slab_array<s-1> {
  using slab_array<s-1>::get;

  Slab<pow2(s)>* this_slab;

  Slab<pow2(s)>*& get(std::integral_constant<size_t, s>* = nullptr) {
    return this_slab;
  }
};

template<>
struct slab_array<0> {
  Slab<pow2(0)>* this_slab;

  Slab<pow2(0)>*& get(std::integral_constant<size_t, 0>* = nullptr) {
    return this_slab;
  }
};


struct SlabAllocatorInternal {
  // Allows for slabs with slot size up to 2 << MAX_SLABS bytes
  static int constexpr MAX_SLABS = 40;

  slab_array<MAX_SLABS-1> slabs;

  std::mutex mux_slabs;

  ~SlabAllocatorInternal() {
    // TODO: Delete all the slabs
  }
};


template <typename T>
struct SlabAllocator {
  using value_type = T;
  using internals = SlabAllocatorInternal;

  std::shared_ptr<internals> internal;

  // Constructor
  SlabAllocator() : internal(new internals())
  {}

  // Default Destructor
  ~SlabAllocator() = default;

  // Default Copy Constructor
  constexpr SlabAllocator(const SlabAllocator& rhs) noexcept
    : internal(rhs.internal)
  {}

  // Template Copy Constructor
  template <typename U>
  constexpr SlabAllocator(const SlabAllocator<U>& rhs) noexcept
    : internal(rhs.internal)
  {}

  [[nodiscard]]
  value_type* allocate(size_t n)
  {
    constexpr size_t exponent = log2_int_ceil(n * sizeof(value_type));
    std::integral_constant<size_t, exponent> idx;

    if (exponent >= internal->MAX_SLABS) {
      throw std::runtime_error("Tried to allocate an object that was too large");
    }

    // Create a new SingleAllocator the first time this particular
    // rounded_size is needed.
    // TODO: Replace lock with atomic bool
    if (internal->slabs.get(&idx) == nullptr) {
      std::lock_guard<std::mutex> lock(internal->mux_slabs);
      if (internal->slabs.get(&idx) == nullptr) {
        internal->slabs.get(&idx) = new Slab<exponent>();
      }
    }

    // Find the correct allocator for this size and use it do allocation
    Slab<exponent>* slab = internal->slabs.get(&idx);
    auto [p, unused1, unused2] = slab->allocate();

    return static_cast<value_type*>(p);
  }

  void deallocate(value_type* p, size_t n) noexcept
  {
    constexpr size_t exponent = log2_int_ceil(n * sizeof(value_type));
    std::integral_constant<size_t, exponent> idx;

    if (exponent >= internal->MAX_SLABS) {
      throw std::runtime_error("Tried to deallocate an object that coudn't have been allocated because it was too large");
    } else {
      // Find the correct allocator for this size and use it do allocation
      Slab<exponent>* slab = internal->slabs.get(&idx);
      slab->deallocate(p);
    }
  }
};
