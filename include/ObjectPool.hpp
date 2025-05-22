#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <vector>

template <class T>
class ObjectPoolST {
public:
    explicit ObjectPoolST(size_t slab_count = 1, size_t SlabN = 1<<20) {
        slab_n_ = SlabN;
        while (slab_count--) addSlab();
    }

    inline T* acquire() noexcept {
        if (free_head_) {
            Node* n = free_head_;
            free_head_ = n->next;
            ++hits_;
            return &n->val;
        }
        return allocateFromSlab();
    }

    inline void release(T* obj) noexcept {
        Node* n = reinterpret_cast<Node*>(obj);
        n->next  = free_head_;
        free_head_ = n;
    }

    static inline void reset(T* obj) noexcept {
        std::memset(obj, 0, sizeof(T));
    }

    size_t capacity() const noexcept { return slabs_ * slab_n_; }
    size_t size()     const noexcept { return capacity() - freeCount(); }
    size_t hits()     const noexcept { return hits_; }

private:
    struct Node {
        T val;
        Node* next = nullptr;
    };

    inline T* allocateFromSlab() {
        if (cur_index_ >= slab_n_) {
            addSlab();
            ++cur_slab_idx_;
        }
        Node* n = slabs_mem_[cur_slab_idx_] + cur_index_++;
        return &n->val;
    }

    inline void addSlab() {
        Node* slab = static_cast<Node*>(
            ::operator new[](sizeof(Node) * slab_n_));
        slabs_mem_.push_back(slab);
        ++slabs_;
        cur_index_ = 0;
    }

    inline size_t freeCount() const noexcept {
        size_t cnt = 0;
        for (Node* p = free_head_; p; p = p->next) ++cnt;
        return cnt;
    }

    size_t slab_n_ = 1<<20;

    Node* free_head_ = nullptr;
    std::vector<Node*> slabs_mem_;
    size_t cur_index_ = 0;
    size_t cur_slab_idx_ = 0;
    size_t slabs_ = 0;
    size_t hits_ = 0;
};
