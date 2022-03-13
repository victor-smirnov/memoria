
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

#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/core/tools/accessors.hpp>


#include <memoria/core/packed/sseq/packed_ssrle_seq.hpp>

#include <memoria/core/packed/datatype_buffer/packed_datatype_opt_buffer_so.hpp>
#include <memoria/core/packed/datatype_buffer/packed_datatype_buffer.hpp>

#include <memoria/api/common/packed_api.hpp>

#include <type_traits>

namespace memoria {

template <typename Types>
class PackedDataTypeOptBuffer;

template <typename DataType, bool Indexed, size_t Columns, DTOrdering Ordering>
struct PackedDataTypeOptBufferTypes {};

template <typename DataType, bool Indexed, size_t Columns, DTOrdering Ordering>
using PackedDataTypeOptBufferT = PackedDataTypeOptBuffer<PackedDataTypeOptBufferTypes<DataType, Indexed, Columns, Ordering>>;


template <typename DataType, bool Indexed, size_t Columns, DTOrdering Ordering>
class PackedDataTypeOptBuffer<PackedDataTypeOptBufferTypes<DataType, Indexed, Columns, Ordering>>: public PackedAllocator {
    using Base = PackedAllocator;
public:

    using MyType = PackedDataTypeOptBuffer;

    static constexpr uint32_t VERSION = 1;

    static constexpr size_t Blocks = Columns;
    static constexpr size_t Indexes = Indexed ? Columns : 0;

    using Array     = PackedDataTypeBufferT<DataType, Indexed, Columns, Ordering>;
    using Bitmap    = PkdSSRLESeqT<2, 256, true> ;//  PkdFSSeq<typename PkdFSSeqTF<1>::Type>;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                typename Array::FieldsList,
                typename Bitmap::FieldsList,
                ConstValue<uint32_t, VERSION>
    >;

    enum {BITMAP, ARRAY, STRUCTS_NUM__};

    using IndexValue = typename Array::ViewType;
    using ArrayValue = typename Array::ViewType;

    using Value      = Optional<typename Array::ViewType>;
    using Values     = core::StaticVector<Value, Blocks>;

    using ExtData = DTTTypeDimensionsTuple<typename Array::DataType>;
    using SparseObject = PackedDataTypeOptBufferSO<ExtData, MyType>;

    using Base::block_size;

    Bitmap* bitmap() {
        return this->template get<Bitmap>(BITMAP);
    }

    const Bitmap* bitmap() const  {
        return this->template get<Bitmap>(BITMAP);
    }

    Array* array() {
        return this->template get<Array>(ARRAY);
    }

    const Array* array() const  {
        return this->template get<Array>(ARRAY);
    }

    static constexpr size_t default_size(size_t available_space)
    {
        return empty_size();
    }

    VoidResult init_default(size_t block_size)  {
        return init();
    }

    static size_t empty_size()
    {
        size_t parent_size = PackedAllocator::empty_size(STRUCTS_NUM__);
        return parent_size + Bitmap::empty_size() + Array::empty_size();
    }


    static size_t block_size(size_t capacity)
    {
        return Bitmap::block_size(capacity) + Array::empty_size();
    }

    size_t block_size(const MyType* other) const
    {
        return MyType::block_size(size() + other->size());
    }

    VoidResult init()
    {
        size_t capacity = 0;
        MEMORIA_TRY_VOID(Base::init(block_size(capacity), STRUCTS_NUM__));

        size_t bitmap_block_size = Bitmap::block_size(capacity);

        MEMORIA_TRY(bitmap, allocate_space<Bitmap>(BITMAP, bitmap_block_size));

        MEMORIA_TRY_VOID(bitmap->init(bitmap_block_size));

        MEMORIA_TRY(array, allocate_space<Array>(ARRAY, Array::empty_size()));

        return array->init();
    }

    size_t size() const
    {
        return bitmap()->size();
    }






    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        bitmap()->serialize(buf);
        return array()->serialize(buf);
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        bitmap()->deserialize(buf);
        return array()->deserialize(buf);
    }



protected:


};

template <typename DataType, bool Indexed, size_t Columns, DTOrdering Ordering>
struct PackedStructTraits<PackedDataTypeOptBuffer<PackedDataTypeOptBufferTypes<DataType, Indexed, Columns, Ordering>>> {
    using SearchKeyDataType = DataType;

    using AccumType = DTTViewType<SearchKeyDataType>;
    using SearchKeyType = Optional<DTTViewType<SearchKeyDataType>>;

    static constexpr PackedDataTypeSize DataTypeSize = PackedDataTypeSize::VARIABLE;

    static constexpr PkdSearchType KeySearchType = PkdSearchType::MAX;
    static constexpr size_t Indexes = Indexed ? Columns : 0;
};

}
