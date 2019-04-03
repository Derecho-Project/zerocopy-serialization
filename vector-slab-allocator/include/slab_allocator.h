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

struct SlabAllocatorInternal {
  // Allows for slabs with slot size up to 2 << MAX_SLABS bytes
  static int constexpr MAX_SLABS = 40;

  std::array<Slab*, MAX_SLABS> slabs;

  std::mutex mux_slabs;

  ~SlabAllocatorInternal() {
    for (Slab* slab : slabs) {
      delete slab;
    }
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
    size_t exp = log2_int_ceil(n * sizeof(value_type));
    std::cout << "exponent = " << exp << std::endl;

    if (exp >= internal->MAX_SLABS) {
      throw std::runtime_error("Tried to allocate an object that was too large");
    }

    // Create a new SingleAllocator the first time this particular
    // rounded_size is needed.
    // TODO: Replace lock with atomic bool
    if (internal->slabs[exp] == nullptr) {
      std::lock_guard<std::mutex> lock(internal->mux_slabs);
      if (internal->slabs[exp] == nullptr) {
        internal->slabs[exp] = new Slab(pow2(exp));
      }
    }

    // Find the correct allocator for this size and use it do allocation
    Slab* slab = internal->slabs[exp];
    auto [p, unused1, unused2] = slab->allocate();

    return static_cast<value_type*>(p);
  }

  void deallocate(value_type* p, size_t n) noexcept
  {
    size_t exp = log2_int_ceil(n * sizeof(value_type));

    if (exp >= internal->MAX_SLABS) {
      throw std::runtime_error("Tried to deallocate an object that coudn't have been allocated because it was too large");
    } else {
      // Find the correct allocator for this size and use it do allocation
      Slab* slab = internal->slabs[exp];
      slab->deallocate(p);
    }
  }
};
