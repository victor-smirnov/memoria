
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

#include <memoria/v1/core/iovector/io_substream_col_array_fixed_size.hpp>
#include <memoria/v1/core/iovector/io_substream_col_array_fixed_size_view.hpp>

#include <memoria/v1/core/packed/array/packed_fse_array_so.hpp>



#include <type_traits>

namespace memoria {
namespace v1 {

template <
    typename V,
    int32_t Blocks_ = 1,
    int32_t Indexes_ = 0
>
struct PackedFixedSizeElementArrayTypes {
    using DataType = V;
    static constexpr int32_t Blocks = Blocks_;
    static constexpr int32_t Indexes = Indexes_;
};

template <typename Types> class PackedFixedSizeElementArray;

template <typename V, int32_t Blocks = 1, int32_t Indexes = 0>
using PkdFSEArray2T = PackedFixedSizeElementArray<PackedFixedSizeElementArrayTypes<V, Blocks, Indexes>>;





template <typename Types_>
class PackedFixedSizeElementArray: public PackedAllocator {
    using Base = PackedAllocator;
public:
    static constexpr uint32_t VERSION = 1;

    static constexpr PkdSearchType KeySearchType = PkdSearchType::MAX;
    static constexpr psize_t Blocks  = Types_::Blocks;
    static constexpr psize_t Indexes = Types_::Indexes;

    using Types = Types_;
    using MyType = PackedFixedSizeElementArray;

    using DataType = typename Types::DataType;
    using IndexDataType = DataType;

    using Value = typename DataTypeTraits<DataType>::ViewType;

    using Allocator = PackedAllocator;

    using ExtData       = typename DataTypeTraits<Value>::ExtData;
    using SparseObject  = PackedFixedSizeElementArraySO<ExtData, MyType>;


    using GrowableIOSubstream = io::IOColumnwiseFixedSizeArraySubstreamImpl<Value, Blocks>;
    using IOSubstreamView     = io::IOColumnwiseFixedSizeArraySubstreamViewImpl<Value, Blocks>;

    enum {METADATA = 0, VALUES = 1};

    using Base::get;

    class Metadata {
        psize_t size_;
    public:
        psize_t& size() {return size_;}
        const psize_t& size() const {return size_;}
    };

public:

    using PackedAllocator::block_size;
    using PackedAllocator::init;
    using PackedAllocator::allocate;
    using PackedAllocator::allocateArrayBySize;

    PackedFixedSizeElementArray() = default;

    static psize_t empty_size()
    {
        int32_t metadata_length = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        return PackedAllocator::block_size(
            metadata_length, Blocks + 1
        );
    }

    OpStatus init()
    {
        if(isFail(init(empty_size(), Blocks + 1))) {
            return OpStatus::FAIL;
        }


        Metadata* meta = allocate<Metadata>(METADATA);

        if(isFail(meta)) {
            return OpStatus::FAIL;
        }

        meta->size() = 0;

        for (psize_t block = 0; block < Blocks; block++)
        {
            if(isFail(allocateArrayBySize<Value>(block + 1, 0))) {
                return OpStatus::FAIL;
            }
        }

        return OpStatus::OK;
    }

    static psize_t packed_block_size(psize_t capacity)
    {
        int32_t metadata_length = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
        int32_t values_length   = PackedAllocatable::roundUpBytesToAlignmentBlocks(capacity * sizeof(Value));

        return PackedAllocator::block_size(
            metadata_length + (values_length) * Blocks, Blocks + 1
        );
    }

    psize_t block_size(const PackedFixedSizeElementArray* other) const
    {
        auto& my_meta = metadata();
        auto& other_meta = other->metadata();

        int32_t metadata_length = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
        int32_t values_length   = PackedAllocatable::roundUpBytesToAlignmentBlocks(
                (my_meta.size() + other_meta.size()) * sizeof(Value)
        );

        return PackedAllocator::block_size(
            metadata_length + (values_length) * Blocks, Blocks + 1
        );
    }




    Metadata& metadata() {
        return *get<Metadata>(METADATA);
    }

    const Metadata& metadata() const {
        return *get<Metadata>(METADATA);
    }

    Value* values(psize_t blk) {
        return get<Value>(VALUES + blk);
    }

    const Value* values(psize_t blk) const {
        return get<Value>(VALUES + blk);
    }

    psize_t& size() {return metadata().size();}
    const psize_t& size() const {return metadata().size();}


    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        auto& meta = this->metadata();

        FieldFactory<psize_t>::serialize(buf, meta.size());

        for (psize_t b = 0; b < Blocks; b++) {
            FieldFactory<Value>::serialize(buf, values(b), meta.size());
        }
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        auto& meta = this->metadata();

        FieldFactory<psize_t>::deserialize(buf, meta.size());

        for (psize_t b = 0; b < Blocks; b++) {
            FieldFactory<Value>::deserialize(buf, values(b), meta.size());
        }
    }
};

template <typename Types>
struct PackedStructTraits<PackedFixedSizeElementArray<Types>> {
    using SearchKeyDataType = typename Types::DataType;

    static constexpr PackedDataTypeSize DataTypeSize = PackedDataTypeSize::FIXED;
    static constexpr PkdSearchType KeySearchType = PkdSearchType::MAX;
    static constexpr int32_t Blocks = PackedFixedSizeElementArray<Types>::Blocks;
    static constexpr int32_t Indexes = PackedFixedSizeElementArray<Types>::Indexes;
};

}}
