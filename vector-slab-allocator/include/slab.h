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
struct BlockMD;
struct Block;

struct Slab {
  char *data;

  Slab(size_t s);

  SlabMD* slab_md();

  Block* nth_block(size_t n);

  std::tuple<void*, bool, void*> allocate();

  void deallocate(void* p);

  void resize();
};

struct SlabMD {
  int sz;
  int num_blocks;
  uint64_t free_block_list;

  SlabMD(int s) : sz(s), num_blocks(1), free_block_list(-1ULL)
  { }

  SlabMD(int s, int n, uint64_t fl = -1ULL)
    : sz(s), num_blocks(n), free_block_list(fl)
  { }
};

struct BlockMD {
  Slab *start;
  uint64_t free_slot_list;

  BlockMD()
    : start(nullptr), free_slot_list(-1ULL)
  { }

  BlockMD(Slab *s, size_t md_sz)
    : start(s), free_slot_list(-1ULL)
  {
    int num_taken = std::max(1UL, md_sz / s->slab_md()->sz);
    for (int i = 0; i < num_taken; ++i) {
      bit_clear(free_slot_list, i+1);
    }
  }
};

union Metadata {
  BlockMD block_md;
  struct {
    BlockMD b;
    SlabMD s;
  } all_md;
};

struct Block {
  char data[];

  Metadata* get_metadata() {
    return reinterpret_cast<Metadata*>(&data[0]);
  }

  BlockMD* block_md() {
    return &(reinterpret_cast<Metadata*>(&data[0])->block_md);
  }

  SlabMD* slab_md() {
    return &(reinterpret_cast<Metadata*>(&data[0])->all_md.s);
  }

  size_t get_slot_sz() {
    return block_md()->start->slab_md()->sz;
  }

  void initialize_head(Slab *slab, int s) {
    Metadata* md = this->get_metadata();
    md->all_md.s = SlabMD(s);
    md->all_md.b = BlockMD(slab, sizeof(md->all_md));
  }

  void initialize_tail(Slab *slab) {
    Metadata* md = this->get_metadata();
    md->block_md = BlockMD(slab, sizeof(md->block_md));
  }

  bool is_full() {
    return this->block_md()->free_slot_list == 0;
  }

  // Returns the pointer into the actual data
  std::pair<void*, bool> find_free_slot() {
    // Note: returns the position as 1-indexed from the right (LSB)
    BlockMD *bmd = this->block_md();
    int free_slot_pos = ffsll(bmd->free_slot_list);
    assert(free_slot_pos != 0 && "No free slot found when calling find_free_slot");

    bit_clear(bmd->free_slot_list, free_slot_pos);

    void *ret = &data[0] + (free_slot_pos - 1)*get_slot_sz();

    return {ret, bmd->free_slot_list == 0};
  }
};

Slab::Slab(size_t s)
{
  posix_memalign((void**)&data, 64*s, 64*s);
  nth_block(0)->initialize_head(this, s);
  std::cout << "Slab starts at address " << (void*)this << std::endl;
}

Block* Slab::nth_block(size_t n) {
  return reinterpret_cast<Block*>(&data[0] + (n * 64*this->slab_md()->sz));
}

SlabMD* Slab::slab_md() {
  return reinterpret_cast<Block*>(data)->slab_md();
}

std::tuple<void*, bool, void*> Slab::allocate() {
  // Note: returns the position as 1-indexed from the right (LSB)
  int free_block_pos = ffsll(this->slab_md()->free_block_list);
  void* ret = nullptr;
  bool is_full;

  if (free_block_pos > this->slab_md()->num_blocks) {
    // There are no more free blocks from the blocks we've already allocated,
    // so we have to resize
    this->resize();
    auto [ret, _, blocks] = this->allocate();
    return {ret, true, blocks};
  } else if (free_block_pos == 0) {
    // Couldn't find a free block in the free block list. Manually scan
    // through the remaining blocks to see if there is a free block
    for (int i = 64; i < this->slab_md()->num_blocks; ++i) {
      if (!nth_block(i)->is_full()) {
        std::tie(ret, is_full) = nth_block(i)->find_free_slot();
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
    std::tie(ret, is_full) = nth_block(free_block_pos - 1)->find_free_slot();

    // Only clear bits in the free_block_list for blocks found
    // in the free block list
    if (is_full) {
      bit_clear(this->slab_md()->free_block_list, free_block_pos);
    }
  }

  return {ret, false, data};
}

void Slab::deallocate(void* p) {
  // Pointers [p] have the form:
  // p = 64*sz*n + 64*sz*i + sz*j
  //   = 64*sz*(n+i) + sz*j
  // where sz is the size of each block slot, n is some integer (for alignment),
  // i is the block that i lives in, and j is the slot in the block
  int sz = this->slab_md()->sz;
  int slot_num = (uint64_t(p) % (64*sz)) / sz;

  Block *blk = reinterpret_cast<Block*>
    (static_cast<char*>(p) - slot_num*sz);

  Slab *slab = blk->block_md()->start;

  int block_num = ((uint64_t(blk)) - (uint64_t(slab->data))) / (64*sz);

  std::cout << "slot_num: " << slot_num << std::endl;
  std::cout << "block_num: " << block_num << std::endl;

  bit_set(blk->block_md()->free_slot_list, slot_num + 1);
  bit_set(slab->slab_md()->free_block_list, block_num + 1);
}

void Slab::resize() {
  int sz = this->slab_md()->sz;
  int old_num_blocks = this->slab_md()->num_blocks;
  int new_num_blocks = 2 * old_num_blocks;

  char *new_data;

  posix_memalign((void**)&new_data,
                 64*sz,
                 new_num_blocks * 64*sz);

  // Copy all old data into new one
  memcpy(new_data, data, old_num_blocks * 64*sz);

  // Free the old blocks
  free(data);

  // Update the blocks in the Slab to now be the new blocks
  data = new_data;

  // Update the number of blocks in the old data
  this->slab_md()->num_blocks = new_num_blocks;

  // Initialize all the new blocks we created
  for (int i = old_num_blocks; i < new_num_blocks; ++i) {
    this->nth_block(i)->initialize_tail(this);
  }
}
