#pragma once

#include "slab.h"
#include "fancy_pointer.h"

// Computes 2^p
constexpr size_t pow2(size_t p) {
  return 1UL << p;
}

// Internal data for a slab allocator
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
  using pointer    = fancy_pointer<T>;
  using internals  = SlabAllocatorInternal;

  // Shared pointer for allocator internals so copy allocators is easy,
  // and can be automatically cleaned up once all copies are gone
  std::shared_ptr<internals> internal;

  // Default Constructor
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
  pointer allocate(size_t n)
  {
    printf("---------%s---------\n", __PRETTY_FUNCTION__);
    // The index into the array of slabs. The slab at this index is the
    // slab that best fits the objects being allocated.
    size_t exp = log2_int_ceil(n * sizeof(value_type));

    if (exp >= internal->MAX_SLABS) {
      throw std::runtime_error("Tried to allocate an object that was too large");
    }

    // Create a new Slab the first time this particular slab is needed.
    // Uses double-checked locking paradigm
    // TODO: Replace lock with atomic bool
    if (internal->slabs[exp] == nullptr) {
      std::lock_guard<std::mutex> lock(internal->mux_slabs);
      if (internal->slabs[exp] == nullptr) {
        internal->slabs[exp] = new Slab(pow2(exp));
      }
    }

    // Find the correct slab for this size and use it do allocation
    Slab* slab = internal->slabs[exp];
    auto [p, unused1, unused2] = slab->allocate();

    // Create a fancy pointer from the pointer allocated from the slab
    auto ret = pointer::pointer_to(*static_cast<value_type*>(p));
    std::cout << "-------- Returning " << ret << " -----------" << std::endl;
    return ret;
  }

  void deallocate(pointer p, size_t n) noexcept
  {
    if (p == nullptr) {
      return;
    }
    // The index into the array of slabs. The slab at this index is the
    // slab that allocated [p].
    size_t exp = log2_int_ceil(n * sizeof(value_type));

    if (exp >= internal->MAX_SLABS) {
      return;
      //throw std::runtime_error("Tried to deallocate an object that coudn't
      // have been allocated because it was too large");
    } else {
      // Find the correct slab for this size and use it do allocation
      Slab* slab = internal->slabs[exp];
      void *void_p = (static_cast<void*>(fancy_pointer<T>::to_address(p)));
      slab->deallocate(void_p);
    }
  }
};
