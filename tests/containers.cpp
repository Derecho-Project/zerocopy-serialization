#include "../include/lock_free_allocator/slab_allocator.h"
#include <iostream>

// All containers
#include <string>
#include <vector>
#include <array>
#include <deque>
#include <list>
#include <forward_list>
#include <stack>
#include <queue>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

using namespace std;

int const n = 1000;

struct UInt64Type {
  uint64_t num;

  UInt64Type(int i) : num(i) { }

  bool operator<(const UInt64Type& rhs) const {
    return num < rhs.num;
  }
};

struct IntArrType {
  int arr[256] = {0};

  IntArrType(int i) { }

  bool operator<(const IntArrType& rhs) const {
    return arr[0] < rhs.arr[0];
  }
};

using ValueType = IntArrType;

using AllocatorType = SlabAllocator<ValueType>;

auto getFileName(auto container) -> string {
  return string() + "out_" + typeid(container).name() + ".txt";
}

void testVector() {
  vector<ValueType, AllocatorType> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.push_back(ValueType(i));
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

void testDeque() {
  deque<ValueType, AllocatorType> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.insert(end(container), ValueType(i));
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

void testForwardList() {
  forward_list<ValueType, AllocatorType> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.push_front(ValueType(i));
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

void testList() {
  list<ValueType, AllocatorType> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.insert(end(container), ValueType(i));
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

// void testStack() {
//   stack<ValueType, deque<ValueType, AllocatorType>> container;
//   string file_name = getFileName(container);
//   freopen(file_name.c_str(), "w", stdout);
//   for (int i = 0; i < n; ++i) {
//     container.push(i);
//   }
//   container.get_allocator().viewState();
//   fclose(stdout);
// }
// 
// void testQueue() {
//   queue<ValueType, deque<ValueType, AllocatorType>> container;
//   string file_name = getFileName(container);
//   freopen(file_name.c_str(), "w", stdout);
//   for (int i = 0; i < n; ++i) {
//     container.push(i);
//   }
//   container.get_allocator().viewState();
//   fclose(stdout);
// }
// 
// void testPriorityQueue() {
//   priority_queue<ValueType, vector<ValueType, AllocatorType>> container;
//   string file_name = getFileName(container);
//   freopen(file_name.c_str(), "w", stdout);
//   for (int i = 0; i < n; ++i) {
//     container.push(i);
//   }
//   container.get_allocator().viewState();
//   fclose(stdout);
// }

void testSet() {
  set<ValueType, less<ValueType>, AllocatorType> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.insert(ValueType(i));
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

void testMultiSet() {
  multiset<ValueType, less<ValueType>, AllocatorType> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.insert(ValueType(i));
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

void testMap() {
  map<ValueType, ValueType, less<ValueType>, SlabAllocator<pair<const ValueType, ValueType>>> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.insert({ValueType(i), ValueType(i)});
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

void testMultiMap() {
  multimap<ValueType, ValueType, less<ValueType>, SlabAllocator<pair<const ValueType, ValueType>>> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.insert({ValueType(i), ValueType(i)});
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

// void testUOSet() {
//   unordered_set<ValueType, hash<ValueType>, equal_to<ValueType>, AllocatorType> container;
//   string file_name = getFileName(container);
//   freopen(file_name.c_str(), "w", stdout);
//   for (int i = 0; i < n; ++i) {
//     container.insert(i);
//   }
//   container.get_allocator().viewState();
//   fclose(stdout);
// }

// void testUOMultiSet() {
//   unordered_multiset<ValueType, hash<ValueType>, equal_to<ValueType>, AllocatorType> container;
//   string file_name = getFileName(container);
//   freopen(file_name.c_str(), "w", stdout);
//   for (int i = 0; i < n; ++i) {
//     container.insert(i);
//   }
//   container.get_allocator().viewState();
//   fclose(stdout);
// }

// void testUOMap() {
//   unordered_map<ValueType, ValueType, hash<ValueType>, equal_to<ValueType>, SlabAllocator<pair<const ValueType, ValueType>>> container;
//   string file_name = getFileName(container);
//   freopen(file_name.c_str(), "w", stdout);
//   for (int i = 0; i < n; ++i) {
//     container.insert({i, i});
//   }
//   container.get_allocator().viewState();
//   fclose(stdout);
// }

// void testUOMultiMap() {
//   unordered_multimap<ValueType, ValueType, hash<ValueType>, equal_to<ValueType>, SlabAllocator<pair<const ValueType, ValueType>>> container;
//   string file_name = getFileName(container);
//   freopen(file_name.c_str(), "w", stdout);
//   for (int i = 0; i < n; ++i) {
//     container.insert({i, i});
//   }
//   container.get_allocator().viewState();
//   fclose(stdout);
// }


int main(void)
{
  testVector();
  testForwardList();
  testList();
  testDeque();
  testSet();
  testMultiSet();
  testMap();
  testMultiMap();
  // testUOSet();
  // testUOMultiSet();
  // testUOMap();
  // testUOMultiMap();
  return 0;
}
