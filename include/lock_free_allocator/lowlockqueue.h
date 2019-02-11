#include <atomic>
#include <deque>
#include "slab.h"

constexpr int CACHE_LINE_SIZE = 64;

// Adapted from https://www.drdobbs.com/parallel/writing-a-generalized-concurrent-queue/211601363
struct LowLockQueue {
private:
  struct Node {
  Node( Slab* val ) : value(val), next(nullptr) { }
    Slab* value;
    std::atomic<Node*> next;
    char pad[CACHE_LINE_SIZE - sizeof(Slab*)- sizeof(std::atomic<Node*>)];
  };
  char pad0[CACHE_LINE_SIZE];

  // for one consumer at a time
  Node* first;

  char pad1[CACHE_LINE_SIZE - sizeof(Node*)];

  // shared among consumers
  std::atomic<bool> consumerLock;

  char pad2[CACHE_LINE_SIZE - sizeof(std::atomic<bool>)];

  // for one producer at a time
  Node* last; 

  char pad3[CACHE_LINE_SIZE - sizeof(Node*)];

  // shared among producers
  std::atomic<bool> producerLock;

  char pad4[CACHE_LINE_SIZE - sizeof(std::atomic<bool>)];

public:
  LowLockQueue() {
    first = last = new Node( nullptr );
    producerLock = consumerLock = false;
  }

  ~LowLockQueue() {
    while( first != nullptr ) {      // release the list
      Node* tmp = first;
      first = tmp->next;
      delete tmp->value;       // no-op if null
      delete tmp;
    }
  }

  void push_back( Slab *slab ) {
    Node* tmp = new Node( slab );
    while( producerLock.exchange(true) )
      { }   // acquire exclusivity
    last->next = tmp;     // publish to consumers
    last = tmp;         // swing last forward
    producerLock = false;     // release exclusivity
  }

  bool pop_front( Slab*& result, const size_t default_sz, std::deque<Slab*>& all_slabs ) {
    while( consumerLock.exchange(true) )
      { }   // acquire exclusivity
    Node* theFirst = first;
    Node* theNext = first-> next;
    if( theNext != nullptr ) {    // if queue is nonempty
      Slab* val = theNext->value;   // take it out
      int new_num_free = val->lock_slot();
      if (new_num_free != 0) {
        consumerLock = false;
        result = val;
        return true;
      } else {
        theNext->value = nullptr;  // of the Node
        first = theNext;       // swing first forward
        consumerLock = false;            // release exclusivity
        result = val;    // now copy it back
        //delete val;      // clean up the value
        delete theFirst;    // and the old dummy
        return true;     // and report success
      }
    } else {
      if (!producerLock.exchange(true)) {
        Slab *s = new Slab(default_sz);
        all_slabs.push_back(s);
        Node* tmp = new Node(s);
        tmp->value->num_free.fetch_sub(1);

        last->next = tmp;     // publish to consumers
        last = tmp;         // swing last forward
        producerLock = false;    // release exclusivity

        consumerLock = false;   // release exclusivity
        result = s;
        return true;
      } else {
        consumerLock = false;   // release exclusivity
        return false;           // report queue was empty
      }
    }
  }

  Slab* front() {
    Node *start = first->next.load();
    return (start != nullptr ? start->value : nullptr);
  }

  bool empty() {
    return (first->next.load() == nullptr);
  }
};

// Adapted from https://www.drdobbs.com/parallel/writing-a-generalized-concurrent-queue/211601363
//template <typename T>
//struct LowLockQueue {
//private:
//    struct Node {
//        Node( T* val ) : value(val), next(nullptr) { }
//        T* value;
//        std::atomic<Node*> next;
//        char pad[CACHE_LINE_SIZE - sizeof(T*)- sizeof(std::atomic<Node*>)];
//    };
//    char pad0[CACHE_LINE_SIZE];
//
//    // for one consumer at a time
//    Node* first;
//
//    char pad1[CACHE_LINE_SIZE - sizeof(Node*)];
//
//    // shared among consumers
//    std::atomic<bool> consumerLock;
//
//    char pad2[CACHE_LINE_SIZE - sizeof(std::atomic<bool>)];
//
//    // for one producer at a time
//    Node* last; 
//
//    char pad3[CACHE_LINE_SIZE - sizeof(Node*)];
//
//    // shared among producers
//    std::atomic<bool> producerLock;
//
//    char pad4[CACHE_LINE_SIZE - sizeof(std::atomic<bool>)];
//
//public:
//    LowLockQueue() {
//        first = last = new Node( nullptr );
//        producerLock = consumerLock = false;
//    }
//
//    ~LowLockQueue() {
//        while( first != nullptr ) {      // release the list
//            Node* tmp = first;
//            first = tmp->next;
//            delete tmp->value;       // no-op if null
//            delete tmp;
//        }
//    }
//
//    void Produce( const T& t ) {
//        Node* tmp = new Node( new T(t) );
//        while( producerLock.exchange(true) )
//        { }   // acquire exclusivity
//        last->next = tmp;     // publish to consumers
//        last = tmp;         // swing last forward
//        producerLock = false;     // release exclusivity
//    }
//
//    bool Consume( T& result ) {
//        while( consumerLock.exchange(true) ) 
//        { }   // acquire exclusivity
//        Node* theFirst = first;
//        Node* theNext = first-> next;
//        if( theNext != nullptr ) {    // if queue is nonempty
//            T* val = theNext->value;   // take it out
//            theNext->value = nullptr;  // of the Node
//            first = theNext;       // swing first forward
//            consumerLock = false;            // release exclusivity
//            result = *val;    // now copy it back
//            delete val;      // clean up the value
//            delete theFirst;    // and the old dummy
//            return true;     // and report success
//        }
//        consumerLock = false;   // release exclusivity
//        return false;                  // report queue was empty
//    }
//};
