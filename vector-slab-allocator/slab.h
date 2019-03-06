#pragma once

#include <array>
#include <iostream>
#include <utility>
#include <vector>

#include <bitset>
#include <type_traits>

// assert
#include <cassert>

// uint64_t
#include <cstdint>

// posix_memalign
#include <stdlib.h>

// Helper Functions
// Note: [n] is the position of the LSB, which is 1-indexed starting from the right.
void bit_set(uint64_t& num, int n) {
  num |= 1ULL << (n-1);
}

void bit_clear(uint64_t& num, int n) {
  num &= ~(1ULL << (n-1));
}

void bit_toggle(uint64_t& num, int n) {
  num ^= 1ULL << (n-1);
}

bool bit_check(uint64_t& num, int n) {
  return (num >> (n-1)) & 1U;
}

void bit_change(uint64_t& num, int n, int x) {
  num ^= (-x ^ num) & (1UL << (n-1));
}

// Forward Declarations
template <int sz> struct SlabMetadata;
template <int sz> struct BlockMetadata;
template <int sz> struct Block;
template <int sz> struct Slab;
template <int sz> struct AlignedBlockVector;

template <int sz>
struct SlabMetadata {
  int num_blocks;
  uint64_t free_blocks;

  SlabMetadata() : num_blocks(0), free_blocks(-1ULL)
  { }

  SlabMetadata(int n, uint64_t fl = -1ULL)
    : num_blocks(n), free_blocks(fl)
  { }
};


template <int sz>
struct BlockMetadata {
  Slab<sz> *start;
  uint64_t free_slots;

  BlockMetadata(Slab<sz> *s)
    : start(s), free_slots(-1ULL)
  {
    // Make sure that each block has its first slot marked as "full" since
    // the metadata will take up space, making the slot unusable.
    bit_clear(free_slots, 1);
  }
};


template <int sz>
struct Block {
  static constexpr size_t sizeof_all_metadata() {
    return sizeof(SlabMetadata<sz>) + sizeof(BlockMetadata<sz>);
  }

  // Note: ONLY use this field when the block is the FIRST in a list of blocks
  SlabMetadata<sz> slab_md;

  BlockMetadata<sz> block_md;

  std::array<char, 64*sz - sizeof_all_metadata()> data;

  // Returns the pointer into the actual data,
  std::pair<void*, bool> find_free_slot() {
    std::cout << std::bitset<64>(block_md.free_slots) << std::endl;

    // Note: returns the position as 1-indexed from the right (LSB)
    int free_slot_pos = ffsll(block_md.free_slots);
    assert(free_slot_pos != 0 && "No free slot found when calling find_free_slot");

    bit_clear(block_md.free_slots, free_slot_pos);

    void *ret = this + sizeof_all_metadata() + ((free_slot_pos - 1) * sz);

    return {ret, block_md.free_slots == 0};
  }
};


template <int sz>
struct AlignedBlockVector {
  Block<sz> *blocks;

  AlignedBlockVector(Slab<sz> *slab) {
    posix_memalign((void**)&blocks, 64*sz, sizeof(Block<sz>));
    //blocks = (Block<sz>*) malloc(sizeof(Block<sz>));

    blocks[0].slab_md = SlabMetadata<sz>(1);
    blocks[0].block_md = BlockMetadata<sz>(slab);
  }

  void resize() {
    std::cout << "Did resize" << std::endl;

    Block<sz>& blk_hd = blocks[0];

    int old_num_blocks = blk_hd.slab_md.num_blocks;
    int new_num_blocks = 2 * old_num_blocks;

    // Allocate a new list of blocks with double the number of blocks
    Block<sz> *new_blocks;
    posix_memalign((void**)&new_blocks, 64*sz, new_num_blocks * sizeof(Block<sz>));
    std::cout << "old_num_blocks: " << old_num_blocks << std::endl;
    std::cout << "new_num_blocks: " << new_num_blocks << std::endl;

    std::cout << "Doing memcpy" << std::endl;
    // Copy all the data from the old blocks to the new one
    memcpy(&new_blocks, &blocks, old_num_blocks);
    std::cout << "Done memcpy" << std::endl;

    // "Initialize" all the new blocks we created
    for (int i = old_num_blocks; i < new_num_blocks; ++i) {
      new_blocks[i].slab_md = SlabMetadata<sz>(1);
      new_blocks[i].block_md = BlockMetadata<sz>(blk_hd.block_md.start);
      std::cout << "new_blocks at " << i << " at mem: "
                << (void*)(&(new_blocks[i])) << std::endl;
    }

    // Update the number of blocks in the slab metadata
    new_blocks[0].slab_md.num_blocks = new_num_blocks;

    // Free the old block.
    free(blocks);

    // Set the blocks that this AlignedBlockVector manages to the new block
    blocks = new_blocks;

    std::cout << "Done resize" << std::endl;
  }

  std::tuple<void*, bool, void*> allocate() {
    Block<sz>& blk_hd = blocks[0];
    Block<sz> *block_ptr;
    bool did_resize = false;

    // Note: returns the position as 1-indexed from the right (LSB)
    int free_block_pos = ffsll(blk_hd.slab_md.free_blocks);

    std::cout << "free_block_pos: " << free_block_pos << std::endl;

    if (free_block_pos == 0) {
      for (int i = 64; i < blk_hd.slab_md.free_blocks; ++i) {
        Block<sz> *tmp = (Block<sz>*)
          (((char*) &blk_hd) + ((i - 1) * sizeof(Block<sz>)));

        if (tmp->block_md.free_slots != 0) {
          block_ptr = tmp;
          break;
        }
      }

      if (block_ptr == nullptr) {
        this->resize();
        did_resize = true;
        return allocate();
      }
    } else {
      if (free_block_pos > blk_hd.slab_md.num_blocks) {
        this->resize();
        did_resize = true;
      }
      block_ptr = (Block<sz>*)
        (((char*) &blk_hd) + ((free_block_pos - 1) * sizeof(Block<sz>)));
    }

    std::cout << "block_ptr = " << (void*)(block_ptr) << std::endl;

    auto [ret, is_full] = block_ptr->find_free_slot();

    if (is_full) {
      bit_clear(blk_hd.slab_md.free_blocks, free_block_pos);
    }

    return {ret, did_resize, blocks};
  }
};


template <int sz>
struct Slab {
  AlignedBlockVector<sz> blocks;

  Slab () : blocks(this) {
  }

  std::tuple<void*, bool, void*> allocate() {
    return blocks.allocate();
  }

  // FUNCTIONS
};



//template <int pow>
//constexpr void assertSlabMetadataSize();
//
//template <>
//constexpr void assertSlabMetadataSize()<0> {
//}
//
//template <int pow>
//constexpr void assertSlabMetadataSize() {
//  static_assert(sizeof(SlabMetadata<2 << pow>) == 16, "SlabMetadata was expected to have size 16");
//  assertSlabMetadataSize<pow-1>();
//}
//
//
//
//

static_assert(sizeof(SlabMetadata<8>) == 16, "SlabMetadata was expected to have size 16");
static_assert(sizeof(BlockMetadata<8>) == 16, "BlockMetadata was expected to have size 16");
