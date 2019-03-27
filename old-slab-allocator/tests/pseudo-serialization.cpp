#include "../include/lock_free_allocator/slab_allocator.h"
#include <iostream>
#include <string>
#include <deque>

using namespace std;

int main(void)
{
    deque<int, SlabAllocator<int>> con1;
    for (int i = 0; i < 10; ++i) {
        con1.push_back(i);
    }
    //con1.get_allocator().viewState();

    deque<int, SlabAllocator<int>> con2;
    static_assert(sizeof(con1) == sizeof(con2), "container sizes are not equal!");
    memcpy((void*)&con2, (void*)&con1, sizeof(con1));
    memset((void*)&con1, 0, sizeof(con1));
    for (auto&& elt : con2) {
        cout << elt << ' ';
    }
    //con2.get_allocator().viewState();
    cout << endl;
    return 0;
}
