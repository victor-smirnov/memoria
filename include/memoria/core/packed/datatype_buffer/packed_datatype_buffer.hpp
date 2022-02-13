
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

#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/tools/accessors.hpp>

#include <memoria/core/packed/datatype_buffer/packed_datatype_buffer_so.hpp>

#include <memoria/api/common/packed_api.hpp>

#include <type_traits>

namespace memoria {

template <typename Types>
class PackedDataTypeBuffer;

template <typename DataType, bool Indexed, size_t Columns, DTOrdering Ordering>
struct PackedDataTypeBufferTypes {};

template <typename DataType, bool Indexed, size_t Columns, DTOrdering Ordering>
using PackedDataTypeBufferT = PackedDataTypeBuffer<PackedDataTypeBufferTypes<DataType, Indexed, Columns, Ordering>>;

template <typename DataType_, bool Indexed_, size_t Columns_, DTOrdering Ordering_>
class PackedDataTypeBuffer<PackedDataTypeBufferTypes<DataType_, Indexed_, Columns_, Ordering_>>:
    public PackedAllocator
{
    using Base = PackedAllocator;
public:
    using DataType = DataType_;
    using DataDimenstionsList = typename DataTypeTraits<DataType>::DataDimensionsList;

    static constexpr uint32_t VERSION   = 1;
    static constexpr size_t Dimensions  = ListSize<DataDimenstionsList>;
    static constexpr bool Indexed       = Indexed_;
    static constexpr size_t Columns     = Columns_;
    static constexpr size_t Indexes     = Indexed ? Columns : 0;
    static constexpr DTOrdering Ordering = Ordering_;

    static constexpr bool HasIndex =
            Ordering == DTOrdering::SUM && DataTypeTraits<DataType>::isArithmetic;


    using MyType    = PackedDataTypeBuffer;
    using ViewType  = DTTViewType<DataType>;
    using Allocator = PackedAllocator;

    using IndexType = MyType;

    enum {METADATA = 0, INDEX, LAST_HEADER_BLOCK_};

    static constexpr size_t DimensionsBlocksTotal = pdtbuf_::DimensionsListWidthBuilder<
            DataDimenstionsList,
            PackedDataTypeBuffer
    >::Value;

    using DataDimensionsStructs = typename pdtbuf_::DimensionsListBuilder<
        DataDimenstionsList,
        PackedDataTypeBuffer,

        DimensionsBlocksTotal,
        LAST_HEADER_BLOCK_
    >::Type;

    template <size_t Idx>
    using Dimension = Select<Idx, DataDimensionsStructs>;

    using ExtData       = DTTTypeDimensionsTuple<DataType>;
    using SparseObject  = PackedDataTypeBufferSO<ExtData, MyType>;

    using GrowableIOSubstream = DataTypeBuffer<DataType>;
    using IOSubstreamView     = pdtbuf_::PackedDataTypeNDBufferIO<DataType, SparseObject>;

    using Base::get;

    class Metadata {
        psize_t size_;
        psize_t data_size_[Dimensions * Columns];
    public:
        psize_t& size() {return size_;}
        const psize_t& size() const {return size_;}

        psize_t* data_size() {return data_size_;}
        psize_t offsets_size() const {return size_ + 1;}

        const psize_t* data_size() const {return data_size_;}

        psize_t& data_size(size_t column, size_t idx) {return data_size_[column * Columns + idx];}
        const psize_t& data_size(size_t column, size_t idx) const {return data_size_[column * Columns + idx];}
    };

public:

    using PackedAllocator::block_size;
    using PackedAllocator::init;
    using PackedAllocator::allocate;
    using PackedAllocator::allocate_empty;
    using PackedAllocator::allocate_array_by_size;
    using PackedAllocator::is_empty;
    using PackedAllocator::free;

    PackedDataTypeBuffer() = default;

    template <typename Fn>
    static void for_each_dimension(Fn&& fn) {
        ForEach<0, Dimensions>::process_fn(fn);
    }

    template <typename Fn>
    static VoidResult for_each_dimension_res(Fn&& fn)  {
        return ForEach<0, Dimensions>::process_res_fn(fn);
    }

    static constexpr size_t default_size(size_t available_space) {
        return empty_size();
    }

    VoidResult init_default(size_t block_size)  {
        return init();
    }

    static size_t empty_size()
    {
        size_t dimensions_size{};

        for_each_dimension([&](auto idx){
            dimensions_size += Dimension<idx>::empty_size_aligned();
        });

        return base_size(dimensions_size * Columns);
    }

    static size_t base_size(size_t dimensions_size)
    {
        size_t metadata_length = PackedAllocatable::round_up_bytes_to_alignment_blocks(sizeof(Metadata));

        return PackedAllocator::block_size(
            metadata_length + dimensions_size, DimensionsBlocksTotal * Columns + LAST_HEADER_BLOCK_
        );
    }

    static size_t packed_block_size(size_t capacity)
    {
        size_t aligned_data_size{};

        for_each_dimension([&](auto idx){
            aligned_data_size += Dimension<idx>::data_block_size(capacity);
        });

        return base_size(aligned_data_size * Columns);
    }

    VoidResult init_bs(size_t) {
        return init();
    }

    VoidResult init()
    {
        MEMORIA_TRY_VOID(init(empty_size(), DimensionsBlocksTotal * Columns + LAST_HEADER_BLOCK_));

        MEMORIA_TRY(meta, allocate<Metadata>(METADATA));
        meta->size() = 0;

        for (size_t column = 0; column < Columns; column++)
        {
            VoidResult res = for_each_dimension_res([&](auto idx)  -> VoidResult {
                using DimensionStruct = Dimension<idx>;

                MEMORIA_TRY_VOID(DimensionStruct::allocate_empty(this, column));

                DimensionStruct::init_metadata(*meta, column);
                return VoidResult::of();
            });

            MEMORIA_RETURN_IF_ERROR(res);
        }

        return VoidResult::of();
    }


    size_t block_size_for(const PackedDataTypeBuffer* other) const
    {
        auto& my_meta = metadata();
        auto& other_meta = other->metadata();

        size_t values_length{};

        for_each_dimension([&values_length, my_meta, other, other_meta, this](auto dim_idx){
            values_length += this->dimension<dim_idx>(0).joint_data_length(my_meta, other, other_meta);
        });

        return base_size(values_length);
    }

    Metadata& metadata() {
        return *get<Metadata>(METADATA);
    }

    const Metadata& metadata() const  {
        return *get<Metadata>(METADATA);
    }

    IndexType* index() {
        return get<IndexType>(INDEX);
    }

    const IndexType* index() const {
        return get<IndexType>(INDEX);
    }

    bool has_index() const {
        return !is_empty(INDEX);
    }

    VoidResult create_index()
    {
        if (has_index()) {
            MEMORIA_TRY_VOID(remove_index());
        }

        MEMORIA_TRY_VOID(allocate_empty<IndexType>(INDEX));
        return VoidResult::of();
    }

    VoidResult remove_index() {
        return free(INDEX);
    }

    template <size_t Idx>
    Dimension<Idx> dimension(size_t column)  {
        return Dimension<Idx>(this, column);
    }

    template <size_t Idx>
    const Dimension<Idx> dimension(size_t columns) const  {
        return Dimension<Idx>(const_cast<PackedDataTypeBuffer*>(this), columns);
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
        FieldFactory<psize_t>::serialize(buf, meta.data_size(), Dimensions * Columns);

        for (size_t column = 0; column < Columns; column++) {
            return for_each_dimension([&, this](auto idx){
                return this->dimension<idx>(column).serialize(meta, buf);
            });
        }
    }


    template <typename SerializationData, typename IDResolver>
    void cow_serialize(SerializationData& buf, const IDResolver* resolver) const
    {
        Base::serialize(buf);

        auto& meta = this->metadata();

        FieldFactory<psize_t>::serialize(buf, meta.size());
        FieldFactory<psize_t>::serialize(buf, meta.data_size(), Dimensions);

        for (size_t column = 0; column < Columns; column++) {
            return for_each_dimension([&, this](auto idx){
                return this->dimension<idx>(column).cow_serialize(meta, buf, resolver);
            });
        }
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        auto& meta = this->metadata();

        FieldFactory<psize_t>::deserialize(buf, meta.size());
        FieldFactory<psize_t>::deserialize(buf, meta.data_size(), Dimensions);

        for (size_t column = 0; column < Columns; column++) {
            return for_each_dimension([&, this](auto idx){
                return this->dimension<idx>(column).deserialize(meta, buf);
            });
        }
    }
};

template <typename DataType, bool Indexed, size_t Columns, DTOrdering Ordering>
struct PackedStructTraits<PackedDataTypeBuffer<PackedDataTypeBufferTypes<DataType, Indexed, Columns, Ordering>>> {
    using SearchKeyDataType = DataType;

    using AccumType = DTTViewType<SearchKeyDataType>;
    using SearchKeyType = DTTViewType<SearchKeyDataType>;

    static constexpr PackedDataTypeSize DataTypeSize = pdtbuf_::BufferSizeTypeSelector<
        typename DataTypeTraits<DataType>::DataDimensionsList
    >::DataTypeSize;

    static constexpr PkdSearchType KeySearchType = PkdSearchType::MAX;
    static constexpr size_t Blocks = Columns;
    static constexpr size_t Indexes = Indexed ? Blocks : 0;
};


}
