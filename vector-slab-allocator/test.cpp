#include "slab.h"

int main(void) {
  Slab<8> slab = Slab<8>();
  slab.allocate();
}
