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

using AllocatorType = SlabAllocator<uint64_t>;

auto getFileName(auto container) -> string {
  return string() + "out_" + typeid(container).name() + ".txt";
}

void testVector() {
  vector<uint64_t, AllocatorType> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.push_back(i);
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

void testDeque() {
  deque<uint64_t, AllocatorType> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.insert(end(container), i);
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

void testForwardList() {
  forward_list<uint64_t, AllocatorType> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.push_front(i);
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

void testList() {
  list<uint64_t, AllocatorType> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.insert(end(container), i);
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

// void testStack() {
//   stack<uint64_t, deque<uint64_t, AllocatorType>> container;
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
//   queue<uint64_t, deque<uint64_t, AllocatorType>> container;
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
//   priority_queue<uint64_t, vector<uint64_t, AllocatorType>> container;
//   string file_name = getFileName(container);
//   freopen(file_name.c_str(), "w", stdout);
//   for (int i = 0; i < n; ++i) {
//     container.push(i);
//   }
//   container.get_allocator().viewState();
//   fclose(stdout);
// }

void testSet() {
  set<uint64_t, less<uint64_t>, AllocatorType> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.insert(i);
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

void testMultiSet() {
  multiset<uint64_t, less<uint64_t>, AllocatorType> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.insert(i);
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

void testMap() {
  map<uint64_t, uint64_t, less<uint64_t>, SlabAllocator<pair<const uint64_t, uint64_t>>> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.insert({i, i});
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

void testMultiMap() {
  multimap<uint64_t, uint64_t, less<uint64_t>, SlabAllocator<pair<const uint64_t, uint64_t>>> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.insert({i, i});
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

void testUOSet() {
  unordered_set<uint64_t, hash<uint64_t>, equal_to<uint64_t>, AllocatorType> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.insert(i);
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

void testUOMultiSet() {
  unordered_multiset<uint64_t, hash<uint64_t>, equal_to<uint64_t>, AllocatorType> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.insert(i);
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

void testUOMap() {
  unordered_map<uint64_t, uint64_t, hash<uint64_t>, equal_to<uint64_t>, SlabAllocator<pair<const uint64_t, uint64_t>>> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.insert({i, i});
  }
  container.get_allocator().viewState();
  fclose(stdout);
}

void testUOMultiMap() {
  unordered_multimap<uint64_t, uint64_t, hash<uint64_t>, equal_to<uint64_t>, SlabAllocator<pair<const uint64_t, uint64_t>>> container;
  string file_name = getFileName(container);
  freopen(file_name.c_str(), "w", stdout);
  for (int i = 0; i < n; ++i) {
    container.insert({i, i});
  }
  container.get_allocator().viewState();
  fclose(stdout);
}


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
  testUOSet();
  testUOMultiSet();
  testUOMap();
  testUOMultiMap();
  return 0;
}
