
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
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/types/typehash.hpp>

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/tools/arena_buffer.hpp>

#include <memoria/core/tools/vle_arena_buffer.hpp>

#include <memoria/core/iovector/io_substream_base.hpp>

#include <boost/any.hpp>

#include <type_traits>

namespace memoria {

template <bool FixedSize, typename TypeDimensionsList, typename DataDimensionsList>
class SparseObjectAdapterDescriptor {};

template <
        typename DataType,
        typename SelectorTag = SparseObjectAdapterDescriptor<
            DTTIs1DFixedSize<DataType>,
            typename DataTypeTraits<DataType>::TypeDimensionsList,
            typename DataTypeTraits<DataType>::DataDimensionsList
        >
>
class DataTypeBuffer;


namespace detail {

    template <typename T, typename SizeT = size_t>
    class DataTypeBufferDimension;

    template <typename T, typename SizeT>
    class DataTypeBufferDimension<const T*, SizeT> {
        using AtomType = std::remove_const_t<T>;
        ArenaBuffer<AtomType> buffer_;
        SizeT offset_{};
    public:
        ArenaBuffer<AtomType>& buffer() {return buffer_;}
        const ArenaBuffer<AtomType>& buffer() const {return buffer_;}

        AtomType* data() {
            return buffer_.data();
        }

        const AtomType* data() const {
            return buffer_.data();
        }

        SizeT size() const {
            return buffer_.size();
        }

        bool ensure(SizeT size) {
            return buffer_.ensure(size);
        }

        template <typename TT>
        bool emplace_back_nockeck_tool(TT) {
            return false;
        }

        void reset_iterator() {
            offset_ = 0;
        }

        void reset() {
            buffer_.reset();
        }

        void clear() {
            buffer_.clear();
        }

        SizeT data_length(SizeT start, SizeT size) const {
            return size * sizeof(AtomType);
        }

        const AtomType* data(SizeT start) const {
            return buffer_.data() + start;
        }

        void set_invalidation_listener(LifetimeInvalidationListener listener) const noexcept {
            buffer_.set_invalidation_listener(listener);
        }
    };

    template <typename T, typename SizeT>
    class DataTypeBufferDimension<Span<T>, SizeT> {
        using AtomType  = std::remove_const_t<T>;
        using ArenaType = VLEArenaBuffer<AtomType, SizeT>;

        ArenaType buffer_;
        typename ArenaType::Iterator iterator_;

    public:
        VLEArenaBuffer<AtomType>& buffer() {return buffer_;}
        const VLEArenaBuffer<AtomType>& buffer() const {return buffer_;}

        AtomType* data() {return buffer_.data();}
        const AtomType* data() const {return buffer_.data();}

        SizeT size() const {
            return buffer_.data_size();
        }

        bool ensure(SizeT size) {
            return buffer_.ensure(size);
        }


        bool emplace_back_nockeck_tool(Span<const T>& span)
        {
            bool resized = buffer_.append(span);

            span = buffer_.head();

            return resized;
        }

        void reset_iterator() {
            iterator_ = buffer_.begin();
        }

        void next(Span<const T>& data)
        {
            data = *iterator_;
            ++iterator_;
        }

        void append_top(const Span<const T> data)
        {
            buffer_.append_size(data.size());
        }

        void reset() {
            buffer_.reset();
        }

        void clear() {
            buffer_.clear();
        }

        SizeT data_length(SizeT start, SizeT size) const {
            return buffer_.length(start, size);
        }

        const AtomType* data(SizeT start) const {
            return buffer_.data(start);
        }

        const SizeT* offsets(SizeT start) const {
            return buffer_.offsets(start);
        }

        void set_invalidation_listener(LifetimeInvalidationListener listener) const noexcept {
            buffer_.set_invalidation_listener(listener);
        }
    };
}

}
