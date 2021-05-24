
// Copyright 2011-2021 Victor Smirnov
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

#include <memoria/core/types.hpp>

namespace memoria {

template <typename ValueHolder_>
class CowBlockID {
    ValueHolder_ holder_;
public:
    using ValueHolder = ValueHolder_;

    CowBlockID() noexcept = default;
    explicit CowBlockID(ValueHolder value) noexcept:
        holder_(value)
    {}

    ValueHolder& value() noexcept {
        return holder_;
    }

    const ValueHolder& value() const noexcept {
        return holder_;
    }

    bool operator==(const CowBlockID& other) const noexcept {
        return holder_ == other.holder_;
    }

    bool operator!=(const CowBlockID& other) const noexcept {
        return holder_ != other.holder_;
    }

    bool operator<(const CowBlockID& other) const noexcept {
        return holder_ < other.holder_;
    }

    bool isSet() const noexcept {
        return holder_;
    }

    bool is_set() const noexcept {
        return holder_;
    }

    bool is_null() const noexcept {
        return !isSet();
    }

    operator bool() const noexcept {
        return (bool)holder_;
    }
};

template <typename VH>
std::ostream& operator<<(std::ostream& out, const CowBlockID<VH>& block_id) noexcept {
    out << block_id.value();

    return out;
}




}

namespace std {

template <typename ValueHolder>
class hash<memoria::CowBlockID<ValueHolder>> {
public:
    size_t operator()(const memoria::CowBlockID<ValueHolder>& obj) const noexcept {
        return std::hash<ValueHolder>()(obj.value());
    }
};

template <typename ValueHolder>
void swap(memoria::CowBlockID<ValueHolder>& one, memoria::CowBlockID<ValueHolder>& two) noexcept {
    std::swap(one.value(), two.value());
}

}
