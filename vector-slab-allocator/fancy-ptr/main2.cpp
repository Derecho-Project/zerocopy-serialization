#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include "fancy_pointer.h"
#include "slab_lookup_table.h"

int main() {
  // Calculate the fake slab size
  int slab_size = getpagesize() / 4;
  printf("[INFO] Page size is %d, setting fake slab size to %d\n", getpagesize(), getpagesize());
  
  // Allocate a page (enough for four fake slabs)
  char *new_region;
  int new_region_alignment = getpagesize();
  int new_region_size = getpagesize();
  posix_memalign((void **) &new_region, new_region_alignment, new_region_size);
  printf("[INFO] Allocated %d bytes with alignment %d at address %p\n",
        new_region_size, new_region_alignment, new_region);

  // Populate the slab lookup table
  slab_lookup_table[0][0] = new_region;
  slab_lookup_table[0][1] = new_region + slab_size;
  slab_lookup_table[0][2] = new_region + slab_size * 2;
  slab_lookup_table[0][3] = new_region + slab_size * 3;
  printf("[INFO] Slab 1 at %p\n[INFO] Slab 2 at %p\n[INFO] Slab 3 at %p\n[INFO] Slab 4 at %p\n",
          slab_lookup_table[0][0], slab_lookup_table[0][1], 
          slab_lookup_table[0][2], slab_lookup_table[0][3]);

  // Allocate two arrays of ints and copy them into slabs
  {
    char test1[] = "0123456789";
    char test2[] = "abcdefghij";
    std::memcpy(slab_lookup_table[0][0], &test1, sizeof(test1));
    std::memcpy(slab_lookup_table[0][1], &test2, sizeof(test2));
  }

  for (int i = 0; i < 10; i++) {
    auto fp = fancy_pointer<char>(0, 0, i);
    printf("%c\n", *fp);
  }

  fancy_pointer<void> p1 (0, 0, 5);
  fancy_pointer<char> p2 = static_cast<fancy_pointer<char>>(p1);  
  printf("%c at address %p\n", *p2, fancy_pointer<char>::to_address(p2));
}
