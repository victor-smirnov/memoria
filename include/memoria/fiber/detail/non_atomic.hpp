
//          Copyright Victor Smirnov 2023.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <atomic>

namespace memoria::fibers::detail {

template <typename T>
class NonAtomic {
    T value_;
public:
    NonAtomic() {}
    NonAtomic(T value): value_(value) {}

    T fetch_add(T v, std::memory_order order) {
        auto tmp = value_;
        value_ += v;
        return tmp;
    }

    T fetch_sub(T v, std::memory_order order) {
        auto tmp = value_;
        value_ -= v;
        return tmp;
    }

    void store(T v, std::memory_order order) {
        value_ = v;
    }

    T load(std::memory_order order) {
        return value_;
    }

    bool compare_exchange_strong(
            T& expected, T desired,
            std::memory_order, std::memory_order = std::memory_order_relaxed
    ) {
        if (expected == value_) {
            value_ = desired;
            return true;
        }
        else {
            expected = value_;
            return false;
        }
    }

    T exchange( T desired, std::memory_order = std::memory_order_seq_cst ) {
        auto tmp = value_;
        value_ = desired;
        return tmp;
    }
};

}
