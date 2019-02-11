#include "../include/mutex_allocator/slab_allocator.h"
#include <thread>

using namespace std;

class Test {
public:
    static int const sz = 16;
    int arr[sz];
};

SlabAllocator<Test> alloc;

void threadFn(int const ID, int const n) {
    deque<Test, SlabAllocator<Test>> nums(alloc);

    for (int i = 0; i < n; ++i) {
        nums.push_back(Test());
        for (int j = 0; j < Test::sz; ++j) {
            nums.back().arr[j] = ID;
        }
    }

    for (int i = 0; i < n; ++i) {
        Test t = nums.front();
        nums.pop_front();
        for (int j = 0; j < Test::sz; ++j) {
            if (t.arr[j] != ID) {
                throw std::runtime_error("Bad value stored in arr");
            }
        }
    }
}

void test() {
    int const num_threads = 128;
    int const num_allocs = 1000000;
    vector<thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(threadFn, i, num_allocs/num_threads);
    }
    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    alloc.viewState();
}

int main(void)
{
    test();

    return 0;
}
