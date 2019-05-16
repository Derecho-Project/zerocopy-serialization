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
#include <vector>
#include <atomic>
#include <shared_mutex>

// Should conform to the FancyPointer named requirement
class Wrapper {
public:
  Wrapper(char *v, std::shared_mutex* m) : value(v), rw_mutex(m)
  {}

  ~Wrapper()
  {
    rw_mutex->unlock_shared();
  }

  std::shared_mutex *rw_mutex;
  char *value;
};

class Row {
public:
  // TODO: make this a row of "entries" that each have a shared mutex and a pointer,
  // so when we [get] an entry, we get a shared lock on the entry,
  // and when we [update] an entry, we get a unique lock on the entry
  std::vector<char*> _row;
  std::shared_mutex row_mutex;
  std::atomic<uint64_t> next_slab_id = {0ULL};

  // WRITER
  void resize(std::shared_lock<std::shared_mutex>& s_lock) {
    s_lock.unlock();

    if (is_empty()) {
      std::unique_lock u_lock(row_mutex);
      if (is_empty()) {
        _row.resize(2 * _row.capacity());
      }
    }
  }

  bool is_empty() {
    return next_slab_id >= _row.capacity();
  }

  uint64_t insert(char* slab_ptr) {
    std::shared_lock s_lock(row_mutex);

    while (1) {
      uint64_t slab_id = next_slab_id.fetch_add(1);

      if (is_empty()) {
        resize(s_lock);
      }

      _row[slab_id] = slab_ptr;
      return slab_id;
    }
  }

  // TODO: make sure we put something like a RW lock on *each entry* in the row
  // to prevent the value returned by [get] to go out of date because of a
  // call to [update]
  void update(uint64_t slab_id, char *slab_ptr) {
    std::shared_lock s_lock(row_mutex);
    _row[slab_id] = slab_ptr;
  }

  char* get(uint64_t slab_id) {
    row_mutex.lock_shared();
    char* slab = _row[slab_id];
    return slab;
  }

  char* operator[](int slab_id) {
    return get(slab_id);
  }
};

/*
              SLAB_ID
             0 1 2 3 4
        0: [ _ _ _ _ _ ]
  M_ID  1: [ _ _ _ _ _  _ _]
        2: [ _ _ _ _ _ _]

*/
class Table {
public:
  std::vector<Row*> machines;
  uint64_t next_machine_id = {0ULL};

  Row& operator[](int i) {
    return *machines[i];
  }

  void resize() {
    machines.resize(2 * machines.capacity());
  }

  void insertMachine() {
    uint64_t m_id = next_machine_id++;
    if (m_id > machines.capacity()) {
      resize();
    }
    machines[m_id] = new Row();
  }

};


Table slab_lookup_table = Table();


#endif
