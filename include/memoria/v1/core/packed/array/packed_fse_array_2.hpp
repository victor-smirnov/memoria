
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

#include <memoria/v1/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/v1/core/tools/accessors.hpp>

#include <memoria/v1/core/iovector/io_substream_row_array_fixed_size.hpp>
#include <memoria/v1/core/iovector/io_substream_row_array_fixed_size_view.hpp>

#include <memoria/v1/core/packed/array/packed_fse_array_2_so.hpp>

#include <type_traits>

namespace memoria {
namespace v1 {

template <
    typename V,
    int32_t Blocks_ = 1
>
struct PackedFixedSizeElementArrayTypes {
    using Value = V;
    static constexpr int32_t Blocks = Blocks_;
};

template <typename Types> class PackedFixedSizeElementArray;

template <typename V, int32_t Blocks = 1>
using PkdFSEArrayT = PackedFixedSizeElementArray<PackedFixedSizeElementArrayTypes<V, Blocks>>;





template <typename Types_>
class PackedFixedSizeElementArray: public PackedAllocator {
    using Base = PackedAllocator;
public:
    static constexpr uint32_t VERSION = 1;

    static constexpr PkdSearchType KeySearchType = PkdSearchType::SUM;
    static constexpr psize_t Blocks = 1;

    using Types = Types_;
    using MyType = PackedFixedSizeElementArray;

    using Allocator = PackedAllocator;

    using ExtData = EmptyType;
    using SparseObject = PackedFixedSizeElementArraySO<ExtData, MyType>;

    using Value = typename Types::Value;

    enum {METADATA = 0, VALUES = 1};

    using Base::get;

    class Metadata {
        psize_t size_;
        psize_t max_size_;
    public:
        psize_t& size() {return size_;}
        const psize_t& size() const {return size_;}

        psize_t& max_size() {return size_;}
        const psize_t& max_size() const {return size_;}

        psize_t capacity() const {
            return max_size_ - size_;
        }
    };

public:
    PackedFixedSizeElementArray() = default;

    Metadata& metadata() {
        return *get<Metadata>(METADATA);
    }

    const Metadata& metadata() {
        return *get<Metadata>(METADATA);
    }

    Value* values(psize_t blk) {
        return *get<Value>(VALUES);
    }

    const Value* values(psize_t blk) const {
        return *get<Value>(VALUES);
    }

    int32_t& size() {return metadata().size();}
    const int32_t& size() const {return metadata().size();}

    int32_t& max_size() {return metadata().max_size();}
    const int32_t& max_size() const {return metadata().max_size();}

    int32_t capacity() const {return metadata().capacity();}





    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        auto& meta = this->metadata();

        FieldFactory<int32_t>::serialize(buf, meta.size());
        FieldFactory<int32_t>::serialize(buf, meta.max_size());

        for (psize_t b = 0; b < Blocks; b++) {
            FieldFactory<Value>::serialize(buf, values(b), meta.size());
        }
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        auto& meta = this->metadata();

        FieldFactory<int32_t>::deserialize(buf, meta.size());
        FieldFactory<int32_t>::deserialize(buf, meta.max_size());

        for (psize_t b = 0; b < Blocks; b++) {
            FieldFactory<Value>::deserialize(buf, values(b), meta.size());
        }
    }
};

template <typename Types>
struct PackedStructTraits<PackedFixedSizeElementArray<Types>> {

};

template <typename ExtData, typename PkdStruct>
struct PackedStructTraits<PackedFixedSizeElementArraySO<ExtData, PkdStruct>>: PackedStructTraits<PkdStruct> {

};




template <typename Types>
struct PkdStructSizeType<PackedFixedSizeElementArray<Types>> {
    static const PackedSizeType Value = PackedSizeType::FIXED;
};

template <typename T>
struct StructSizeProvider<PackedFixedSizeElementArray<T>> {
    static const int32_t Value = 0;
};


template <typename T>
struct PkdSearchKeyTypeProvider<PackedFixedSizeElementArray<T>> {
    using Type = typename PackedFSEArray<T>::Value;
};



}}
