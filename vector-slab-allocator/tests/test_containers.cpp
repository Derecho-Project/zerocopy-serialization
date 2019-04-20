#include "slab_allocator.h"
#include "test_defs.h"

#include <list>
#include <vector>
#include <map>

using value_type = int;

int main(void) {
  size_t container_sz = 10;
  SlabAllocator<std::list<int, SlabAllocator<int>>> container_alloc;
  SlabAllocator<int> value_alloc{container_alloc};
  std::list<int, SlabAllocator<int>>& lst = *container_alloc.allocate(1);
  new (&lst) std::list<int, SlabAllocator<int>>(value_alloc);

  for (size_t i = 0; i < container_sz; ++i) {
    lst.push_back(i);
  }

  for (auto& n : lst) {
    std::cout << n << std::endl;
  }
}
