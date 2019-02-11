#include "../include/lock_free_allocator/slab_allocator.h"
#include <thread>
#include <benchmark/benchmark.h>

using namespace std;

class Test {
public:
    static int const sz = 16;
    int arr[sz];
};

static void escape(void *p) {
    asm volatile("" : : "g"(p) : "memory");
}

//static void clobber() {
//    asm volatile("" : : : "memory");
//}


void slab_allocator_create(benchmark::State &state) {
    if (state.thread_index == 0) {
    }

    for (auto _ : state) {
        SlabAllocator<Test> alloc;
        escape(&alloc);
    }

    if (state.thread_index == 0) {
        // Teardown code
    }
}

SlabAllocator<Test> slab_alloc;

void slab_allocator_allocate_deallocate(benchmark::State &state) {
    if (state.thread_index == 0) {
    }

    for (auto _ : state) {
        //std::cout << "--------- start test ----------\n";
        //std::cout << "Start iter" << std::endl;
        int const n = 100;
        Test* arr[n];
        for (int i = 0; i < n; ++i) {
            Test *p = slab_alloc.allocate(16);
            arr[i] = p;
        }
        for (int i = 0; i < n; ++i) {
            slab_alloc.deallocate(arr[i], 16);
        }
        //std::cout << "--------- end test ----------\n\n";
    }

    if (state.thread_index == 0) {
        // Teardown code
    }
}

void slab_allocator_alloc(benchmark::State &state) {
    if (state.thread_index == 0) {
    }

    for (auto _ : state) {
        Test *p = slab_alloc.allocate(16);
        slab_alloc.deallocate(p, 16);
    }

    if (state.thread_index == 0) {
        // Teardown code
    }
}

unsigned long long num = 0;

void baseline(benchmark::State &state) {
    if (state.thread_index == 0) {
    }

    for (auto _ : state) {
        ++num;
        escape(&num);
    }

    if (state.thread_index == 0) {
        // Teardown code
    }
}

std::atomic<uint64_t> num_atomic{0};
void atomic_inc(benchmark::State &state) {
    if (state.thread_index == 0) {
    }

    for (auto _ : state) {
        ++num_atomic;
        escape(&num_atomic);
    }

    if (state.thread_index == 0) {
        // Teardown code
    }
}

std::mutex mux;
int locked_num;
clock_t begin_time;
void locked_inc(benchmark::State &state) {
    if (state.thread_index == 0) {
        begin_time = clock();
    }

    for (auto _ : state) {
        std::lock_guard<std::mutex> lock(mux);
        ++locked_num;
        escape(&locked_num);
    }

    if (state.thread_index == 0) {
        std::cout << "Num: " << locked_num << std::endl;
        std::cout << "Time taken: " << float(clock() - begin_time) / CLOCKS_PER_SEC << std::endl;
        locked_num = 0;
    }
}


//BENCHMARK(slab_allocator_create)->RangeMultiplier(2)->ThreadRange(1, 128);
BENCHMARK(slab_allocator_alloc)->RangeMultiplier(2)->ThreadRange(1, 1024)->UseRealTime();
//BENCHMARK(slab_allocator_allocate_deallocate)->RangeMultiplier(2)->ThreadRange(1, 128)->UseRealTime();
//BENCHMARK(baseline)->RangeMultiplier(2)->ThreadRange(1, 128)->UseRealTime();
//BENCHMARK(atomic_inc)->RangeMultiplier(2)->ThreadRange(1, 128)->UseRealTime();
//BENCHMARK(locked_inc)->RangeMultiplier(2)->ThreadRange(1, 128)->UseRealTime();

BENCHMARK_MAIN();
