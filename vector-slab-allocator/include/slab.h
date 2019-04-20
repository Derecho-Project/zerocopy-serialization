#pragma once

#include "slab_lookup_table.h"

#include <array>
#include <iostream>
#include <shared_mutex>
#include <queue>

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

uint64_t bit_clear_copy(uint64_t& num, int n) {
  return num & ~(1ULL << (n-1));
}

uint64_t bit_set_copy(uint64_t& num, int n) {
  return num | (1ULL << (n-1));
}

// Forward Declarations
struct SlabMD;
struct BlockMD;
struct Block;

// struct BlockPtrQueue {
//   struct Node {
//     Block *data;
//     Node *next;
// 
//     Node() : data(nullptr), next(nullptr)
//     {}
// 
//     Node(Block *d, Node *n) : data(d), next(n)
//     {}
//   };
// 
//   Node *head;
//   Node *tail;
// 
//   BlockPtrQueue() : head(nullptr), tail(nullptr)
//   {}
// 
//   bool is_empty() {
//     return (head == nullptr);
//   }
// 
//   void enqueue(Block *item) {
//     if (is_empty()) {
//       head = new Node(item, )
//     } else {
//     }
//   }
// 
//   Block* dequeue() {
//   }
// 
// };

struct Slab {
  // A resize-able list of blocks, and a shared mutex for it
  char *blocks;
  std::shared_mutex mux_blocks;

  // A free list of blocks as a queue, with a mutex for it
  std::queue<Block*> free_blocks;
  std::mutex mux_free_blocks;

  // Create a slab of 1 block, where each slot in the block is [s] bytes
  Slab(size_t s);

  // Get the metadata for this slab
  SlabMD* slab_md();

  // Get the [n]th block for this slab
  Block* nth_block(size_t n);

  // Return a pointer [p] to a slot that can hold (at most) [slab_md()->sz] bytes
  void* allocate();

  // Free [p] in this slab, so that the space can be allocated again
  // Precondition: [p] must have been returned by a call to [this->allocate()]
  void deallocate(void* p);

  // Helper function to resize [blocks] once it gets full
  void resize(std::shared_lock<std::shared_mutex>& lock);
};

struct SlabMD {
  // Size of each slot in the slab
  int sz;

  // Number of blocks (currently allocated) in this slab
  int num_blocks;

  // Bitmap showing which blocks are free (for the first 64 blocks)
  uint64_t free_block_list;

  SlabMD(int s) : sz(s), num_blocks(1), free_block_list(-1ULL)
  { }

  SlabMD(int s, int n, uint64_t fl = -1ULL)
    : sz(s), num_blocks(n), free_block_list(fl)
  { }
};

struct BlockMD {
  // Pointer back to the slab that owns this block
  Slab *start;

  // Bitmap showing which slots are free in this block
  // 1 means slot is free, 0 means slot is used up.
  std::atomic<uint64_t> free_slot_list;

  // Create an invalid block metadata
  BlockMD()
    : start(nullptr), free_slot_list(-1ULL)
  { }

  // Create metadata for a block, where the block belongs in the slab [s] and
  // the size of its metadata is [md_sz].
  // Note: [md_sz] is necessary because in a Slab, the first Block contains
  // both slab metadata and block metadata, and all subsequent Blocks have
  // only block metadata.
  BlockMD(Slab *s, size_t md_sz)
    : start(s), free_slot_list(-1ULL)
  {
    // Mark, as not-free, the slots that have metadata
    int num_taken = std::max(1UL, md_sz / s->slab_md()->sz);
    for (int i = 0; i < num_taken; ++i) {
      bit_clear(free_slot_list, i+1);
    }
  }
};

// Union to ensure access to metadata is guaranteed by the compiler to be safe
union Metadata {
  BlockMD block_md;
  struct {
    BlockMD b;
    SlabMD s;
  } all_md;
};

// A large chunk of memory with 64 slots to store data
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

  // Initialize the first block in a list of blocks, which contains both
  // slab AND block metadata
  void initialize_head(Slab *slab, int s) {
    Metadata* md = this->get_metadata();
    md->all_md.s = SlabMD(s);
    md->all_md.b = BlockMD(slab, sizeof(md->all_md));
  }

  // Initialize subsequent blocks in a list of blocks, which only
  // contains block metadata
  void initialize_tail(Slab *slab) {
    Metadata* md = this->get_metadata();
    md->block_md = BlockMD(slab, sizeof(md->block_md));
  }

  // Check if this block is full, i.e. has no more free slots
  bool is_full() {
    return this->block_md()->free_slot_list == 0;
  }

  // Returns a pointer [p] to a slot that is guaranteed to be free.
  // If [p == nullptr], then there are no free slots in this block
  std::pair<void*, bool> find_free_slot() {
    // Note: returns the position as 1-indexed from the right (LSB)
    BlockMD *bmd = this->block_md();
    int free_slot_pos = 0;
    bool found_last_slot = false;

    // Find a free slot and mark it as in use
    uint64_t desired;
    uint64_t expected = bmd->free_slot_list.load();
    do {
      free_slot_pos = ffsll(expected);
      if (free_slot_pos == 0) {
        return {nullptr, false};
      }
      desired = bit_clear_copy(expected, free_slot_pos);
      found_last_slot = (desired == 0ULL);
    } while (!bmd->free_slot_list.compare_exchange_weak(expected, desired));

    void *ret = &data[0] + (free_slot_pos - 1)*get_slot_sz();
    return {ret, found_last_slot};
  }
};

// Round up to the nearest power of 2
constexpr size_t round_pow2(size_t sz) {
  size_t rounded_size = 1;
  while (rounded_size < sz) {
    rounded_size <<= 1;
  }
  return rounded_size;
}

// Compute ceil(log_2(n))
constexpr size_t log2_int_ceil(size_t n) {
  size_t rounded_size = 1;
  size_t exponent = 0;
  while (rounded_size < n) {
    rounded_size <<= 1;
    ++exponent;
  }
  return exponent;
}


Slab::Slab(size_t s)
{
  assert(s == round_pow2(s) && "Slabs can only have sizes of powers of 2");

  posix_memalign((void**)&blocks, 64*s, 64*s);

  nth_block(0)->initialize_head(this, s);

  slab_lookup_table[M_ID][log2_int_ceil(this->slab_md()->sz) + 1] =
    reinterpret_cast<char*>(this);
}

Block* Slab::nth_block(size_t n) {
  return reinterpret_cast<Block*>(&blocks[0] + (n * 64*this->slab_md()->sz));
}

SlabMD* Slab::slab_md() {
  return reinterpret_cast<Block*>(blocks)->slab_md();
}

void* Slab::allocate() {
  void* ret = nullptr;

  // Continue trying to allocate until we succeed
  while (ret != nullptr) {
    // Grab a shared lock when trying to allocate
    std::shared_lock lock(mux_blocks);

    // Note: returns the position as 1-indexed from the right (LSB)
    int free_block_pos = ffsll(this->slab_md()->free_block_list);
    bool is_full;

    if (free_block_pos > this->slab_md()->num_blocks) {
      // There are no more free blocks from the blocks we've already allocated,
      // so we have to resize and try allocating again
      this->resize(lock);
      continue;
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
      // If so, don't need to do anything else; there's nothing in the bitmap
      //   to update, so just return ret
      // If not, resize the blocks and try to allocate again.
      if (ret == nullptr) {
        this->resize(lock);
        continue;
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
  } // End while (ret != nullptr)

  return ret;
}

void Slab::allocate2() {
  void *ret = nullptr;

  while (ret == nullptr) {
    // Grab a shared lock when trying to allocate
    std::shared_lock lock(mux_blocks);

    mux_free_blocks.lock();
    // TODO: Fix race condition here.
    if (free_blocks.empty()) {
      mux_free_blocks.unlock();
      this->resize(lock);
    } else {
      Block *free_block = free_blocks.front();
      mux_free_blocks.unlock();

      bool found_last_slot = false;

      // Found a free block in the free_block_list
      std::tie(ret, found_last_slot) = free_block->find_free_slot();

      // "Delink"
      if (found_last_slot) {
        // Lock the queue
        std::unique_lock<std::mutex> lock(mux_free_blocks);
        // - "delink", i.e. remove block from queue
        free_blocks.pop();
      }
    }
  }
}

void Slab::deallocate2(void* p) {
  // Pointers [p] have the form:
  // p = 64*sz*n + 64*sz*i + sz*j
  //   = 64*sz*(n+i) + sz*j
  // where sz is the size of each block slot, n is some integer (for alignment),
  // i is the block that i lives in, and j is the slot in the block
  assert(p != nullptr);
  int sz = this->slab_md()->sz;
  int slot_num = (uint64_t(p) % (64*sz)) / sz;

  Block *blk = reinterpret_cast<Block*>
    (static_cast<char*>(p) - slot_num*sz);

  Slab *slab = blk->block_md()->start;

  int block_num = ((uint64_t(blk)) - (uint64_t(slab->blocks))) / (64*sz);

  uint64_t expected = blk->block_md()->free_slot_list.load();

  bool freed_first_slot = false;
  while(!blk->block_md()->free_slot_list
        .compare_exchange_weak(expected, bit_set_copy(expected, slot_num + 1)))
  {
    freed_first_slot = (expected == 0ULL);
  }

  if (freed_first_slot) {
    // Lock the queue
    std::unique_lock<std::mutex> lock(mux_free_blocks);
    // - "relink", i.e. add block to queue
    free_blocks.push(blk);
  }

  // compare_exchange_strong(free_slot_list, bit_set_copy(free_slot_list, slot_num + 1));
}

void Slab::deallocate(void* p) {
  // Pointers [p] have the form:
  // p = 64*sz*n + 64*sz*i + sz*j
  //   = 64*sz*(n+i) + sz*j
  // where sz is the size of each block slot, n is some integer (for alignment),
  // i is the block that i lives in, and j is the slot in the block
  assert(p != nullptr);
  int sz = this->slab_md()->sz;
  int slot_num = (uint64_t(p) % (64*sz)) / sz;

  Block *blk = reinterpret_cast<Block*>
    (static_cast<char*>(p) - slot_num*sz);

  Slab *slab = blk->block_md()->start;

  int block_num = ((uint64_t(blk)) - (uint64_t(slab->blocks))) / (64*sz);

  bit_set(blk->block_md()->free_slot_list, slot_num + 1);
  bit_set(slab->slab_md()->free_block_list, block_num + 1);
}

void Slab::resize(std::shared_lock<std::shared_mutex>& s_lock) {
  // Make sure the callee has acquired shared ownership of s_lock's resource
  assert(s_lock.owns_lock() &&
         "lock did not have shared ownership over the blocks");

  // Release shared ownership of resource
  s_lock.unlock();

  if (free_blocks.empty()) {

    std::unique_lock<std::shared_mutex> u_lock(mux_blocks);
    if (free_blocks.empty()) {
      // Block until we obtain exclusive ownership of the blocks, at which
      // point we can resize
      std::unique_lock u_blocks_lock(mux_blocks);

      int sz = this->slab_md()->sz;
      int old_num_blocks = this->slab_md()->num_blocks;
      int new_num_blocks = 2 * old_num_blocks;

      char *new_blocks;

      posix_memalign((void**)&new_blocks,
                     64*sz,
                     new_num_blocks * 64*sz);

      // Copy all old blocks into new one
      memcpy(new_blocks, blocks, old_num_blocks * 64*sz);

      // Free the old blocks
      free(blocks);

      // Update the blocks in the Slab to now be the new blocks
      blocks = new_blocks;

      // Update the number of blocks in the old blocks
      this->slab_md()->num_blocks = new_num_blocks;

      // Initialize all the new blocks we created
      for (int i = old_num_blocks; i < new_num_blocks; ++i) {
        this->nth_block(i)->initialize_tail(this);
        free_blocks.push(this->nth_block(i));
      }

      slab_lookup_table[M_ID][log2_int_ceil(this->slab_md()->sz) + 1] =
        reinterpret_cast<char*>(this);
    }
  }
}
