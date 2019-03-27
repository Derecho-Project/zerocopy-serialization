#include "../include/lock_free_allocator/slab_allocator.h"
#include <iostream>
#include <list>
#include <numeric>
#include <iterator>

int main(void)
{
  using ValueType = uint64_t;
  using AllocType = SlabAllocator<ValueType>;

  AllocType alloc;
  std::list<ValueType, AllocType> lst(100, alloc);
  int i = 0;
  for (auto it = lst.begin(); it != lst.end(); ++it) {
    *it = 42 + (i++);
  }

  for (int i = 0; i < alloc.internal_state->MAX_ALLOCATORS; ++i) {
    SingleAllocator* sa = alloc.internal_state->allocators[i];
    if (sa != nullptr) {
      for (Slab* slab : sa->all_slabs) {
        std::cout << "Slab data starts at: " << (void*) slab->data << std::endl;
      }
    }
  }

  auto start = lst.begin();
  std::advance(start, 50);
  return 0;
}
