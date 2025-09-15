//
// Created by General Suslik on 15.09.2025.
//

#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>

class Semaphore {
public:
    explicit Semaphore(int count) : count_{count} {
    }

    void Acquire(auto callback) {
        std::unique_lock lock{mutex_};

        const auto& cv = std::make_shared<std::condition_variable>();
        cv_queue_.push(cv);

        cv->wait(lock, [&] { return cv == cv_queue_.front() && count_ > 0; });

        callback(count_);
        cv_queue_.pop();
    }

    void Acquire() {
        Acquire([](int& value) { --value; });
    }

    void Release() {
        std::lock_guard lock{mutex_};
        ++count_;
        if (!cv_queue_.empty()) {
            cv_queue_.front()->notify_one();
        }
    }

private:
    int count_;
    std::mutex mutex_;
    std::queue<std::shared_ptr<std::condition_variable>> cv_queue_;
};
