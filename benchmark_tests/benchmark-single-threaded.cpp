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

void std_allocator_create(benchmark::State &state) {
    while (state.KeepRunning()) {
        std::allocator<Test> alloc;
        escape(&alloc);
    }
}

void std_allocator_allocate(benchmark::State &state) {
    while (state.KeepRunning()) {
        std::allocator<Test> alloc;
        Test *p = alloc.allocate(16);
        (void)p;
    }
}

void std_allocator_deallocate(benchmark::State &state) {
    while (state.KeepRunning()) {
        std::allocator<Test> alloc;
        Test *p = alloc.allocate(16);
        alloc.deallocate(p, 16);
    }
}

void slab_allocator_create(benchmark::State &state) {
    while (state.KeepRunning()) {
        SlabAllocator<Test> alloc;
    }
}

//void slab_allocator_create_delete(benchmark::State &state) {
//    while (state.KeepRunning()) {
//        SlabAllocator<Test> alloc;
//        alloc.clear();
//    }
//}

void slab_allocator_allocate_deallocate(benchmark::State &state) {
    while (state.KeepRunning()) {
        SlabAllocator<Test> alloc;
        Test *p = alloc.allocate(16);
        alloc.deallocate(p, 16);
    }
}


// Default allocator benchmarks
BENCHMARK(std_allocator_create);
BENCHMARK(std_allocator_allocate);
BENCHMARK(std_allocator_deallocate);

// Slab allocator benchmarks
BENCHMARK(slab_allocator_create);
BENCHMARK(slab_allocator_allocate_deallocate);


BENCHMARK_MAIN();
