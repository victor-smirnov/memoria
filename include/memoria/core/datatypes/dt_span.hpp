
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/memory/shared_ptr.hpp>
#include <memoria/core/memory/object_pool.hpp>


#include <memoria/core/tools/result.hpp>


#include <boost/iterator/iterator_facade.hpp>

namespace memoria {

namespace detail {

template <typename ArrayT>
struct SpanIteratorAccessor {
    using element_type = typename ArrayT::element_type;

    mutable ArrayT array;

    SpanIteratorAccessor(ArrayT array):
        array(array)
    {}


    auto get(uint64_t idx) const {
        return array.get(idx);
    }

    bool operator==(const SpanIteratorAccessor& other) const {
        return true;//array.equals(other.array);
    }
};

}



template <typename AccessorType>
class RandomAccessSpanIterator: public boost::iterator_facade<
        RandomAccessSpanIterator<AccessorType>,
        const typename AccessorType::element_type,
        std::random_access_iterator_tag,
        const typename AccessorType::element_type
> {
    using ViewType = typename AccessorType::element_type;

    size_t pos_;
    size_t size_;
    AccessorType accessor_;

    using Iterator = RandomAccessSpanIterator;

public:
    RandomAccessSpanIterator() : pos_(), size_(), accessor_() {}

    RandomAccessSpanIterator(AccessorType accessor, size_t pos, size_t size) :
        pos_(pos), size_(size), accessor_(accessor)
    {}

    AccessorType& accessor() {return accessor_;}
    const AccessorType& accessor() const {return accessor_;}

    size_t size() const noexcept {return size_;}
    size_t pos() const noexcept {return pos_;}

    bool is_end() const noexcept {return pos_ >= size_;}
    operator bool() const noexcept {return !is_end();}

private:
    friend class boost::iterator_core_access;

    ViewType dereference() const  {
        return accessor_.get(pos_);
    }

    bool equal(const RandomAccessSpanIterator& other) const  {
        return accessor_ == other.accessor_ && pos_ == other.pos_;
    }

    void increment() {
        pos_ += 1;
    }

    void decrement() {
        pos_ -= 1;
    }

    void advance(int64_t n)  {
        pos_ += n;
    }

    ptrdiff_t distance_to(const RandomAccessSpanIterator& other) const
    {
        ptrdiff_t res = static_cast<ptrdiff_t>(other.pos_) - static_cast<ptrdiff_t>(pos_);
        return res;
    }
};


template <typename T, typename StorageT>
class OSpan {
    using StorageSpan = Span<const StorageT>;

    mutable LWMemHolder* mem_holder_;
    Span<const StorageT> span_;

    using Accessor = detail::SpanIteratorAccessor<OSpan>;

public:
    using element_type = T;
    using iterator = RandomAccessSpanIterator<Accessor>;
    using const_iterator = iterator;


    OSpan():
        mem_holder_(), span_()
    {}

    OSpan(LWMemHolder* mem_holder, StorageSpan span):
        mem_holder_(mem_holder),
        span_(span)
    {
        if (mem_holder_) {
            mem_holder_->ref_copy();
        }
    }

    OSpan(const OSpan& other):
        mem_holder_(other.mem_holder_),
        span_(other.span_)
    {
        if (mem_holder_) {
            mem_holder_->ref_copy();
        }
    }

    OSpan(OSpan&& other):
        mem_holder_(other.mem_holder_),
        span_(other.span_)
    {
        other.mem_holder_ = nullptr;
    }

    ~OSpan() noexcept {
        if (mem_holder_) {
            mem_holder_->unref();
        }
    }

    OSpan& operator=(const OSpan& other)
    {
        if (&other != this)
        {
            if (mem_holder_) {
                mem_holder_->unref();
            }

            span_ = other.span_;
            mem_holder_ = other.mem_holder_;

            if (mem_holder_) {
                mem_holder_->ref_copy();
            }
        }

        return *this;
    }

    OSpan& operator=(OSpan&& other)
    {
        if (&other != this)
        {
            if (mem_holder_) {
                mem_holder_->unref();
            }

            span_ = other.span_;
            mem_holder_ = other.mem_holder_;

            other.mem_holder_ = nullptr;
        }

        return *this;
    }

    iterator begin() const {
        return iterator(*this, 0, span_.length());
    }

    iterator end() const {
        return iterator(*this, span_.length(), span_.length());
    }

    const_iterator cbegin() const {
        return const_iterator(*this, 0, span_.length());
    }

    const_iterator cend() const {
        return const_iterator(*this, span_.length(), span_.length());
    }

    bool operator==(const OSpan& other) const noexcept {
        return span_.data() == other.span_.data() && span_.lenght() == other.span.length();
    }

    T operator[](size_t idx) const {
        return get(idx);
    }

    T get(size_t idx) const
    {
        if (idx < span_.length()) {
            return T(mem_holder_, span_[idx]);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Range check in OSpan {}::{}", idx, span_.length()).do_throw();
        }
    }

    size_t size() const noexcept {
        return span_.length();
    }

    bool is_null() const {
        return mem_holder_ == nullptr;
    }

    bool is_not_null() const {
        return mem_holder_ != nullptr;
    }

    OSpan subspan(size_t start) const
    {
        if (start < span_.length()) {
            return OSpan(mem_holder_, span_.subspan(start));
        }
        else {
            return OSpan(mem_holder_, span_.subspan(span_.length()));
        }
    }

    OSpan subspan(size_t start, size_t size) const
    {
        if (start < span_.length())
        {
            size_t sz = (start + size) <= size ? size : (span_.length() - start);
            return OSpan(mem_holder_, span_.subspan(start, sz));
        }
        else {
            return OSpan(mem_holder_, span_.subspan(span_.length(), 0));
        }
    }

    OSpan first(size_t size) const
    {
        if (size < span_.length())
        {
            return OSpan(mem_holder_, span_.subspan(0, size));
        }
        else {
            return OSpan(mem_holder_, span_);
        }
    }

    // FIXME. This is needed for SWMRStore, should be
    // removed after refactoring
    Span<const StorageT> raw_span() const {
        return Span<const StorageT>(span_.data(), span_.length());
    }
};


}
