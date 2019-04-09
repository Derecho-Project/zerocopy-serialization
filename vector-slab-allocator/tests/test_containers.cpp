#include "slab_allocator.h"
#include "test_defs.h"

#include <list>
#include <vector>
#include <map>

using value_type = int;

int main(void) {
  int const container_sz = 10;
  //std::list<value_type> lst;
  SlabAllocator<std::pair<const value_type, value_type>> a;
  std::map<value_type, value_type, std::less<>, SlabAllocator<std::pair<const value_type, value_type>>> m(a);
  //std::vector<value_type, SlabAllocator<value_type>> vec;


  for (int i = 0; i < container_sz; ++i) {
    m.insert({value_type(i), value_type(i)});
    //lst.insert(std::end(lst), value_type(i));
    //vec.push_back(i);
  }

  //for (auto& n : vec) {
  //  std::cout << n << std::endl;
  //}

  for (auto& kv : m) {
  std::cout << kv.second << std::endl;
  }
}
