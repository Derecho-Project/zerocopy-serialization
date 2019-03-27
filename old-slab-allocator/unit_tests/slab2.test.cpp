#include "catch.hpp"
#include "../include/lock_free_allocator/slab.h"

TEST_CASE( "A single slot (of size 8) in a slab can be inserted and removed" ) {
    Slab slab(8);

    int new_num_free = slab.lock_slot();
    
    SECTION ("The number of free slots is reduced") {
        REQUIRE( slab.num_free == Slab::MAX_SLOTS - 1 );
    }
    SECTION ("The returned value is the new number of free slots") {
        REQUIRE (new_num_free == Slab::MAX_SLOTS - 1);
    }

    void *p = slab.get_pointer();

    SECTION ("The first slot is marked as not free") {
        REQUIRE( slab.free_slots == (-1ULL) << 1 );
    }
    SECTION( "The pointer returned matches the slot that was marked free" ) {
        REQUIRE( p == slab.data + sizeof(void*) );
    }

    int old_num_free = slab.release_slot(0);

    SECTION ("Erasing the corresponding slot frees up the free_slots and num_free") {
        REQUIRE( slab.num_free == Slab::MAX_SLOTS );
        REQUIRE( slab.free_slots == -1ULL );
    }
    SECTION ("The returned value matches the old number of free slots") {
        REQUIRE( old_num_free == Slab::MAX_SLOTS - 1 );
    }
}
