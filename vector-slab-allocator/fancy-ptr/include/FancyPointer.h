//
// Created by Alex Katz on 11/13/18.
//

#ifndef STL_EXPLORATION_FANCYPOINTER_H
#define STL_EXPLORATION_FANCYPOINTER_H

#include <cstddef>
#include <type_traits>

template<typename T>
struct FancyPointer {
  char* base = 0;
  std::size_t offset = 0;

  FancyPointer() {}

  FancyPointer(std::nullptr_t n) {}

  FancyPointer(char* base, std::size_t offset)
    : base(base)
    , offset(offset) {}

  FancyPointer(const FancyPointer<T>& p)
    : base (p.base)
    , offset(p.offset) {}

  template<typename U>
  explicit FancyPointer(const FancyPointer<U>& p)
    : base (p.base)
    , offset(p.offset) {}

  template<bool V = !std::is_void_v<T>>
  static const FancyPointer pointer_to(std::enable_if_t<V, T>& r) {
    return *(new FancyPointer<T>(reinterpret_cast<char*>(std::addressof(r)), 0));
  }

  static const T* to_address(FancyPointer<T> p) {
    return static_cast<T*>(p.base + p.offset);
  }
  
  T* operator->() const {return (T*)(base + offset);};
  T operator*() const {return *(T*)(base + offset);}
  explicit operator bool() const {return base != 0;}
};

template<typename T>
bool operator== (const FancyPointer<T>& a, const FancyPointer<T>& b) {
  return (a.base == b.base) && (a.offset == b.offset);
}

template<typename T>
bool operator!= (const FancyPointer<T>& a, const FancyPointer<T>& b) {
  return !(a == b);
}

template<typename T>
bool operator== (const FancyPointer<T>& a, std::nullptr_t n) {
  return (a.base == 0 && a.offset == 0);
}

template<typename T>
bool operator!= (const FancyPointer<T>& a, std::nullptr_t n) {
  return (a.base != 0 || a.offset != 0);
}

#endif //STL_EXPLORATION_FANCYPOINTER_H
