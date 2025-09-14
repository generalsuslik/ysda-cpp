//
// Created by General Suslik on 14.09.2025.
//

#pragma once
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>

template <typename T>
class BufferedChannel {
public:
    explicit BufferedChannel(std::uint32_t size)
        : cap_{size}
        , is_open_{true}
    {
    }

    template <typename U>
    void Send(U&& val) {
        std::unique_lock lock{mutex_};
        cv_not_full_.wait(lock, [this] { return buffer_.size() < cap_ || !is_open_; });

        if (!is_open_) {
            throw std::runtime_error("Channel is closed");
        }

        buffer_.emplace_back(std::forward<U>(val));
        cv_not_empty_.notify_one();
    }

    std::optional<T> Recv() {
        std::unique_lock lock{mutex_};
        cv_not_empty_.wait(lock, [this] { return !buffer_.empty() || !is_open_; });

        if (buffer_.empty()) {
            return std::nullopt;
        }

        auto val = std::move(buffer_.front());
        buffer_.pop_front();
        cv_not_full_.notify_one();
        return val;
    }

    void Close() {
        std::unique_lock lock{mutex_};
        is_open_ = false;
        cv_not_empty_.notify_all();
        cv_not_full_.notify_all();
    }

private:
    std::uint32_t cap_;
    std::deque<T> buffer_;

    std::mutex mutex_;
    std::condition_variable cv_not_full_;
    std::condition_variable cv_not_empty_;
    bool is_open_;
};
