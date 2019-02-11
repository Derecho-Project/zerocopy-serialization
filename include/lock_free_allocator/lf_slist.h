#ifndef LF_SLIST_H
#define LF_SLIST_H

#include <atomic>
#include "slab.h"

// Implementation adapted from Herb Sutter:
// https://www.youtube.com/watch?v=CmxkPChOcvw
class lf_slist {
public:
    lf_slist(Slab* s) {
        tail.store(new Node(s, nullptr));
        head.store(tail);
    }

    ~lf_slist() {
        Node *curr = head.load();
        while (curr != nullptr) {
            Node *unlinked = curr;
            curr = curr->next;
            delete unlinked;
        }
    }

    void push_back(Slab *s) {
        Node *new_node = new Node(s, nullptr);
        Node *expected_tail = tail.load();
        while (!tail.compare_exchange_weak(expected_tail, new_node)) {
            // LOOP
        }
        expected_tail->next = new_node;
    }

    //void pop_front(size_t default_size) {
    //    ListMetaData expected_md = metadata;
    //    ListMetaData desired_md;
    //    do {
    //        desired_md = expected_md;
    //        expected_md.head = expected_md.head->next;

    //        if (expected_md.head == nullptr) {
    //            this->push_back(new Slab(default_size));
    //            return;
    //        }
    //    } while (metadata.compare_exchange_weak(expected_md, desired_md));
    //}

    Slab* front() {
        return head.load()->slab;
    }

    struct Node {
        Slab *slab;
        Node *next;
        Node () {}
        Node (Slab* other_s, Node *other_next) : slab(other_s), next(other_next) {}
    };
    std::atomic<Node*> head {nullptr};
    std::atomic<Node*> tail {nullptr};

    lf_slist(lf_slist&) = delete;
    void operator=(lf_slist&) = delete;
};

class bad_slist {
public:
    bad_slist(Slab* s) {
        Node *start = new Node(s, nullptr);
        data.store(Obj(start, start));
        std::cout << std::boolalpha << data.is_lock_free() << std::endl;
    }

    void push_back(Slab *s) {
        Node *new_node = new Node(s, nullptr);
        Node *new_tail = new Node(nullptr, new_node);

        Obj desired_data;
        Obj expected_data = data.load();
        do {
            desired_data = expected_data;
            new_tail->slab = desired_data.tail->slab;
            expected_data.tail = new_tail;
        } while (data.compare_exchange_weak(expected_data, desired_data));
    }

    struct Node {
        Slab *slab;
        Node *next;
        Node () {}
        Node (Slab* other_s, Node *other_next) : slab(other_s), next(other_next) {}
    };

    struct Obj {
        Node *head = nullptr;
        Node *tail = nullptr;
        Obj() {}
        Obj(Node *h, Node *t) : head(h), tail(t) {}
    };
    std::atomic<Obj> data{ Obj() };

    bad_slist(bad_slist&) = delete;
    void operator=(bad_slist&) = delete;
};

#endif /* ifndef LF_SLIST_H */
