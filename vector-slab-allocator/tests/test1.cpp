#include "slab.h"
#include "test_defs.h"
#include <array>
#include <iostream>

using value_type = Test;
constexpr size_t sz = round_pow2(sizeof(value_type));

int main(void)
{
  Slab<sz> slab = Slab<sz>();

  int const arr_sz = 1000;
  Block<sz> *blocks = slab.blocks;
  std::array<value_type*, arr_sz> entries;

  for (int i = 0; i < arr_sz; ++i) {
    auto [ret, did_resize, new_blocks] = slab.allocate();

    // Rewrite pointers if necessary
    if (did_resize) {
      for (int j = 0; j < i; ++j) {
        entries[j] = (value_type*) (
              (char*)(entries[j]) +
              ((uint64_t(new_blocks)) - (uint64_t(blocks)))
        );
      }
    }

    entries[i] = (value_type*) ret;
    (*(entries[i])).id = i;
    blocks = (Block<sz>*) new_blocks;
  }

  for (int i = 0; i < arr_sz; ++i) {
    std::cout << "Pointer at " << i << ": " << (void*)(entries[i]) << std::endl;
    std::cout << "Element " << i << ": " << *(entries[i]) << std::endl;
    slab.deallocate(entries[i]);
  }
}
