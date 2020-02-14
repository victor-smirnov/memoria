
// Copyright 2019 Victor Smirnov
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

#include <boost/iterator/iterator_facade.hpp>

#include <iterator>

namespace memoria {

template <typename AccessorType>
class PkdRandomAccessIterator: public boost::iterator_facade<
        PkdRandomAccessIterator<AccessorType>,
        const typename AccessorType::ViewType,
        std::random_access_iterator_tag,
        const typename AccessorType::ViewType
> {
    using ViewType = typename AccessorType::ViewType;

    psize_t pos_;
    psize_t size_;
    AccessorType accessor_;

    using Iterator = PkdRandomAccessIterator;

public:
    PkdRandomAccessIterator() noexcept: pos_(), size_(), accessor_() {}

    PkdRandomAccessIterator(AccessorType accessor, psize_t pos, psize_t size) noexcept:
        pos_(pos), size_(size), accessor_(accessor)
    {}

    psize_t size() const noexcept {return size_;}
    psize_t pos() const noexcept {return pos_;}

    bool is_end() const noexcept {return pos_ >= size_;}
    operator bool() const noexcept {return !is_end();}

private:
    friend class boost::iterator_core_access;

    ViewType dereference() const noexcept {
        return accessor_.get(pos_);
    }

    bool equal(const PkdRandomAccessIterator& other) const noexcept {
        return accessor_ == other.accessor_ && pos_ == other.pos_;
    }

    void increment() noexcept {
        pos_ += 1;
    }

    void decrement() noexcept {
        pos_ -= 1;
    }

    void advance(int64_t n) noexcept {
        pos_ += n;
    }

    ptrdiff_t distance_to(const PkdRandomAccessIterator& other) const noexcept
    {
        ptrdiff_t res = static_cast<ptrdiff_t>(other.pos_) - static_cast<ptrdiff_t>(pos_);
        return res;
    }
};

}
