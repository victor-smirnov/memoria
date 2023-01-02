
// Copyright 2019-2023 Victor Smirnov
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
#include <memoria/core/memory/malloc.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/lifetime_guard.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/core/tools/result.hpp>

#include <type_traits>
#include <functional>
#include <algorithm>


namespace memoria {


template <typename T>
class ArenaBuffer: public std::vector<T> {
    using Base = std::vector<T>;
public:
    template <typename... Args>
    ArenaBuffer(Args&&... args): Base(std::forward<Args>(args)...)
    {}

    ArenaBuffer(const ArenaBuffer& other): Base(other)
    {}

    ArenaBuffer(ArenaBuffer&& other): Base(std::move(other))
    {}

    ArenaBuffer& operator=(const ArenaBuffer& other) {
        Base::operator=(other);
        return *this;
    }

    ArenaBuffer& operator=(ArenaBuffer&& other) {
        Base::operator=(std::move(other));
        return *this;
    }

    void reset() {
        Base::clear();
    }

    using Base::push_back;

    void push_back(Span<const T> other) {
        Base::insert(Base::end(), other.begin(), other.end());
    }

    Span<T> span() {
        return Span<T>{Base::data(), Base::size()};
    }

    Span<const T> span() const {
        return Span<T>{Base::data(), Base::size()};
    }

    Span<T> span(size_t from) {
        return span().subspan(from);
    }

    Span<const T> span(size_t from) const {
        return span().subspan(from);
    }


    T& tail() noexcept {
        return *Base::data();
    }


    const T& tail() const noexcept {
        return *Base::data();
    }

    T& head() noexcept {
        return *(Base::data() + Base::size() - 1);
    }

    const T& head() const noexcept {
        return *(Base::data() + Base::size() - 1);
    }

    void sort() {
        std::sort(Base::begin(), Base::end());
    }
};


}
