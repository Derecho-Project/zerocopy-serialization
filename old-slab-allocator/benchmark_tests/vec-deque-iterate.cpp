#include <benchmark/benchmark.h>
#include <vector>
#include <deque>
#include <iostream>

using namespace std;

static void escape(void *p) {
    asm volatile("" : : "g"(p) : "memory");
}

void vector_iterate(benchmark::State &state) {
    vector<long long> vec(state.range(0));
    escape(&vec);
    std::fill_n(vec.begin(), state.range(0), 42);

    for (auto it = vec.begin(); it != vec.end(); ++it) {
        ++(*it);
    }
}

void deque_iterate(benchmark::State &state) {
    deque<long long> deq(state.range(0));
    escape(&deq);

    std::fill_n(deq.begin(), state.range(0), 42);

    for (auto it = deq.begin(); it != deq.end(); ++it) {
        ++(*it);
    }
}

BENCHMARK(vector_iterate)->RangeMultiplier(2)->Range(1, 2 << 20);
BENCHMARK(deque_iterate)->RangeMultiplier(2)->Range(1, 2 << 20);

BENCHMARK_MAIN();
