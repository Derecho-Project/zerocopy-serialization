#pragma once

#include <algorithm>
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
struct SlabMD;
template <size_t sz, bool is_head> struct BlockMD;
template <size_t sz, bool is_head> struct Block;
template <size_t sz> struct SlabInternal;
template <size_t sz> struct Slab;

template <size_t sz> using BlockHeadMD = BlockMD<sz, true>;
template <size_t sz> using BlockHead = Block<sz, true>;

template <size_t sz> using BlockTailMD = BlockMD<sz, false>;
template <size_t sz> using BlockTail = Block<sz, false>;



struct SlabMD {
  int num_blocks;
  uint64_t free_block_list;

  SlabMD() : num_blocks(0), free_block_list(-1ULL)
  { }

  SlabMD(int n, uint64_t fl = -1ULL)
    : num_blocks(n), free_block_list(fl)
  { }

  void print() {
    std::cout << "num_blocks: " << num_blocks << "\n"
              << "free_block_list: "
              << std::bitset<64>(free_block_list) << std::endl;
  }
};


template <size_t sz, bool is_head>
struct BlockMD {
  Slab<sz> *start;
  uint64_t free_slots;

  void print() {
    std::cout << "start: " << (void*) start << "\n"
              << "free_slots: "
              << std::bitset<64>(free_slots) << std::endl;
  }

  static constexpr size_t sizeof_metadata() {
    if constexpr (is_head) {
        return sizeof(SlabMD) + sizeof(BlockMD<sz, is_head>);
      }
    else {
      return sizeof(BlockMD<sz, is_head>);
    }
  }

  BlockMD()
    : start(nullptr), free_slots(-1ULL)
  { }

  BlockMD(Slab<sz> *s)
    : start(s), free_slots(-1ULL)
  {
    int num_taken = std::max(1UL, sizeof_metadata() / sz);

    for (int i = 0; i < num_taken; ++i) {
      bit_clear(free_slots, i+1);
    }
  }
};


template <size_t sz, bool is_head>
struct Block {
  BlockMD<sz, is_head> block_md;
  std::array<char, 64*sz - BlockMD<sz, is_head>::sizeof_metadata()> data;


  Block(Slab<sz> *slab) : block_md(slab)
  { }

  // Returns the pointer into the actual data,
  std::pair<void*, bool> find_free_slot() {
    // Note: returns the position as 1-indexed from the right (LSB)
    int free_slot_pos = ffsll(block_md.free_slots);
    assert(free_slot_pos != 0 && "No free slot found when calling find_free_slot");

    bit_clear(block_md.free_slots, free_slot_pos);

    void *ret;
    if constexpr (is_head) {
        ret = this + (free_slot_pos - 1) * sz - sizeof(SlabMD);
      } else {
      ret = this + (free_slot_pos - 1) * sz;
    }

    return {ret, block_md.free_slots == 0};
  }
};


template <size_t sz>
struct SlabInternal {
  SlabMD slab_md;
  BlockHead<sz> block_hd;
  BlockTail<sz> blocks[];

  SlabInternal()
    : slab_md(), block_hd()
  {}

  SlabInternal& operator=(const SlabInternal<sz>& rhs) {
    slab_md = rhs.slab_md;
    block_hd = rhs.block_hd;
    memcpy(blocks, rhs.blocks, rhs.slab_md.num_blocks * sizeof(BlockTail<sz>));

    return *this;
  }

  SlabInternal(const SlabInternal<sz>& rhs) {
    *this = rhs;
  }


  int total_num_blocks() const {
    return slab_md.num_blocks + 1;
  }
};


template <size_t sz>
struct Slab {
  SlabInternal<sz> *internal;
  Slab () {
    posix_memalign((void**)&internal, 64*sz,
                   sizeof(SlabMD) + sizeof(BlockHead<sz>));
    internal->slab_md = SlabMD();
    internal->block_hd.block_md = BlockHeadMD<sz>(this);
  }

  /**
     Get a [BlockTail<sz>] pointer to [internal->blocks[n]]
  */
  BlockTail<sz>* get_nth_block(int n) {
    return &(internal->blocks[n]);
  }

  std::tuple<void*, bool, void*> allocate() {
    // Note: returns the position as 1-indexed from the right (LSB)
    int free_block_pos = ffsll(internal->slab_md.free_block_list);
    void* ret;
    bool is_full;

    if (free_block_pos > internal->total_num_blocks()) {
      // There are no more free blocks from the blocks we've already allocated,
      // so we have to resize
      this->resize();
      auto [ret, _, internal] = this->allocate();
      return {ret, true, internal};
    } else if (free_block_pos == 1) {
      // Use block_hd
      std::tie(ret, is_full) = internal->block_hd.find_free_slot();
    } else if (free_block_pos != 0) {
      // Use blocks

      // This is the offset between what [ffsll] returns and the corresponding
      // index into [internal->blocks]
      const int BLOCK_INDEX_OFFSET = 2;

      BlockTail<sz> *block_ptr =
        get_nth_block(free_block_pos - BLOCK_INDEX_OFFSET);

      std::tie(ret, is_full) = block_ptr->find_free_slot();
    } else {
      // Couldn't find a free block in the free_block_list
      BlockTail<sz> *block_ptr;

      // Look for a free block in the list of blocks
      for (int i = 63; i < internal->slab_md.num_blocks; ++i) {
        BlockTail<sz> *tmp = get_nth_block(i);

        if (tmp->block_md.free_slots != 0) {
          block_ptr = tmp;
          break;
        }
      }

      // Check to see if a block was found. If not, resize the internals and
      // try to allocate again
      if (block_ptr != nullptr) {
        std::tie(ret, is_full) = block_ptr->find_free_slot();
      } else {
        this->resize();
        auto [ret, _, internal] = this->allocate();
        return {ret, true, internal};
      }
    }

    if (is_full) {
      bit_clear(internal->slab_md.free_block_list, free_block_pos);
    }

    return {ret, true, internal};
  }

  void resize() {
    int old_num_blocks = internal->slab_md.num_blocks;
    int new_num_blocks = std::max(1, 2 * internal->slab_md.num_blocks);

    SlabInternal<sz> *new_internal;

    // Allocate new internals, where we're doubling the total number of blocks
    // (including the block_hd)
    posix_memalign((void**)&new_internal, 64*sz,
                   sizeof(SlabMD) + sizeof(BlockHead<sz>)
                   + new_num_blocks * sizeof(BlockTail<sz>)
                   );

    // Copy all the data from the old internals to the new one
    *new_internal = *internal;

    // Update the number of blocks in the slab metadata
    new_internal->slab_md.num_blocks = new_num_blocks;

    // Initialize all the new blocks we created
    for (int i = old_num_blocks; i < new_num_blocks; ++i) {
      new_internal->blocks[i].block_md =
        BlockTailMD<sz>(new_internal->block_hd.block_md.start);
    }

    // Free the old slab internals
    free(internal);

    // Update the internals to now use this newly allocated internals
    internal = new_internal;
  }

  void deallocate(void* p) {
    // Pointers [p] have the form:
    // p = 64*sz*n + 64*sz*i + sz*j
    // where n is some integer (for alignment), i is the block that i lives in,
    // and j is the slot in the block
    int slot_num = (uint64_t(p) % (64*sz)) / sz;
    std::cout << "slot_num: " << slot_num << std::endl;
  }

};


static_assert(sizeof(Block<8, false>) == 64*8, "block size not 64*sz");
static_assert(sizeof(Block<8, true>) + sizeof(SlabMD)
              == 64*8, "block size not 64*sz");
