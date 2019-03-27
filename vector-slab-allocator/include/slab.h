#pragma once

#include <array>
#include <iostream>

// uint64_t
#include <cstdint>

// size_t
#include <cstddef>

// Helper Functions
// Note: [n] is the position of the LSB, which is 1-indexed starting from the right.
void bit_clear(uint64_t& num, int n) {
  num &= ~(1ULL << (n-1));
}

void bit_set(uint64_t& num, int n) {
  num |= (1ULL << (n-1));
}

// Forward Declarations
struct SlabMD;
template <size_t sz> struct BlockMD;
template <size_t sz> struct Block;
template <size_t sz> struct Slab;

struct SlabMD {
  int num_blocks;
  uint64_t free_block_list;

  SlabMD() : num_blocks(1), free_block_list(-1ULL)
  { }

  SlabMD(int n, uint64_t fl = -1ULL)
    : num_blocks(n), free_block_list(fl)
  { }
};

template <size_t sz>
struct BlockMD {
  Slab<sz> *start;
  uint64_t free_slot_list;

  BlockMD()
    : start(nullptr), free_slot_list(-1ULL)
  { }

  BlockMD(Slab<sz> *s, size_t md_sz)
    : start(s), free_slot_list(-1ULL)
  {
    // TODO: Can probably do this completely constexpr
    int num_taken = std::max(1UL, md_sz / sz);
    for (int i = 0; i < num_taken; ++i) {
      bit_clear(free_slot_list, i+1);
    }
  }
};

template <size_t sz>
union Metadata {
  BlockMD<sz> block_md;
  struct {
    BlockMD<sz> b;
    SlabMD s;
  } all_md;
};

template <size_t sz>
struct Block {
  std::array<char, 64*sz> data;

  Metadata<sz>* get_metadata() {
    return reinterpret_cast<Metadata<sz>*>(&data[0]);
  }

  BlockMD<sz>* get_block_md() {
    return &(reinterpret_cast<Metadata<sz>*>(&data[0])->block_md);
  }

  SlabMD* get_slab_md() {
    return &(reinterpret_cast<Metadata<sz>*>(&data[0])->all_md.s);
  }

  void initialize_head(Slab<sz> *slab) {
    Metadata<sz>* md = this->get_metadata();
    md->all_md.s = SlabMD();
    md->all_md.b = BlockMD<sz>(slab, sizeof(md->all_md));
  }

  void initialize_tail(Slab<sz> *slab) {
    Metadata<sz>* md = this->get_metadata();
    md->block_md = BlockMD<sz>(slab, sizeof(md->block_md));
  }

  bool is_full() {
    return this->get_block_md()->free_slot_list == 0;
  }

  // Returns the pointer into the actual data
  std::pair<void*, bool> find_free_slot() {
    // Note: returns the position as 1-indexed from the right (LSB)
    BlockMD<sz> *bmd = this->get_block_md();
    int free_slot_pos = ffsll(bmd->free_slot_list);
    assert(free_slot_pos != 0 && "No free slot found when calling find_free_slot");

    bit_clear(bmd->free_slot_list, free_slot_pos);

    void *ret = &data[0] + (free_slot_pos - 1)*sz;

    return {ret, bmd->free_slot_list == 0};
  }
};

template <size_t sz>
struct Slab {
  Block<sz> *blocks;

  SlabMD* get_slab_md() {
    return blocks[0].get_slab_md();
  }

  Slab() {
    posix_memalign((void**)&blocks, 64*sz, sizeof(Block<sz>));
    blocks[0].initialize_head(this);
    std::cout << "Slab starts at address " << (void*)this << std::endl;
  }

  std::tuple<void*, bool, void*> allocate() {
    // Note: returns the position as 1-indexed from the right (LSB)
    int free_block_pos = ffsll(this->get_slab_md()->free_block_list);
    void* ret = nullptr;
    bool is_full;

    if (free_block_pos > this->get_slab_md()->num_blocks) {
      // There are no more free blocks from the blocks we've already allocated,
      // so we have to resize
      this->resize();
      auto [ret, _, blocks] = this->allocate();
      return {ret, true, blocks};
    } else if (free_block_pos == 0) {
      // Couldn't find a free block in the free block list. Manually scan
      // through the remaining blocks to see if there is a free block
      for (int i = 64; i < this->get_slab_md()->num_blocks; ++i) {
        if (!blocks[i].is_full()) {
          std::tie(ret, is_full) = blocks[i].find_free_slot();
          break;
        }
      }

      // Check to see if a block was actually found.
      // If so, don't need to do anything else.
      // If not, resize the blocks and try to allocate again. 
      if (ret == nullptr) {
        this->resize();
        auto [ret, _, blocks] = this->allocate();
        return {ret, true, blocks};
      }
    } else {
      // Found a free block in the free_block_list
      std::tie(ret, is_full) = blocks[free_block_pos - 1].find_free_slot();

      // Only clear bits in the free_block_list for blocks found
      // in the free block list
      if (is_full) {
        bit_clear(this->get_slab_md()->free_block_list, free_block_pos);
      }
    }

    return {ret, false, blocks};
  }

  void deallocate(void* p) {
    // Pointers [p] have the form:
    // p = 64*sz*n + 64*sz*i + sz*j
    //   = 64*sz*(n+i) + sz*j
    // where n is some integer (for alignment), i is the block that i lives in,
    // and j is the slot in the block
    int slot_num = (uint64_t(p) % (64*sz)) / sz;

    Block<sz> *blk = reinterpret_cast<Block<sz>*>
      (static_cast<char*>(p) - slot_num*sz);

    Slab<sz> *slab = blk->get_block_md()->start;

    int block_num = ((uint64_t(blk)) - (uint64_t(slab->blocks))) / (64*sz);

    std::cout << "slot_num: " << slot_num << std::endl;
    std::cout << "block_num: " << block_num << std::endl;

    bit_set(blk->get_block_md()->free_slot_list, slot_num + 1);
    bit_set(slab->get_slab_md()->free_block_list, block_num + 1);
  }

  void resize() {
    int old_num_blocks = this->get_slab_md()->num_blocks;
    int new_num_blocks = 2 * old_num_blocks;

    Block<sz> *new_blocks;

    posix_memalign((void**)&new_blocks,
                   64*sz,
                   new_num_blocks * sizeof(Block<sz>));

    // Copy all the old blocks to the new blocks
    for (int i = 0; i < old_num_blocks; ++i) {
      new_blocks[i] = blocks[i];
    }

    // Update the number of blocks in the new slab metadata
    new_blocks[0].get_slab_md()->num_blocks = new_num_blocks;

    // Initialize all the new blocks we created
    for (int i = old_num_blocks; i < new_num_blocks; ++i) {
      new_blocks[i].initialize_tail(this);
    }

    // Free the old blocks
    free(blocks);

    // Update the blocks in the Slab to now be the new blocks
    blocks = new_blocks;
  }

};
