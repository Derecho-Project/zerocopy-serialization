#include "slab.h"
#include "test_defs.h"
#include <array>
#include <iostream>

using value_type = int;
constexpr size_t sz = round_pow2(sizeof(value_type));

// Test that alloc --> dealloc --> alloc doesn't cause resizes on the
// second call to alloc
int main(void)
{
  Slab slab = Slab(sz);

  int const arr_sz = 10;
  char *data = slab.data;
  std::array<value_type*, arr_sz> entries;

  for (int i = 0; i < arr_sz; ++i) {
    auto [ret, did_resize, new_data] = slab.allocate();

		// Rewrite pointers if necessary
		if (did_resize) {
			for (int j = 0; j < i; ++j) {
				entries[j] = (value_type*) (
																		(char*)(entries[j]) +
																		((uint64_t(new_data)) - (uint64_t(data)))
																		);
			}
		}

		entries[i] = (value_type*) ret;
		*(entries[i]) = i;
		data = (char*) new_data;
	}

  for (int i = 0; i < arr_sz; ++i) {
    std::cout << "Pointer at " << i << ": " << (void*)(entries[i]) << std::endl;
    std::cout << "Element " << i << ": " << *(entries[i]) << std::endl;
    assert(*(entries[i]) == value_type(i) && "Value at ith entry was incorrect");
    slab.deallocate(entries[i]);
  }

  std::cout << "---------------------------------------------------" << std::endl;

  for (int i = 0; i < arr_sz; ++i) {
    auto [ret, did_resize, new_data] = slab.allocate();

		assert(!did_resize && "Blocks were resized, but it shouldn't have");

    // Rewrite pointers if necessary
		for (int j = 0; j < i; ++j) {
			entries[j] = (value_type*) (
																	(char*)(entries[j]) +
																	((uint64_t(new_data)) - (uint64_t(data)))
																	);
		}

		entries[i] = (value_type*) ret;
		*(entries[i]) = i;
		data = (char*) new_data;
  }

  for (int i = 0; i < arr_sz; ++i) {
    std::cout << "Pointer at " << i << ": " << (void*)(entries[i]) << std::endl;
    std::cout << "Element " << i << ": " << *(entries[i]) << std::endl;
    assert(*(entries[i]) == value_type(i) && "Value at ith entry was incorrect");
    slab.deallocate(entries[i]);
  }

}
