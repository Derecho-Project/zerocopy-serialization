#ifndef SINGLE_ALLOCATOR_H
#define SINGLE_ALLOCATOR_H

#include "slab.h"
#include "lowlockqueue.h"
#include <deque>
#include <stack>

struct SingleAllocator {
    LowLockQueue free_slabs;
        // Queue for all the free slabs available

    size_t sz;
        // Size of slots for all slabs created

    // used for debugging
    std::deque<Slab*> all_slabs;

    void viewState() {
        std::cout << "\t----- Viewing state for SingleAllocator(" << sz << ") -----" << std::endl;
        int slab_num = 0;
        for (Slab* slab : all_slabs) {
            std::cout << "\t----- Slab# " << slab_num << "-----\n"
                << "\tnum_free = " << slab->num_free << '\n'
                << "\tfree_slots = " << std::bitset<64>(slab->free_slots) << '\n'
                << "\t----------\n" << std::endl;
            ++slab_num;
        }
        std::cout << "\t----- End viewState for SingleAllocator -----\n\n\n" << std::endl;
    }

    // Constructs a SingleAllocator that contains Slabs, where each slot
    // in the Slab is of size 's'
    SingleAllocator(size_t s) : sz(s)
    {
        //std::cout << "Create singleallocator" << std::endl;
        free_slabs.push_back(new Slab(sz));
        all_slabs.emplace_back(free_slabs.front());
    }

    [[nodiscard]]
    void* allocate()
    {
        Slab *slab;

        bool success = free_slabs.pop_front(slab, sz, all_slabs);
        while (!success) {
          success = free_slabs.pop_front(slab, sz, all_slabs);
        }

        void *p = slab->get_pointer();

        return p;
    }

    void deallocate(void* p)
    {
        Slab *slab;
        int slot_num;
        std::tie(slab, slot_num) = Slab::find_slab_info(p, sz);
 
        int old_num_free = slab->release_slot(slot_num);

        // If the slab was full, add it to the free list since it now
        // has a free slot
        if (old_num_free == 0) {
            free_slabs.push_back(slab);
        }
    }

    ~SingleAllocator() {
    }

};

#endif /* ifndef SINGLE_ALLOCATOR_H */
