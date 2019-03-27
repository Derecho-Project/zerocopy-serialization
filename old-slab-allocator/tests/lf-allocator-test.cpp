#include "../include/lock_free_allocator/slab_allocator.h"
#include <thread>

using namespace std;

class Test {
public:
    static int const sz = 16;
    int arr[sz];
};

SlabAllocator<Test> alloc;

void threadFn(int const ID, int const n) {
    vector<Test*> arr(n);
    for (int iter = 0; iter < n/100; ++iter) {
        for (int i = 0; i < 100; ++i) {
            Test *p = alloc.allocate(16);
            arr[i] = p;
        }
        for (int i = 0; i < 100; ++i) {
            alloc.deallocate(arr[i], 16);
        }
    }
}

void test() {
    int const num_threads = 32;
    int const num_allocs = 10000;
    vector<thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(threadFn, i, num_allocs/num_threads);
    }
    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    //alloc.viewState();
}

int main(void)
{
    test();

    return 0;
}
