#pragma once

#include <iostream>
#include <array>

struct Test {
  int id;
  std::array<char, 64> arr;

  Test(int i) : id(i)
  {
    for (int j = 0; j < 64; ++j) {
      arr[j] = 'a' + (j % 26);
    }
  }

  friend bool operator==(Test const& lhs, Test const& rhs) {
    return lhs.id == rhs.id && lhs.arr == rhs.arr;
  }

  friend std::ostream& operator<<(std::ostream& os, Test &t) {
    return os << "{id = " << t.id << "; arr = "
              << std::string(t.arr.begin(), t.arr.end()) << "}";
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
