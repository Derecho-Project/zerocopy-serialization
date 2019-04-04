#pragma once

#include "slab.h"
#include <iostream>
#include <array>

struct Test {
  int id;
  std::array<char, 64> arr;

  Test(int id) : id(id)
  {
    std::fill(arr.begin(), arr.end(), 'a');
  }

  friend bool operator==(Test const& lhs, Test const& rhs) {
    return lhs.id == rhs.id && lhs.arr == rhs.arr;
  }

  friend std::ostream& operator<<(std::ostream& os, Test &t) {
    return os << "{id = " << t.id << "; arr = "
              << std::string(t.arr.begin(), t.arr.end())
              << " (" << (void*) &t.arr[0] <<  ") }";
  }
};

static_assert(round_pow2(sizeof(Test)) == 128, "");
