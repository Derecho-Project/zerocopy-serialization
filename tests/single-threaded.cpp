#include "../include/mutex_allocator/slab_allocator.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

class Test {
public:
    int num = 0;
    int arr[16];
    string str = "";

    Test() {}

    Test(int n, string s) : num(n), str(s) {}
};

void test0() {
    SlabAllocator<Test> test_sa;
    SlabAllocator<Test> test_sa2{test_sa};
}

void test1() {
    SlabAllocator<Test> test_sa;

    int n = 10;
    vector<Test, SlabAllocator<Test>> vec{test_sa};
    //vector<Test, SlabAllocator<Test>> vec;
    for (int i = 0; i < n; ++i) {
        vec.push_back(Test(1, "aasdokfnaonfoneaognoeawnogboeiwbfiobe"));
    }
    //vec.get_allocator().viewState();
    
    //SlabAllocator<int> int_sa;
    //vector<int, SlabAllocator<int>> vec2(int_sa);
    //for (int i = 0; i < n; ++i) {
    //    vec2.push_back(i);
    //}
    //int_sa.viewState();

    //cout << std::boolalpha << (int_sa == test_sa) << endl;
    //test_sa.clear();
}

void test2() {
    SlabAllocator<uint64_t> uint64_t_sa;

    //int n = 770;
    int n = 10000;
    deque<uint64_t, SlabAllocator<uint64_t>> lst(uint64_t_sa);
    for (int i = 0; i < n; ++i) {
        lst.push_back(1);
    }
    std::cout << "Done allocating." << std::endl;
    uint64_t_sa.viewState();

    //SlabAllocator<int> int_sa;
    //deque<int, SlabAllocator<int>> lst2(int_sa);
    //for (int i = 0; i < n; ++i) {
    //    lst2.push_back(i);
    //}

}

void test3() {
    SlabAllocator<Test> test_sa;

    {
        int n = 1000;
        deque<Test, SlabAllocator<Test>> lst(test_sa);
        int num_iters = 6;
        for (int i = 0; i < num_iters; ++i) {
            if (i % 3 == 2) {
                lst.clear();
                std::cout << "Cleared lst" << std::endl;
                //test_sa.viewState();
                continue;
            }

            for (int j = 0; j < n; ++j) {
                lst.push_back(Test(1, "aasdokfnaonfoneaognoeawnogboeiwbfiobe"));
                std::cout << "Inserted element #" << j << std::endl;
            }
            //test_sa.viewState();
        }
    }
    std::cout << "End of test3" << std::endl;
    //test_sa.viewState();
}

int main(void)
{
    test0();
    //test1();
    //test2();
    //test3();
    return 0;
}
