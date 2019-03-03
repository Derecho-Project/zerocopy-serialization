#pragma once

#include <array>
#include <iostream>
#include <vector>

// uint64_t
#include <cstdint>

// posix_memalign
#include <stdlib.h>

// Forward Declarations
template <int sz> struct SlabMetadata;
template <int sz> struct BlockMetadata;
template <int sz> struct Block;
template <int sz> struct Slab;
template <int sz> struct AlignedBlockVector;

template <int sz>
struct SlabMetadata {
  int num_blocks;
  uint64_t free_list;

  SlabMetadata() : num_blocks(0), free_list(-1ULL)
  { }

  SlabMetadata(int n, uint64_t fl = -1ULL)
    : num_blocks(n), free_list(fl)
  { }
};


template <int sz>
struct BlockMetadata {
  Slab<sz> *start;
  uint64_t free_list;

  BlockMetadata(Slab<sz> *s, uint64_t fl)
    : start(s), free_list(fl)
  { }
};


template <int sz>
struct Block {
  SlabMetadata<sz> slab_md;
  BlockMetadata<sz> block_md;
  std::array<char, 64*sz - sizeof(BlockMetadata<sz>)> data;

  Block (BlockMetadata<sz> md) : block_md(md)
  { }
};


template <int sz>
struct AlignedBlockVector {
  Block<sz> *blocks;

  AlignedBlockVector(Slab<sz> *slab) {
    posix_memalign((void**)&blocks, 64*sz, sizeof(Block<sz>));
    //blocks = (Block<sz>*) malloc(sizeof(Block<sz>));

    blocks[0].slab_md = SlabMetadata<sz>(1);
    blocks[0].block_md = BlockMetadata<sz>(slab, -1ULL);
  }

  void* allocate() {
    Block<sz> blk = blocks[0];

    int free_slot = ffsll(blk.slab_md.free_list);
    std::cout << "free_slot = " << free_slot << std::endl;

    return nullptr;
  }
};


template <int sz>
struct Slab {
  AlignedBlockVector<sz> blocks;

  Slab () : blocks(this) {
  }

  void* allocate() {
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
