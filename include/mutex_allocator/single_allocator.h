#pragma once

#include "slab.h"
#include <deque>
#include <stack>

struct SingleAllocator {
    std::deque<Slab*> free_slabs;
        // Queue for all the free slabs available

    std::mutex mux_free_slabs;
        // Mutex for concurrent access to free_slabs

    size_t sz;

    // used for debugging
    std::deque<Slab*> all_slabs;

    void viewState() {
        std::cout << "\t----- Viewing state for SingleAllocator(" << sz << ") -----" << std::endl;
        int slab_num = 0;
        for (Slab* slab : all_slabs) {
            std::cout << "\t----- Slab# " << slab_num << "-----\n"
                << "\tnum_free = " << slab->num_free << '\n'
                << "\tslots_used = " << slab->slots_used << '\n'
                << "\t----------\n" << std::endl;
            ++slab_num;
        }
        std::cout << "\t----- End viewState for SingleAllocator -----\n\n\n" << std::endl;
    }

    // Constructs a SingleAllocator that contains Slabs, where each slot
    // in the Slab is of size 's'
    SingleAllocator(size_t s) : sz(s)
    {
        free_slabs.emplace_back(new Slab(sz));
        all_slabs.emplace_back(free_slabs.front());
    }


    [[nodiscard]]
    void* allocate()
    {
        std::lock_guard<std::mutex> lock(mux_free_slabs);

        Slab *slab = free_slabs.front();
        void *p = slab->insert();

        // Remove the top (active) slab if there's no more space
        if (slab->num_free == 0) {
            free_slabs.pop_front();

            // Add a new slab if there's no more free space in the top (active) one
            if (free_slabs.size() == 0) {
                free_slabs.emplace_back(new Slab(sz));
                all_slabs.emplace_back(free_slabs.front());
            }
        }

        return p;
    }

    void deallocate(void* p)
    {
        // Alignment for Slab::data
        size_t alignment = sz * 2 * (Slab::MAX_SLOTS + 1);
        // Offset (# of bytes) into Slab::data
        size_t offset = ((long long)p) % alignment;
        // The slot index for p
        int slot_num = (offset - sizeof(void*)) / sz;

        Slab *slab = *((Slab**) (((char*)p) - offset));
        
        int old_num_free = slab->erase(slot_num);

        // If the slab was full, add it to the free list since it now
        // has a free slot
        if (old_num_free == 0) {
            std::lock_guard<std::mutex> lock(mux_free_slabs);
            free_slabs.push_back(slab);
        }
    }

    ~SingleAllocator() {
        for (Slab* slab : free_slabs) {
            delete slab;
        }
    }

};
