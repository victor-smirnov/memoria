
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

#include <memoria/v1/core/iovector/io_substream_array_vlen_base.hpp>
#include <memoria/v1/core/iovector/io_substream_col_array_fixed_size_view.hpp>

#include <memoria/v1/core/packed/array/packed_vle_array_so.hpp>

#include <memoria/v1/api/common/packed_api.hpp>

#include <type_traits>

namespace memoria {
namespace v1 {

template <
    typename V,
    int32_t Blocks_ = 1,
    int32_t Indexes_ = 0
>
struct PackedVLenElementArrayTypes {
    using Value = V;
    static constexpr int32_t Blocks = Blocks_;
    static constexpr int32_t Indexes = Indexes_;
};

template <typename Types> class PackedVLenElementArray;

template <typename V, int32_t Blocks = 1, int32_t Indexes = 0>
using PkdVLEArrayT = PackedVLenElementArray<PackedVLenElementArrayTypes<V, Blocks, Indexes>>;





template <typename Types_>
class PackedVLenElementArray: public PackedAllocator {
    using Base = PackedAllocator;
public:
    static constexpr uint32_t VERSION = 1;

    static constexpr PkdSearchType KeySearchType = PkdSearchType::MAX;
    static constexpr psize_t Blocks             = Types_::Blocks;
    static constexpr psize_t Indexes            = Types_::Indexes;

    static constexpr psize_t SegmentsPerBlock   = 2;
    static constexpr psize_t OffsetsBlk         = 0;
    static constexpr psize_t DataBlk            = 1;


    using Types     = Types_;
    using MyType    = PackedVLenElementArray;
    using Tag       = PkdVLEArrayTag;

    using DataType      = typename Types::Value;
    using IndexDataType = typename Types::Value;

    using ViewType      = typename DataTypeTraits<DataType>::ViewType;
    using ValueType     = typename DataTypeTraits<DataType>::ValueType;
    using DataSizeType  = psize_t;
    using DataAtomType  = typename DataTypeTraits<DataType>::AtomType;

    using AccumValue = ViewType;

    using IndexValue = ValueType;

    using Allocator = PackedAllocator;

    using ExtData = EmptyType;
    using SparseObject = PackedVLenElementArraySO<ExtData, MyType>;

    using Accessor = PkdDataTypeAccessor<DataType, Tag, SparseObject>;

    using GrowableIOSubstream = io::IOVLen1DArraySubstreamImpl<DataType>;
    using IOSubstreamView     = io::IOVLen1DArraySubstreamViewImpl<DataType, SparseObject>;

    enum {METADATA = 0, DATA = 1};

    using Base::get;

    class Metadata {
        psize_t size_;
        psize_t data_size_[Blocks];
        psize_t stride_log2_[Blocks];
    public:
        psize_t& size() {return size_;}
        const psize_t& size() const {return size_;}

        psize_t& data_size(psize_t blk) {return data_size_[blk];}
        const psize_t& data_size(psize_t blk) const {return data_size_[blk];}

        psize_t& stride_log2(psize_t blk) {return stride_log2_[blk];}
        const psize_t& stride_log2(psize_t blk) const {return stride_log2_[blk];}

        psize_t offsets_size() const {return size_ + 1;}
    };

public:

    using PackedAllocator::block_size;
    using PackedAllocator::init;
    using PackedAllocator::allocate;
    using PackedAllocator::allocateArrayBySize;

    PackedVLenElementArray() = default;

    static psize_t empty_size()
    {
        psize_t metadata_length = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
        psize_t offsets_length  = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(psize_t) * 1);

        return PackedAllocator::block_size(
            metadata_length + offsets_length * Blocks, Blocks * SegmentsPerBlock + 1
        );
    }

    OpStatus init()
    {
        if(isFail(init(empty_size(), Blocks * SegmentsPerBlock + 1))) {
            return OpStatus::FAIL;
        }


        Metadata* meta = allocate<Metadata>(METADATA);

        if(isFail(meta)) {
            return OpStatus::FAIL;
        }

        meta->size() = 0;

        for (psize_t block = 0; block < Blocks; block++)
        {
            meta->data_size(block)      = 0;
            meta->stride_log2(block)    = 2; // default value

            DataSizeType* offsets = allocateArrayBySize<DataSizeType>(block * SegmentsPerBlock + OffsetsBlk + 1, 1);

            if(isFail(offsets)) {
                return OpStatus::FAIL;
            }

            offsets[0] = 0;

            if(isFail(allocateArrayBySize<DataAtomType>(block * SegmentsPerBlock + DataBlk + 1, 0))) {
                return OpStatus::FAIL;
            }
        }

        return OpStatus::OK;
    }


    psize_t block_size(const PackedVLenElementArray* other) const
    {
        auto& my_meta = metadata();
        auto& other_meta = other->metadata();

        psize_t metadata_length = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        psize_t data_length{};

        for (psize_t c = 0; c < Blocks; c++)
        {
            data_length += PackedAllocatable::roundUpBytesToAlignmentBlocks(
                    (my_meta.data_size(c) + other_meta.data_size(c)) * sizeof(DataAtomType)
            );
        }

        psize_t offsets_length = PackedAllocatable::roundUpBytesToAlignmentBlocks(
                (my_meta.size() + other_meta.size() + 1) * sizeof(DataSizeType)
        ) * Blocks;

        return PackedAllocator::block_size(
            metadata_length + data_length + offsets_length, Blocks * SegmentsPerBlock + 1
        );
    }


    Metadata& metadata() {
        return *get<Metadata>(METADATA);
    }

    const Metadata& metadata() const {
        return *get<Metadata>(METADATA);
    }

    DataSizeType* offsets(psize_t blk) {
        return get<DataSizeType>(DATA + blk * SegmentsPerBlock);
    }

    const DataSizeType* offsets(psize_t blk) const {
        return get<DataSizeType>(DATA + blk * SegmentsPerBlock);
    }

    DataAtomType* data(psize_t blk) {
        return get<DataAtomType>(DATA + blk * SegmentsPerBlock + 1);
    }

    const DataAtomType* data(psize_t blk) const {
        return get<DataAtomType>(DATA + blk * SegmentsPerBlock + 1);
    }

    psize_t length(psize_t blk, psize_t row) const
    {
        const DataSizeType* ll = offsets(blk) + row;
        return ll[1] - ll[0];
    }

    psize_t& size() {return metadata().size();}
    const psize_t& size() const {return metadata().size();}

    psize_t& data_size(psize_t block) {return metadata().data_size(block);}
    const psize_t& max_size(psize_t block) const {return metadata().data_size(block);}

    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        auto& meta = this->metadata();

        FieldFactory<psize_t>::serialize(buf, meta.size());


        for (psize_t b = 0; b < Blocks; b++)
        {
            FieldFactory<psize_t>::serialize(buf, meta.data_size(b));
            FieldFactory<psize_t>::serialize(buf, meta.stride_log2(b));

            FieldFactory<DataSizeType>::serialize(buf, offsets(b), meta.offsets_size());
            FieldFactory<DataAtomType>::serialize(buf, data(b), meta.data_size(b));
        }
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        auto& meta = this->metadata();

        FieldFactory<psize_t>::deserialize(buf, meta.size());

        for (psize_t b = 0; b < Blocks; b++)
        {
            FieldFactory<psize_t>::deserialize(buf, meta.data_size(b));
            FieldFactory<psize_t>::deserialize(buf, meta.stride_log2(b));

            FieldFactory<DataSizeType>::deserialize(buf, offsets(b), meta.offsets_size());
            FieldFactory<DataAtomType>::deserialize(buf, data(b), meta.data_size(b));
        }
    }
};

template <typename Types>
struct PackedStructTraits<PackedVLenElementArray<Types>> {
    using SearchKeyDataType = typename Types::Value;

    using AccumType = typename DataTypeTraits<SearchKeyDataType>::ViewType;
    using SearchKeyType = typename DataTypeTraits<SearchKeyDataType>::ViewType;

    static constexpr PackedDataTypeSize DataTypeSize = PackedDataTypeSize::VARIABLE;

    static constexpr PkdSearchType KeySearchType = PkdSearchType::MAX;
    static constexpr int32_t Indexes = PackedVLenElementArray<Types>::Indexes;
};







}}
