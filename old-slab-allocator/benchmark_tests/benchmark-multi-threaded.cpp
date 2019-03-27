#include "../include/mutex_allocator/slab_allocator.h"
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

static void clobber() {
    asm volatile("" : : : "memory");
}


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

void slab_allocator_allocate_deallocate(benchmark::State &state) {
    if (state.thread_index == 0) {
    }

    for (auto _ : state) {
        //std::cout << "--------- start test ----------\n";
        //std::cout << "Start iter" << std::endl;
        Test* arr[100];
        for (int i = 0; i < 100; ++i) {
            Test *p = slab_alloc.allocate(16);
            arr[i] = p;
        }
        for (int i = 0; i < 100; ++i) {
            slab_alloc.deallocate(arr[i], 16);
        }
        //std::cout << "--------- end test ----------\n\n";
    }

    if (state.thread_index == 0) {
        // Teardown code
    }
}

//BENCHMARK(slab_allocator_create)->RangeMultiplier(2)->ThreadRange(1, 128)->UseRealTime();
BENCHMARK(slab_allocator_alloc)->RangeMultiplier(2)->ThreadRange(1, 1024)->UseRealTime();
//BENCHMARK(slab_allocator_allocate_deallocate)->RangeMultiplier(2)->ThreadRange(1, 1024)->UseRealTime();

BENCHMARK_MAIN();
