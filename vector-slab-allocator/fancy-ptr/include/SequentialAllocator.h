//
// Created by Alex Katz on 10/18/18.
//

#ifndef STL_EXPLORATION_SEQUENTIAL_ALLOCATOR_H
#define STL_EXPLORATION_SEQUENTIAL_ALLOCATOR_H

#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "config.h"
#include "fancy_pointer.h"
#include "slab_lookup_table.h"

inline size_t& get_offset(){
  static size_t offset = 0;
  return offset;
}

template<class T>
struct SequentialAllocator {
  typedef std::size_t            size_type;
  typedef T                      value_type;
  typedef fancy_pointer<T>       pointer;
  typedef fancy_pointer<const T> const_pointer;

  const size_type max_size;
  static size_type& offset;

  SequentialAllocator()
    : max_size (REGION_SIZE) {
      
    posix_memalign(reinterpret_cast<void**>(&slab_lookup_table[0][1]), getpagesize(), REGION_SIZE);
    std::memset(slab_lookup_table[0][1], 0, REGION_SIZE);
  }

  SequentialAllocator(const SequentialAllocator<T>& allocator)
    : max_size(REGION_SIZE) {}

  template<typename U>
  SequentialAllocator(const SequentialAllocator<U>& allocator)
    : max_size(REGION_SIZE) {}

  ~SequentialAllocator() {
    // Normally we'd free the region here, but it's done in main
  }

  pointer allocate(size_type n) {
    if (n * sizeof(T) > max_size - offset)
      return nullptr;

    std::size_t old_offset = offset;
    offset += n * sizeof(T);
    printf("[INFO] Allocated %lu bytes at offset %lu (address = %p)\n", n * sizeof(T), old_offset,
           static_cast<void*>(slab_lookup_table[0][1] + old_offset));
    return (pointer(0, 1, old_offset));
  }

  void deallocate(__attribute__((unused)) pointer ptr, __attribute__((unused)) size_type n) const {}
  
  char* get_region() const { return slab_lookup_table[0][1]; }
  
  bool address_was_allocated(uintptr_t addr) const {
    return (addr >= (uintptr_t)slab_lookup_table[0][1] && addr < (uintptr_t)slab_lookup_table[0][1] + max_size);
  }
};

template<typename T>
std::size_t& SequentialAllocator<T>::offset = get_offset();

template<class T, class U>
bool operator== (const SequentialAllocator<T>& a, const SequentialAllocator<U>& b) {
  return a.slab_lookup_table[0][1] == b.slab_lookup_table[0][1];
}

template<class T, class U>
bool operator!= (const SequentialAllocator<T>& a, const SequentialAllocator<U>& b) {
  return !(a == b);
}

#endif //STL_EXPLORATION_SEQUENTIAL_ALLOCATOR_H
