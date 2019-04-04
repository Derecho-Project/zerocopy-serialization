#include "slab_allocator.h"
#include "test_defs.h"
#include <array>
#include <iostream>
#include <memory>

// ==============================================================
// = Test Allocator 1: Test slab allocator (which uses fancy    =
// = pointers) on allocation and deallocation                   =
// ==============================================================

using value_type = Test;
using allocator_type = SlabAllocator<value_type>;
using pointer = std::allocator_traits<allocator_type>::pointer;

int main(void)
{
  allocator_type slab_alloc;

  int const arr_sz = 10;
  std::array<pointer, arr_sz> entries;

  for (int i = 0; i < arr_sz; ++i) {
    auto ret = slab_alloc.allocate(1);

    entries[i] = ret;
    *(entries[i]) = i;
  }

  for (int i = 0; i < arr_sz; ++i) {
    std::cout << "Pointer at " << i << ": " << (void*)&(*entries[i]) << std::endl;
    std::cout << "Element " << i << ": " << *(entries[i]) << std::endl;
    assert(*(entries[i]) == value_type(i) && "Value at ith entry was incorrect");
    slab_alloc.deallocate(entries[i], 1);
  }
}
