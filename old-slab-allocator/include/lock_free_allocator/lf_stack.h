#ifndef LF_STACK_H
#define LF_STACK_H

#include <memory>
#include <atomic>
#include "slab.h"

template<typename T>
class lock_free_stack {
    struct Node {
        T t;
        std::shared_ptr<Node> next;
    };

    //std::atomic_shared_ptr<Node> head; // C++20
    std::shared_ptr<Node> head;          // C++11

    lock_free_stack( lock_free_stack &) =delete;
    void operator=(lock_free_stack&) =delete;

public:
    lock_free_stack() =default;
    ~lock_free_stack() =default;

    class reference {
        std::shared_ptr<Node> p;
    public:
        reference(std::shared_ptr<Node> p_) : p{p_} { }
        T& operator* () { return p->t; }
        T* operator->() { return &p->t; }
    };

    [[nodiscard]]
    auto find( T t ) const {
        //auto p = head.load();      // C++20
        auto p = atomic_load(&head); // C++11

        while( p && p->t != t )
            p = p->next;
        return reference(move(p));
    }

    [[nodiscard]]
    auto front() const {
        //return reference(head);             // C++20
        return reference(atomic_load(&head)); // C++11
    }

    void push_front( T t ) {
        auto p = std::make_shared<Node>();
        p->t = t;

        //p->next = head.load();      // C++20
        p->next = atomic_load(&head); // C++11

        //while( !head.compare_exchange_weak(p->next, p) ){ }           // C++20
        while ( !atomic_compare_exchange_weak(&head, &p->next, p) ) { };// C++11
    }

    [[nodiscard]]
    auto pop_front() {
        //auto p = head.load();      // C++20
        auto p = atomic_load(&head); // C++11

        //while( p && !head.compare_exchange_weak(p, p->next) ){ }       //C++20
        while( p && atomic_compare_exchange_weak(&head, &p, p->next) ){ }//C++11

        return reference(move(p));
    }

};

/* Explicit specialization for T = Slab* */
template<>
class lock_free_stack<Slab*> {
    using T = Slab*;

    struct Node {
        T t;
        std::shared_ptr<Node> next;
    };

    //std::atomic_shared_ptr<Node> head; // C++20
    std::shared_ptr<Node> head;          // C++11

    lock_free_stack( lock_free_stack &) =delete;
    void operator=(lock_free_stack&) =delete;

public:
    lock_free_stack() =default;
    ~lock_free_stack() =default;

    class reference {
        std::shared_ptr<Node> p;
    public:
        reference(std::shared_ptr<Node> p_) : p{p_} { }
        T& operator* () { return p->t; }
        T* operator->() { return &p->t; }
    };

    [[nodiscard]]
    auto find( T t ) const {
        //auto p = head.load();      // C++20
        auto p = atomic_load(&head); // C++11

        while( p && p->t != t )
            p = p->next;
        return reference(move(p));
    }

    [[nodiscard]]
    auto front() const {
        //return reference(head);             // C++20
        return reference(atomic_load(&head)); // C++11
    }

    void push_front( T t ) {
        auto p = std::make_shared<Node>();
        p->t = t;

        //p->next = head.load();      // C++20
        p->next = atomic_load(&head); // C++11

        //while( !head.compare_exchange_weak(p->next, p) ){ }           // C++20
        while ( !atomic_compare_exchange_weak(&head, &p->next, p) ) { };// C++11
    }

    [[nodiscard]]
    auto pop_front() {
        //auto p = head.load();      // C++20
        auto p = atomic_load(&head); // C++11

        int new_num_free = p->t->free_slots.fetch_sub(1) - 1;
        if (new_num_free > 0) {
            return p->t;
        }

        //while( p && !head.compare_exchange_weak(p, p->next) ){ }       //C++20
        while( p && atomic_compare_exchange_weak(&head, &p, p->next) ){ }//C++11
    }

};

#endif /* ifndef LF_STACK_H */
