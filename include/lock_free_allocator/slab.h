#ifndef SLAB_H
#define SLAB_H

#include <bitset>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <atomic>
#include <sstream>

#include <strings.h>

struct Slab {
    // TODO: use atomic 
    static constexpr int MAX_SLOTS = 63;

    std::atomic<int> num_free{MAX_SLOTS};
        // Current number of free slots

    size_t sz;
        // Size of each individual slot

    std::atomic<uint64_t> free_slots{-1ULL};
        // 1 represent slot is in free, 0 represents in use.

    char *data;
        // Data layout:
        // [ Ptr to this slab | Slot1 | Slot2 | ... | Slot63 ]

    /**
     * Constructor
     * Creates a Slab where each slot has a size of 's'
     *
     * @param s The size of each slot in the slab
     */
    Slab(size_t s) : sz(s)
    {
        size_t alignment = Slab::get_alignment(sz);

        size_t data_sz = sz * MAX_SLOTS + sizeof(void*);

        posix_memalign(reinterpret_cast<void**>(&data), alignment, data_sz);
        //data = static_cast<char*>(aligned_alloc(alignment, data_sz));

        *reinterpret_cast<Slab**>(data) = this;
    }

    /**
     * lock_slot
     * This function expresses the intention to use a slot in the slab.
     * It is meant to express to other threads that there is one less slot
     * available.
     *
     * @return The new number of free slots
     */
    auto lock_slot() -> int {
        return num_free.fetch_sub(1) - 1;
    }

    /**
     * get_pointer
     * This function retrieves a pointer from a slab and marks the corresponding
     * slot as "not free" anymore.
     * The pointer 'p' returned satisfies the equation:
     *      p = (N * get_alignment(sz)) + sizeof(void*) + (slot_num * sz)
     * for some integer N, with
     *      get_alignment(sz) > sizeof(void*) + (slot_num * sz)
     *
     * PRECONDITION: lock_slot must be called first
     * @return A pointer for Slab::sz bytes
     */
    auto get_pointer() -> void* {
        int free_slot = 0;

        // Find a free slot and mark it as in use
        uint64_t desired_free_slots;
        uint64_t expected_free_slots = free_slots.load();
        do {
            free_slot = ffsll(expected_free_slots) - 1;
            if (free_slot == -1) {
                return nullptr;
            }
            desired_free_slots = expected_free_slots & (~(1ULL << free_slot));
        } while (!free_slots.compare_exchange_weak(expected_free_slots, desired_free_slots));

        //int new_num_free = num_free.fetch_sub(1) - 1;
        //std::cout << std::to_string(new_num_free) + "\n";
        
        void *p = data + (free_slot * sz) + sizeof(void*);
        return p;
    }

    /**
     * release_slot
     * Free's up the slot specified by 'slot_num'
     *
     * @param slot_num The slot to be free'd
     * @return The old number of free slots
     */
    auto release_slot(int slot_num) -> int {
        // Mark the slot_num as free now
        uint64_t desired_free_slots;
        uint64_t expected_free_slots = free_slots.load();
        do {
            desired_free_slots = expected_free_slots | (1ULL << slot_num);
        } while (!free_slots.compare_exchange_weak(expected_free_slots, desired_free_slots));
        return num_free.fetch_add(1);
    }

    /**
     * get_alignment
     * Calculate the alignment for the large chunk of memory allocated
     * by a slab. This varies based on the size of the slots.
     * We must satistfy
     *      get_alignment(sz) > sizeof(void*) + (slot_num * sz)
     * for any sz > 0 and 0 < slot_num < MAX_SLOTS
     *
     * @param sz The size of a slot in the slab you are interested in
     * @return The alignment for the large chunk of memory allocated by a slab
     */
    static auto get_alignment(size_t sz) -> size_t {
        return sz * 2 * (Slab::MAX_SLOTS + 1);
    }

    /**
     * find_slab_info
     * Find the slab and slot number associated with the pointer 'p'. This
     * works only due to the properties of pointers returned by 
     * Slab::get_pointer
     *
     * @param p A pointer that was previously returned from Slab::get_pointer
     * @param sz The size of a slot in the slab that 'p' originated
     * @return A pair with a pointer to the slab and the slot number that
     *  'p' lies
     */
    static auto find_slab_info(void *p, size_t sz) -> std::pair<Slab*, int> {
        // Alignment for Slab::data
        size_t alignment = Slab::get_alignment(sz);

        // Offset (# of bytes) into Slab::data
        size_t offset = ((long long)p) % alignment;

        // The slot index for p
        int slot_num = (offset - sizeof(void*)) / sz;

        Slab *slab = *((Slab**) (((char*)p) - offset));

        return {slab, slot_num};
    }

    /**
     * Destructor
     * Free the large chunk of memory originally allocated
     */
    ~Slab()
    {
        free(data);
    }

};

#endif /* ifndef SLAB_H */
