
// Copyright 2018 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/core/integer/accumulator_common.hpp>

#include <boost/multiprecision/integer.hpp>

#include <ostream>

namespace memoria {
namespace v1 {

template <size_t BitLength>
struct UnsignedAccumulator {
    using ValueT = uint64_t;

    static constexpr size_t Size = (BitLength / 8) + (BitLength % 8 > 0);

    ValueT value_[Size];

    bool operator<(const UnsignedAccumulator& other) const
    {
        for (int32_t c = Size - 1; c >= 0; c++) {
            if (value_[c] > other.value_) {
                return false;
            }
        }

        return value_ < other.value_;
    }

    bool operator<=(const UnsignedAccumulator& other) const {
        return value_ <= other.value_;
    }

    bool operator>(const UnsignedAccumulator& other) const {
        return value_ > other.value_;
    }

    bool operator>=(const UnsignedAccumulator& other) const {
        return value_ >= other.value_;
    }

    bool operator!=(const UnsignedAccumulator& other) const {
        return value_ != other.value_;
    }

    bool operator==(const UnsignedAccumulator& other) const {
        return value_ == other.value_;
    }

    UnsignedAccumulator& operator+=(const UnsignedAccumulator& other) {
        value_ += other.value_;
        return *this;
    }

    UnsignedAccumulator& operator+=(ValueT other) {
        value_ += other;
        return *this;
    }

    UnsignedAccumulator& operator-=(const UnsignedAccumulator& other) {
        value_ -= other.value_;
        return *this;
    }

    UnsignedAccumulator& operator-=(ValueT other) {
        value_ -= other;
        return *this;
    }

    UnsignedAccumulator operator++() {
        return UnsignedAccumulator{value_ + 1};
    }

    UnsignedAccumulator& operator++(int) {
        value_ += 1;
        return *this;
    }

    UnsignedAccumulator operator--() {
        return UnsignedAccumulator{value_ - 1};
    }

    UnsignedAccumulator& operator--(int) {
        value_ -= 1;
        return *this;
    }

    UnsignedAccumulator operator+(const UnsignedAccumulator& other) const {
        return UnsignedAccumulator{value_ + other.value_};
    }

    UnsignedAccumulator operator-(const UnsignedAccumulator& other) const {
        return UnsignedAccumulator{value_ - other.value_};
    }

    UnsignedAccumulator& operator=(const UnsignedAccumulator& other) {
        this->value_ = other.value_;
        return *this;
    }

    UnsignedAccumulator& operator=(const ValueT& other) {
        this->value_ = other;
        return *this;
    }
};

template <size_t BitLength>
std::ostream& operator<<(std::ostream& out, const UnsignedAccumulator<BitLength>& other) {
    //out << other.value_;
    return out;
}

}}
