#include "../include/lock_free_allocator/slab_allocator.h"
#include "../include/lock_free_allocator/lf_slist.h"
#include <thread>
#include <benchmark/benchmark.h>

using namespace std;

static void escape(void *p) {
    asm volatile("" : : "g"(p) : "memory");
}

//static void clobber() {
//    asm volatile("" : : : "memory");
//}

Slab *init_slab = new Slab(16);
lf_slist* lst;

void benchmark_slist(benchmark::State &state) {
    if (state.thread_index == 0) {
        lst = new lf_slist(init_slab);
    }

    for (auto _ : state) {
        lst->push_back(init_slab);
        //lst->pop_front(16);
    }

    if (state.thread_index == 0) {
        delete lst;
    }
}

bad_slist* bad_lst;

void benchmark_bad_slist(benchmark::State &state) {
    if (state.thread_index == 0) {
        bad_lst = new bad_slist(init_slab);
    }

    for (auto _ : state) {
        bad_lst->push_back(init_slab);
        //lst->pop_front(16);
    }

    if (state.thread_index == 0) {
        delete bad_lst;
    }
}

std::mutex mux;
std::deque<Slab*> *deq;

void benchmark_deque(benchmark::State &state) {
    if (state.thread_index == 0) {
        deq = new std::deque<Slab*>();
    }

    for (auto _ : state) {
        std::lock_guard<std::mutex> lock(mux);
        deq->push_back(init_slab);
        //deq->pop_back();
    }

    if (state.thread_index == 0) {
        delete deq;
    }
}

void benchmark_malloc(benchmark::State &state) {
    if (state.thread_index == 0) {
    }

    for (auto _ : state) {
        lf_slist::Node *n = new lf_slist::Node(init_slab, nullptr);
        escape(n);
        delete n;
    }

    if (state.thread_index == 0) {
    }
}

//BENCHMARK(benchmark_bad_slist)->RangeMultiplier(2)->ThreadRange(1, 128)->UseRealTime();
BENCHMARK(benchmark_slist)->RangeMultiplier(2)->ThreadRange(1, 128)->UseRealTime();
//BENCHMARK(benchmark_deque)->RangeMultiplier(2)->ThreadRange(1, 128)->UseRealTime();
//BENCHMARK(benchmark_malloc)->RangeMultiplier(2)->ThreadRange(1, 128)->UseRealTime();

BENCHMARK_MAIN();
