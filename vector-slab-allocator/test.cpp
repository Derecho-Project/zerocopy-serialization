#include "new_slab.h"
#include <array>

using ValueType = int;
constexpr size_t size = sizeof(int);

int main(void)
{
  Slab<size> slab = Slab<size>();

  int const arr_sz = 10;
  SlabInternal<size> *internal = slab.internal;
  std::array<ValueType*, arr_sz> entries;

  for (int i = 0; i < arr_sz; ++i) {
    auto [ret, did_resize, new_internal] = slab.allocate();

    // Rewrite pointers if necessary
    if (did_resize) {
      for (int j = 0; j < i; ++j) {
        entries[i] = (ValueType*) (
              (char*)(entries[i]) +
              ((uint64_t(new_internal)) - (uint64_t(internal)))
        );
      }
    }

    entries[i] = (ValueType*) ret;
    *(entries[i]) = i;
    internal = (SlabInternal<size>*) new_internal;
  }

  for (int i = 0; i < arr_sz; ++i) {
    std::cout << "Pointer at " << i << ": " << (void*)(entries[i]) << std::endl;
    std::cout << "Element " << i << ": " << *(entries[i]) << std::endl;
    slab.deallocate(entries[i]);
  }
}
