
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


#include <memoria/v1/core/types.hpp>

#include <boost/iterator/iterator_facade.hpp>

#include <iterator>

namespace memoria {
namespace v1 {

template <typename AccessorType>
class PkdRandomAccessIterator: boost::iterator_facade<
        PkdRandomAccessIterator<AccessorType>,
        const typename AccessorType::Value,
        std::random_access_iterator_tag,
        const typename AccessorType::Value
> {
    using Value = typename AccessorType::Value;

    psize_t pos_;
    psize_t size_;
    AccessorType accessor_;

    using Iterator = PkdRandomAccessIterator;

public:
    PkdRandomAccessIterator(): pos_(), size_(), accessor_() {}

    PkdRandomAccessIterator(AccessorType accessor, psize_t pos, psize_t size):
        pos_(pos), size_(size), accessor_(accessor)
    {}

    psize_t size() const {return size_;}
    psize_t pos() const {return pos_;}

    bool is_end() const {return pos_ >= size_;}
    operator bool() const {return !is_end();}

private:
    friend class boost::iterator_core_access;

    Value dereference() const {
        return accessor_.get(pos_);
    }

    bool equal(const PkdRandomAccessIterator& other) const {
        return accessor_ == other.accessor_ && pos_ == other.pos_;
    }

    void increment() {
        pos_ += 1;
    }

    void decrement() {
        pos_ -= 1;
    }

    void advance(ssize_t n) {
        pos_ += n;
    }

    ptrdiff_t distance_to(const PkdRandomAccessIterator& other) const
    {
        return other.pos_ - pos_;
    }
};

}}
