#pragma once

#include "slab_lookup_table.h"
#include "lockfreequeue.h"

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

void bit_set(std::atomic<uint64_t>& num, int n) {
  num.fetch_or(1ULL << (n-1));
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

struct Slab {// A resize-able list of blocks, and a shared mutex for it
  char *blocks;
  std::shared_mutex mux_blocks;

  // Mutex to only allow a single thread calling [this->allocate()]
  std::mutex mux_allocate;

  // Slab ID
  uint64_t id;

  // A free list of blocks as a queue
  LowLockQueue<Block> free_blocks;

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
  template <typename FancyPtr>
  void deallocate(FancyPtr fp);

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
    uint64_t fsl = free_slot_list.load();
    for (int i = 0; i < num_taken; ++i) {
      bit_clear(fsl, i+1);
    }
    free_slot_list.fetch_and(fsl);
  }

  // Copy Constructor
  BlockMD(const BlockMD& other)
    : start(other.start), free_slot_list(other.free_slot_list.load())
  { }

  // Copy Assignment operator
  BlockMD& operator=(const BlockMD& rhs) {
    start = rhs.start;
    free_slot_list = rhs.free_slot_list.load();
    return *this;
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

  this->id = slab_lookup_table[M_ID].insert(reinterpret_cast<char*>(this));
  // slab_lookup_table[M_ID][log2_int_ceil(this->slab_md()->sz) + 1] =
  //   reinterpret_cast<char*>(this);
}

Block* Slab::nth_block(size_t n) {
  return reinterpret_cast<Block*>(&blocks[0] + (n * 64*this->slab_md()->sz));
}

SlabMD* Slab::slab_md() {
  return reinterpret_cast<Block*>(blocks)->slab_md();
}


void* Slab::allocate() {
  void *ret = nullptr;

  while (ret == nullptr) {
    // Only allow one thread to allocate at a time
    std::unique_lock allocate_lock(mux_allocate);

    // (I think) this shared lock is only necessary when there are multiple
    // threads allocating. Multiple deallocators would need have a shared lock
    // because of resizing, but only a single allocator wouldn't need to
    // since they are the one doing the resizing
    std::shared_lock s_lock(mux_blocks);

    Block *blk = free_blocks.Peek();
    if (blk == nullptr) {
      this->resize(s_lock);
    } else {
      bool found_last_slot = false;

      // Found a free block in the free_block_list
      std::tie(ret, found_last_slot) = blk->find_free_slot();

      // "Delink"
      if (found_last_slot) {
        Block *del;
        free_blocks.Consume(del);
      }
    }
  }

  return ret;
}

template <typename FancyPtr>
void Slab::deallocate(FancyPtr fp) {
  // Need to get a read lock on the blocks so it doesn't change. We don't
  // want the slab resizing on us as we're working on the block.
  std::shared_lock lock(mux_blocks);

  // Pointers [p] have the form:
  // p = 64*sz*n + 64*sz*i + sz*j
  //   = 64*sz*(n+i) + sz*j
  // where sz is the size of each block slot, n is some integer (for alignment),
  // i is the block that i lives in, and j is the slot in the block
  void *p = (static_cast<void*>(FancyPtr::to_address(fp)));
  assert(p != nullptr);

  int sz = this->slab_md()->sz;
  int slot_num = (uint64_t(p) % (64*sz)) / sz;

  Block *blk = reinterpret_cast<Block*>
    (static_cast<char*>(p) - slot_num*sz);

  Slab *slab = blk->block_md()->start;

  int block_num = ((uint64_t(blk)) - (uint64_t(slab->blocks))) / (64*sz);

  bool freed_first_slot = false;
  uint64_t expected = blk->block_md()->free_slot_list.load();

  while(!blk->block_md()->free_slot_list
        .compare_exchange_weak(expected, bit_set_copy(expected, slot_num + 1)))
  {
    freed_first_slot = (expected == 0ULL);
  }

  if (freed_first_slot) {
    free_blocks.Produce(blk);
  }
}

void Slab::resize(std::shared_lock<std::shared_mutex>& s_lock) {
  // Make sure the callee has acquired shared ownership of s_lock's resource
  assert(s_lock.owns_lock() &&
         "lock did not have shared ownership over the blocks");

  // Release shared ownership of resource
  s_lock.unlock();

  if (free_blocks.empty()) {

    // Block until we obtain exclusive ownership of the blocks, at which
    // point we can resize (if necessary)
    std::unique_lock<std::shared_mutex> u_lock(mux_blocks);

    if (free_blocks.empty()) {
      int sz = this->slab_md()->sz;
      int old_num_blocks = this->slab_md()->num_blocks;
      int new_num_blocks = 2 * old_num_blocks;

      char *new_blocks;

      posix_memalign((void**)&new_blocks,
                     64*sz,
                     new_num_blocks * 64*sz);

      // Copy all old blocks into new one
      memcpy(new_blocks, blocks, old_num_blocks * 64*sz);

      // Update the blocks in the Slab to now be the new blocks, keeping
      // the old block to be freed later
      char* old_blocks = blocks;
      blocks = new_blocks;

      // Update the number of blocks in the old blocks
      this->slab_md()->num_blocks = new_num_blocks;

      // Initialize all the new blocks we created
      for (int i = old_num_blocks; i < new_num_blocks; ++i) {
        this->nth_block(i)->initialize_tail(this);
        free_blocks.Produce(this->nth_block(i));
      }

      slab_lookup_table[M_ID].update(this->id, new_blocks);

      // Free the old blocks only once we've inserted into the slab lookup table
      free(old_blocks);
    }
  }
}
