#ifndef _SLAB_LOOKUP_TABLE_H
#define _SLAB_LOOKUP_TABLE_H

const int M_ID = 0; // Hard coded machine ID

// The slab lookup table is a 2D table, where the row numbers represent the
// machine ID, and the columns represent the slab ID. Entries are pointers
// to slabs.
// Note: we use char* so that pointer arithmetic is easier
// Note: [slab_lookup_table[M_ID][0]] is meant for all fancy pointers that
//       don't belong to a slab. This is necessary because containers create
//       fancy pointers sometimes that haven't been allocated from a
//       Slab/SlabAllocator. TODO: We may have to look into this
// TODO: Currently, the slab lookup table support allocation for ONLY ONE
//       object. The slab lookup table should be resizable
char* slab_lookup_table[1][40] = { {0} };

#endif
