#pragma once

#include <cstddef>
#include <atomic>

// A lock free queue of non-owning pointers
template <typename T>
struct LowLockQueue {
private:
  static constexpr size_t CACHE_LINE_SIZE = 64;

  struct Node {
    static_assert(sizeof(T*) + sizeof(std::atomic<Node*>) <= CACHE_LINE_SIZE,
                  "Padding for Node is negative");

    Node( T* val ) : value(val), next(nullptr) { }
    T* value;
    std::atomic<Node*> next;
    char pad[CACHE_LINE_SIZE - sizeof(T*) - sizeof(std::atomic<Node*>)];
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
    while( first != nullptr ) {  // release the list
      Node* tmp = first;
      first = tmp->next;
      //delete tmp->value;         // no-op if null
      delete tmp;
    }
  }

  void Produce( T* t ) {
    Node* tmp = new Node(t);
    while( producerLock.exchange(true) )
      { }                 // acquire exclusivity
    last->next = tmp;     // publish to consumers
    last = tmp;           // swing last forward
    producerLock = false; // release exclusivity
  }

  bool Consume( T*& result ) {
    while( consumerLock.exchange(true) )
      { }                           // acquire exclusivity
    Node* theFirst = first;
    Node* theNext = first-> next;
    if( theNext != nullptr ) {      // if queue is nonempty
      T* val = theNext->value;      // take it out
      theNext->value = nullptr;     // of the Node
      first = theNext;              // swing first forward
      consumerLock = false;         // release exclusivity
      result = val;                 // now copy it back

      // delete val;                   // clean up the value

      delete theFirst;              // and the old dummy
      return true;                  // and report success
    }
    consumerLock = false;           // release exclusivity
    return false;                   // report queue was empty
  }

  T* Peek() {
    // Peek acts as a consumer that doesn't actually consume anything,
    // but shouldn't have the head switched out from underneath it
    while( consumerLock.exchange(true) )
      { }                           // acquire exclusivity
    Node* theFirst = first;
    Node* theNext = theFirst->next;
    consumerLock = false;           // release exclusivity
    return (theNext == nullptr ? nullptr : theNext->value);
  }

  bool empty() {
    return Peek() == nullptr;
  }

};


// All code below from:
// http://moodycamel.com/blog/2014/solving-the-aba-problem-for-lock-free-free-lists
/**
template <typename N>
struct FreeListNode
{
    FreeListNode() : freeListRefs(0), freeListNext(nullptr) { }

    std::atomic<std::uint32_t> freeListRefs;
    std::atomic<N*> freeListNext;
};

// A simple CAS-based lock-free free list. Not the fastest thing in the world under heavy contention,
// but simple and correct (assuming nodes are never freed until after the free list is destroyed),
// and fairly speedy under low contention.
template<typename N>    // N must inherit FreeListNode or have the same fields (and initialization)
struct FreeList
{
    FreeList() : freeListHead(nullptr) { }

    inline void add(N* node)
    {
        // We know that the should-be-on-freelist bit is 0 at this point, so it's safe to
        // set it using a fetch_add
        if (node->freeListRefs.fetch_add(SHOULD_BE_ON_FREELIST, std::memory_order_release) == 0) {
            // Oh look! We were the last ones referencing this node, and we know
            // we want to add it to the free list, so let's do it!
            add_knowing_refcount_is_zero(node);
        }
    }

    inline N* try_get()
    {
        auto head = freeListHead.load(std::memory_order_acquire);
        while (head != nullptr) {
            auto prevHead = head;
            auto refs = head->freeListRefs.load(std::memory_order_relaxed);
            if ((refs & REFS_MASK) == 0 || !head->freeListRefs.compare_exchange_strong(refs, refs + 1,
                    std::memory_order_acquire, std::memory_order_relaxed)) {
                head = freeListHead.load(std::memory_order_acquire);
                continue;
            }

            // Good, reference count has been incremented (it wasn't at zero), which means
            // we can read the next and not worry about it changing between now and the time
            // we do the CAS
            auto next = head->freeListNext.load(std::memory_order_relaxed);
            if (freeListHead.compare_exchange_strong(head, next,
                    std::memory_order_acquire, std::memory_order_relaxed)) {
                // Yay, got the node. This means it was on the list, which means
                // shouldBeOnFreeList must be false no matter the refcount (because
                // nobody else knows it's been taken off yet, it can't have been put back on).
                assert((head->freeListRefs.load(std::memory_order_relaxed) &
                    SHOULD_BE_ON_FREELIST) == 0);

                // Decrease refcount twice, once for our ref, and once for the list's ref
                head->freeListRefs.fetch_add(-2, std::memory_order_relaxed);

                return head;
            }

            // OK, the head must have changed on us, but we still need to decrease the refcount we
            // increased
            refs = prevHead->freeListRefs.fetch_add(-1, std::memory_order_acq_rel);
            if (refs == SHOULD_BE_ON_FREELIST + 1) {
                add_knowing_refcount_is_zero(prevHead);
            }
        }

        return nullptr;
    }

    // Useful for traversing the list when there's no contention (e.g. to destroy remaining nodes)
    N* head_unsafe() const { return freeListHead.load(std::memory_order_relaxed); }

private:
    inline void add_knowing_refcount_is_zero(N* node)
    {
        // Since the refcount is zero, and nobody can increase it once it's zero (except us, and we
        // run only one copy of this method per node at a time, i.e. the single thread case), then we
        // know we can safely change the next pointer of the node; however, once the refcount is back
        // above zero, then other threads could increase it (happens under heavy contention, when the
        // refcount goes to zero in between a load and a refcount increment of a node in try_get, then
        // back up to something non-zero, then the refcount increment is done by the other thread) --
        // so, if the CAS to add the node to the actual list fails, decrease the refcount and leave
        // the add operation to the next thread who puts the refcount back at zero (which could be us,
        // hence the loop).
        auto head = freeListHead.load(std::memory_order_relaxed);
        while (true) {
            node->freeListNext.store(head, std::memory_order_relaxed);
            node->freeListRefs.store(1, std::memory_order_release);
            if (!freeListHead.compare_exchange_strong(head, node,
                    std::memory_order_release, std::memory_order_relaxed)) {
                // Hmm, the add failed, but we can only try again when the refcount goes back to zero
                if (node->freeListRefs.fetch_add(SHOULD_BE_ON_FREELIST - 1,
                        std::memory_order_release) == 1) {
                    continue;
                }
            }
            return;
        }
    }

private:
    static const std::uint32_t REFS_MASK = 0x7FFFFFFF;
    static const std::uint32_t SHOULD_BE_ON_FREELIST = 0x80000000;

    // Implemented like a stack, but where node order doesn't matter (nodes are
    // inserted out of order under contention)
    std::atomic<N*> freeListHead;
};
*/


/**
// If you know your target architecture supports a double-wide compare-and-swap
// instruction, you can avoid the reference counting dance and use the naïve algorithm
// with a tag to avoid ABA, while still having it be lock-free. (If the tag of the head
// pointer is incremented each time it's changed, then ABA can't happen because all
// head values appear unique even if an element is re-added.)

template <typename N>
struct FreeListNode
{
    FreeListNode() : freeListNext(nullptr) { }

    std::atomic<N*> freeListNext;
};

template<typename N>    // N must inherit FreeListNode or have the same fields (and initialization)
struct FreeList
{
    FreeList()
        : head(HeadPtr())
    {
        assert(head.is_lock_free() && "Your platform must support DCAS for this to be lock-free");
    }

    inline void add(N* node)
    {
        HeadPtr currentHead = head.load(std::memory_order_relaxed);
        HeadPtr newHead = { node };
        do {
            newHead.tag = currentHead.tag + 1;
            node->freeListNext.store(currentHead.ptr, std::memory_order_relaxed);
        } while (!freeListHead.compare_exchange_weak(currentHead, newHead, std::memory_order_release, std::memory_order_relaxed));
    }

    inline N* try_get()
    {
        HeadPtr currentHead = head.load(std::memory_order_acquire);
        HeadPtr newHead;
        while (currentHead.ptr != nullptr) {
            newHead.ptr = currentHead.ptr->freeListNext.load(std::memory_order_relaxed);
            newHead.tag = currentHead.tag + 1;
            if (head.compare_exchange_weak(currentHead, newHead, std::memory_order_release, std::memory_order_acquire)) {
                break;
            }
        }
        return currentHead.ptr;
    }

    // Useful for traversing the list when there's no contention (e.g. to destroy remaining nodes)
    N* head_unsafe() const { return head.load(std::memory_order_relaxed).ptr; }

private:
    struct HeadPtr
    {
        N* ptr;
        std::uintptr_t tag;
    };

    std::atomic<HeadPtr> head;
};
*/
