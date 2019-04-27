#include "slab.h"
#include "fancy_pointer.h"
#include "test_defs.h"
#include <array>
#include <iostream>
#include <memory>

// ==============================================================
// = Test Integration 3: Test Slabs with fancy pointers, so     =
// = manual rewriting should not be necessary                   =
// ==============================================================

using value_type = int;
using pointer = fancy_pointer<value_type>;
constexpr size_t sz = round_pow2(sizeof(value_type));

// Test that alloc --> dealloc --> alloc doesn't cause resizes on the
// second call to alloc
int main(void)
{
  Slab slab = Slab(sz);

  int const arr_sz = 1000;
  std::array<pointer, arr_sz> entries;

  for (int i = 0; i < arr_sz; ++i) {
    void *ret = slab.allocate();

    auto fp = pointer::pointer_to(*static_cast<value_type*>(ret));

    entries[i] = fp;
    *(entries[i]) = i;
  }

  for (int i = 0; i < arr_sz; ++i) {
    std::cout << "Pointer at " << i << ": " << (void*)&(*entries[i]) << std::endl;
    std::cout << "Element " << i << ": " << *(entries[i]) << std::endl;
    assert(*(entries[i]) == value_type(i) && "Value at ith entry was incorrect");
    slab.deallocate(entries[i]);
  }
}
