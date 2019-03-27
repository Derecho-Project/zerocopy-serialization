#pragma once

#include "slab.h"

/**
 * Compute ceil(log_2(n))
 */
constexpr int log2_int_ceil(int n) {
  int rounded_size = 1;
  int exponent = 0;
  while (rounded_size < n) {
    rounded_size <<= 1;
    ++exponent;
  }
  return exponent;
}

struct SlabAllocatorInternal {
  // Allows for slabs with slot size up to 2 << MAX_SLABS bytes
  static int const MAX_SLABS = 40;

  // slabs[i] can be cast to (Slab< (2<<i) >*)
  void* slabs[MAX_SLABS] = {0};

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
  value_type* allocate(std::size_t n)
  {
    constexpr int exponent = log2_int_ceil(n * sizeof(value_type));

    if (exponent >= internal->MAX_SLABS) {
      throw std::runtime_error("Tried to allocate an object that was too large");
    }

    // Create a new SingleAllocator the first time this particular
    // rounded_size is needed.
    // TODO: Replace lock with atomic bool
    if (internal->slabs[exponent] == nullptr) {
      std::lock_guard<std::mutex> lock(internal->mux_slabs);
      if (internal->slabs[exponent] == nullptr) {
        internal->slabs[exponent] = new Slab<exponent>();
      }
    }

    // Find the correct allocator for this size and use it do allocation
    Slab<exponent>* slab = internal->slabs[exponent];
    auto [p, unused1, unused2] = slab->allocate();

    return static_cast<value_type*>(p);
  }

  void deallocate(value_type* p, std::size_t n) noexcept
  {
    constexpr int exponent = log2_int_ceil(n * sizeof(value_type));

    if (exponent >= internal->MAX_SLABS) {
      throw std::runtime_error("Tried to deallocate an object that coudn't have been allocated because it was too large");
    } else {
      // Find the correct allocator for this size and use it do allocation
      Slab<exponent>* slab = internal->slabs[exponent];
      slab->deallocate(p);
    }
  }
};
