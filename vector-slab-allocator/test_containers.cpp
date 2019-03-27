#include "slab_allocator.h"
#include "test_defs.h"

#include <list>

using value_type = int;

int main(void) {
  int const container_sz = 1000;
  std::list<value_type> lst;

  for (int i = 0; i < container_sz; ++i) {
    lst.insert(std::end(lst), value_type(i));
  }

  for (value_type& v : lst) {
    std::cout << v << std::endl;
  }
}
