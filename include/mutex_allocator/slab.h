#pragma once

#include <bitset>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <atomic>
#include <sstream>

struct Slab {
    // TODO: use atomic 
    static constexpr int MAX_SLOTS = 63;

    std::atomic<int> num_free{MAX_SLOTS};
        // Current number of free slots

    size_t sz;
        // Size of each individual slot

    std::bitset<MAX_SLOTS+1> slots_used;
        // 1 represent slot is in use, 0 represents free.

    std::mutex mux_slots_used;
        // Mutex for concurrent access to slots_used

    char *data;
        // Data layout: [ Ptr to this slab | Slot1 | Slot2 | ... | Slot63 ]

    // Constructs a Slab where each slot has a size of 's'
    Slab(size_t s) : sz(s)
    {
        size_t alignment = sz * 2 * (MAX_SLOTS+1);
        size_t data_sz = sz * MAX_SLOTS + sizeof(void*);
        posix_memalign((void**)&data, alignment, data_sz);
        //data = static_cast<char*>(aligned_alloc(alignment, data_sz));
        *((Slab**)data) = this;
    }

    void* insert() {
        size_t free_slot = 0;
        {
            std::lock_guard<std::mutex> lock(mux_slots_used);
            for (; free_slot < slots_used.size(); ++free_slot) {
                if (slots_used[free_slot] == 0) {
                    break;
                }
            }

            // Couldn't find a free slot
            if (free_slot == slots_used.size()) {
                return nullptr;
            }

            slots_used[free_slot] = 1;
        }

        num_free.fetch_sub(1);
        
        void *p = data + (free_slot * sz) + sizeof(void*);
        return p;
    }

    int erase(int slot_num) {
        {
            std::lock_guard<std::mutex> lock(mux_slots_used);
            slots_used[slot_num] = 0;
        }
        return num_free.fetch_add(1);
    }

    ~Slab()
    {
        free(data);
    }

};
