#include "slab.h"
#include <array>

using ValueType = int;
constexpr size_t size = sizeof(int);

int main(void)
{
  Slab<size> slab = Slab<size>();

  int const arr_sz = 100;
  Block<size> *blocks = slab.blocks;
  std::array<ValueType*, arr_sz> entries;

  for (int i = 0; i < arr_sz; ++i) {
    auto [ret, did_resize, new_blocks] = slab.allocate();

    // Rewrite pointers if necessary
    if (did_resize) {
      for (int j = 0; j < i; ++j) {
        entries[i] = (ValueType*) (
              (char*)(entries[i]) +
              ((uint64_t(new_blocks)) - (uint64_t(blocks)))
        );
      }
    }

    entries[i] = (ValueType*) ret;
    *(entries[i]) = i;
    blocks = (Block<size>*) new_blocks;
  }

  for (int i = 0; i < arr_sz; ++i) {
    std::cout << "Pointer at " << i << ": " << (void*)(entries[i]) << std::endl;
    std::cout << "Element " << i << ": " << *(entries[i]) << std::endl;
    slab.deallocate(entries[i]);
  }
}
