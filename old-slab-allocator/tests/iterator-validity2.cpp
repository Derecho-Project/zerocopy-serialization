#include "../include/lock_free_allocator/slab_allocator.h"
#include <iostream>
#include <list>
#include <numeric>
#include <iterator>

template <typename T>
class ArenaAllocator {
  ArenaAllocator() {}

  std::shared_ptr<std::vector<std::any>> internal;
};

int main(void)
{
  using ValueType = uint64_t;
  using AllocType = SlabAllocator<ValueType>;

  AllocType alloc;
  std::list<ValueType, AllocType> lst(100, alloc);
  std::iota(lst.begin(), lst.end(), 0);

  auto start = lst.begin();
  std::advance(start, 50);
  return 0;
}
