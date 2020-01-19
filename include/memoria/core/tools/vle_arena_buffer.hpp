
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

#include <memoria/core/tools/arena_buffer.hpp>

#include <boost/iterator/iterator_facade.hpp>

namespace memoria {

template <typename BufferT, typename ElementT>
class VLEArenaBufferIterator: public boost::iterator_facade<
        VLEArenaBufferIterator<BufferT, ElementT>,
        ElementT,
        std::random_access_iterator_tag,
        ElementT
> {

    psize_t pos_;
    psize_t size_;

    using Iterator = VLEArenaBufferIterator;

    BufferT* buffer_;

public:
    VLEArenaBufferIterator(): pos_(), size_(), buffer_() {}

    VLEArenaBufferIterator(BufferT* buffer, psize_t pos, psize_t size):
        pos_(pos), size_(size), buffer_(buffer)
    {}

    psize_t size() const {return size_;}
    psize_t pos() const {return pos_;}

    bool is_end() const {return pos_ >= size_;}
    operator bool() const {return !is_end();}

private:
    friend class boost::iterator_core_access;

    ElementT dereference() const {
        return buffer_->operator[](pos_);
    }

    bool equal(const VLEArenaBufferIterator& other) const {
        return buffer_ == other.buffer_ && pos_ == other.pos_;
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

    ptrdiff_t distance_to(const VLEArenaBufferIterator& other) const
    {
        ptrdiff_t res = static_cast<ptrdiff_t>(other.pos_) - static_cast<ptrdiff_t>(pos_);
        return res;
    }
};




template <typename T, typename SizeT = size_t>
class VLEArenaBuffer {
    ArenaBuffer<SizeT> offsets_;
    ArenaBuffer<T> data_;
public:
    using Iterator = VLEArenaBufferIterator<VLEArenaBuffer, Span<T>>;
    using ConstIterator = VLEArenaBufferIterator<const VLEArenaBuffer, Span<const T>>;

    VLEArenaBuffer(): offsets_(), data_() {}

    Iterator begin() {
        return Iterator(this, 0, size());
    }

    Iterator end() {
        size_t size = this->size();
        return Iterator(this, size, size);
    }

    ConstIterator begin() const {
        return ConstIterator(this, 0, size());
    }

    ConstIterator end() const {
        size_t size = this->size();
        return ConstIterator(this, size, size);
    }

    ConstIterator cbegin() {
        return ConstIterator(this, 0, size());
    }

    ConstIterator cend() {
        size_t size = this->size();
        return ConstIterator(this, size, size);
    }

    Span<T> operator[](SizeT idx)
    {
        SizeT offset = offsets_[idx];
        SizeT length = this->length(idx);

        return Span<T>(data_.data() + offset, length);
    }

    Span<const T> operator[](SizeT idx) const
    {
        SizeT offset = offsets_[idx];
        SizeT length = this->length(idx);

        return Span<const T>(data_.data() + offset, length);
    }


    SizeT length(SizeT idx) const {
        return offsets_[idx + 1] - offsets_[idx];
    }

    SizeT length(SizeT idx, SizeT size) const {
        return offsets_[idx + size] - offsets_[idx];
    }

    SizeT offset(SizeT idx) const {
        return offsets_[idx];
    }

    const SizeT* offsets(SizeT idx) const {
        return offsets_.data() + idx;
    }

    SizeT* offsets(SizeT idx) {
        return offsets_.data() + idx;
    }

    T* data() {
        return data_.data();
    }

    const T* data() const {
        return data_.data();
    }

    T* data(SizeT start) {
        return data_.data() + offsets_[start];
    }

    const T* data(SizeT start) const {
        return data_.data() + offsets_[start];
    }

    SizeT data_size() const
    {
        return data_.size();
    }

    Span<T> data_span() {
        return data_.span();
    }

    Span<const T> data_span() const {
        return data_.span();
    }

    Span<T> head()
    {
        SizeT size   = this->size();
        SizeT offset = this->offset(size - 1);
        SizeT length = this->length(size - 1);

        return Span<T>(data_.data() + offset, length);
    }

    Span<const T> head() const
    {
        SizeT size   = this->size();
        SizeT offset = this->offset(size - 1);
        SizeT length = this->length(size - 1);

        return Span<T>(data_.data() + offset, length);
    }


    bool append(Span<const T> data)
    {
        if (MMA1_UNLIKELY(offsets_.size() == 0)) {
            offsets_.append_value(0);
        }

        offsets_.append_value(data_.size() + data.length());

        return data_.append_values(data);
    }

    void append_size(SizeT size)
    {
        if (MMA1_UNLIKELY(offsets_.size() == 0)) {
            offsets_.append_value(0);
        }

        offsets_.append_value(data_.size() + size);
        data_.add_size(size);
    }

    SizeT size() const
    {
        SizeT offsets_size = offsets_.size();
        return MMA1_LIKELY(offsets_size > 0) ? offsets_size - 1 : 0;
    }

    bool ensure(SizeT size) {
        return data_.ensure(size);
    }

    void reset() {
        data_.reset();
        offsets_.reset();
    }

    void clear() {
        data_.clear();
        offsets_.clear();
    }
};


}
