#pragma once

#include <iostream>

struct Test {
  int id;
  char arr[64];

  friend std::ostream& operator<<(std::ostream& os, Test &t) {
    return os << "{id = " << t.id << "; arr = " << (void*) t.arr << "}";
  }
};

constexpr size_t round_pow2(size_t sz) {
  size_t rounded_size = 1;
  while (rounded_size < sz) {
    rounded_size <<= 1;
  }
  return rounded_size;
}

static_assert(round_pow2(sizeof(Test)) == 128, "");
