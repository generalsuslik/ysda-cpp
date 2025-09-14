//
// Created by General Suslik on 14.09.2025.
//

#pragma once

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

constexpr std::uint64_t kCacheLineSize = 64;

template <typename T>
class MPMCBoundedQueue {
private:
    struct Node {
        alignas(kCacheLineSize) std::atomic_size_t generation;
        alignas(kCacheLineSize) T value;
    };

public:
    explicit MPMCBoundedQueue(const size_t max_size)
        : max_size_(max_size)
        , queue_(max_size)
    {
        for (size_t i = 0; i < max_size_; ++i) {
            queue_[i].generation.store(i);
        }
    }

    bool Enqueue(const T& val) {
        for (;;) {
            size_t tail = tail_.load();
            if (tail - head_.load() == max_size_) {
                return false;
            }
            Node& tail_node = queue_[tail % max_size_];
            if (tail == tail_node.generation.load() && tail_.compare_exchange_weak(tail, tail + 1)) {
                tail_node.value = val;
                tail_node.generation.store(tail + 1);
                return true;
            }

            /*
             * for (;;) {
             *     try CAS
             *
             *     if (success) return true
             *
             *     wait for some data to load (yes, it's better)
             *     so threads won't spin extremely tightly in the loop,
             *     burning CPU
             * }
             */
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    bool Dequeue(T& data) {
        for (;;) {
            size_t head = head_.load();
            if (head == tail_.load()) {
                return false;
            }
            Node& head_node = queue_[head % max_size_];
            if (head + 1 == head_node.generation.load() && head_.compare_exchange_weak(head, head + 1)) {
                data = head_node.value;
                head_node.generation.store(head + max_size_);
                return true;
            }

            // Same reasons as for Enqueue
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

private:
    const size_t max_size_;

    std::vector<Node> queue_;
    std::atomic_size_t head_{0};
    std::atomic_size_t tail_{0};

};
