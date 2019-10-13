
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

#include <memoria/v1/core/packed/datatype_buffer/packed_datatype_buffer_so.hpp>

#include <memoria/v1/api/common/packed_api.hpp>

#include <memoria/v1/core/packed/array/packed_fse_array.hpp>
#include <memoria/v1/core/packed/array/packed_vle_array.hpp>

#include <type_traits>

namespace memoria {
namespace v1 {

template <typename Types>
class PackedDataTypeBuffer;

template <typename DataType, bool Indexed>
struct PackedDataTypeBufferTypes {};

template <typename DataType, bool Indexed>
using PackedDataTypeBufferT = PackedDataTypeBuffer<PackedDataTypeBufferTypes<DataType, Indexed>>;

template <typename DataType_, bool Indexed>
class PackedDataTypeBuffer<PackedDataTypeBufferTypes<DataType_, Indexed>>:
    public PackedAllocator
{
    using Base = PackedAllocator;
public:
    using DataType = DataType_;
    using DataDimenstionsList = typename DataTypeTraits<DataType>::DataDimensionsList;

    static constexpr uint32_t VERSION   = 1;
    static constexpr int32_t Dimensions = ListSize<DataDimenstionsList>;
    static constexpr int32_t Indexes    = Indexed ? 1 : 0;

    using MyType    = PackedDataTypeBuffer;
    using ViewType  = DTTViewType<DataType>;
    using Allocator = PackedAllocator;

    using DataDimensionsStructs = typename pdtbuf_::DimensionsListBuilder<
        DataDimenstionsList,
        PackedDataTypeBuffer,
        1
    >::Type;

    template <int32_t Idx>
    using Dimension = Select<Idx, DataDimensionsStructs>;

    using ExtData       = DTTTypeDimensionsTuple<DataType>;
    using SparseObject  = PackedDataTypeBufferSO<ExtData, MyType>;

    using GrowableIOSubstream = DataTypeBuffer<DataType>;
    using IOSubstreamView     = pdtbuf_::PackedDataTypeBufferIO<DataType, SparseObject>;

    enum {METADATA = 0};

    using Base::get;

    class Metadata {
        psize_t size_;
        psize_t data_size_[Dimensions];
    public:
        psize_t& size() {return size_;}
        const psize_t& size() const {return size_;}

        psize_t* data_size() {return data_size_;}
        psize_t offsets_size() const {return size_ + 1;}

        const psize_t* data_size() const {return data_size_;}

        psize_t& data_size(psize_t idx) {return data_size_[idx];}
        const psize_t& data_size(psize_t idx) const {return data_size_[idx];}
    };

public:

    using PackedAllocator::block_size;
    using PackedAllocator::init;
    using PackedAllocator::allocate;
    using PackedAllocator::allocateEmpty;
    using PackedAllocator::allocateArrayBySize;

    PackedDataTypeBuffer() = default;

    template <typename Fn>
    static void for_each_dimension(Fn&& fn) {
        ForEach<0, Dimensions>::process_fn(fn);
    }

    static psize_t empty_size()
    {
        psize_t dimensions_size{};

        for_each_dimension([&](auto idx){
            dimensions_size += Dimension<idx>::empty_size_aligned();
        });

        return base_size(dimensions_size);
    }

    static psize_t base_size(psize_t dimensions_size)
    {
        psize_t metadata_length = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        return PackedAllocator::block_size(
            metadata_length + dimensions_size, Dimensions + 1
        );
    }

    static psize_t block_size(psize_t capacity)
    {
        psize_t aligned_data_size{};

        for_each_dimension([&](auto idx){
            aligned_data_size += Dimension<idx>::data_block_size(capacity);
        });

        return base_size(aligned_data_size);
    }

    OpStatus init()
    {
        if(isFail(init(empty_size(), Dimensions + 1))) {
            return OpStatus::FAIL;
        }

        Metadata* meta = allocate<Metadata>(METADATA);

        if(isFail(meta)) {
            return OpStatus::FAIL;
        }

        meta->size() = 0;

        OpStatus status = OpStatus::OK;

        for_each_dimension([&](auto idx){
            using DimensionStruct = Dimension<idx>;
            if (isFail(status))
            {
                if (isFail(DimensionStruct::allocateEmpty(this))) {
                    status = OpStatus::FAIL;
                }
            }

            DimensionStruct::init_metadata(*meta);
        });

        return status;
    }

    static psize_t packed_block_size(psize_t capacity)
    {
        return block_size(capacity);
    }

    psize_t block_size(const PackedDataTypeBuffer* other) const
    {
        auto& my_meta = metadata();
        auto& other_meta = other->metadata();

        psize_t values_length{};

        for_each_dimension([&](auto dim_idx){
            values_length += dimension<dim_idx>().joint_data_length(my_meta, other, other_meta);
        });

        return base_size(values_length);
    }

    Metadata& metadata() {
        return *get<Metadata>(METADATA);
    }

    const Metadata& metadata() const {
        return *get<Metadata>(METADATA);
    }

    template <int32_t Idx>
    Dimension<Idx> dimension() {
        return Dimension<Idx>(this);
    }

    template <int32_t Idx>
    const Dimension<Idx> dimension() const {
        return Dimension<Idx>(const_cast<PackedDataTypeBuffer*>(this));
    }

    psize_t& size() {
        return metadata().size();
    }

    const psize_t& size() const {
        return metadata().size();
    }


    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        auto& meta = this->metadata();

        FieldFactory<psize_t>::serialize(buf, meta.size());
        FieldFactory<psize_t>::serialize(buf, meta.data_size(), Dimensions);


        for_each_dimension([&, this](auto idx){
            this->dimension<idx>().serialize(meta, buf);
        });
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        auto& meta = this->metadata();

        FieldFactory<psize_t>::deserialize(buf, meta.size());
        FieldFactory<psize_t>::deserialize(buf, meta.data_size(), Dimensions);

        for_each_dimension([&, this](auto idx){
            this->dimension<idx>().deserialize(meta, buf);
        });
    }
};

template <typename DataType, bool Indexed>
struct PackedStructTraits<PackedDataTypeBuffer<PackedDataTypeBufferTypes<DataType, Indexed>>> {
    using SearchKeyDataType = DataType;

    using AccumType = DTTViewType<SearchKeyDataType>;
    using SearchKeyType = DTTViewType<SearchKeyDataType>;

    static constexpr PackedDataTypeSize DataTypeSize = pdtbuf_::BufferSizeTypeSelector<
        typename DataTypeTraits<DataType>::DataDimensionsList
    >::DataTypeSize;

    static constexpr PkdSearchType KeySearchType = PkdSearchType::MAX;
    static constexpr int32_t Blocks = 1;
    static constexpr int32_t Indexes = Indexed ? 1 : 0;
};


}}
