
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


#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>

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
    using Bitmap    = PkdFSSeq<typename PkdFSSeqTF<1>::Type>;

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
        return Bitmap::packed_block_size(capacity) + Array::empty_size();
    }

    size_t block_size(const MyType* other) const
    {
        return MyType::block_size(size() + other->size());
    }

    VoidResult init()
    {
        size_t capacity = 0;
        MEMORIA_TRY_VOID(Base::init(block_size(capacity), STRUCTS_NUM__));

        size_t bitmap_block_size = Bitmap::packed_block_size(capacity);

        MEMORIA_TRY(bitmap, allocate_space<Bitmap>(BITMAP, bitmap_block_size));

        MEMORIA_TRY_VOID(bitmap->init(bitmap_block_size));

        MEMORIA_TRY(array, allocate_space<Array>(ARRAY, Array::empty_size()));

        return array->init();
    }

    size_t size() const
    {
        return bitmap()->size();
    }


    const Value value(size_t block, size_t idx) const
    {
        const Bitmap* bitmap = this->bitmap();

        if (bitmap->symbol(idx) == 1)
        {
            size_t array_idx = this->array_idx(bitmap, idx);
            return array()->value(block, array_idx);
        }
        else {
            return Value();
        }
    }

    Values get_values(size_t idx) const
    {
        Values v;

        auto bitmap = this->bitmap();

        if (bitmap->symbol(idx) == 1)
        {
            auto array = this->array();
            size_t array_idx = this->array_idx(idx);

            OptionalAssignmentHelper(v, array->get_values(array_idx));
        }

        return v;
    }


    template <typename T>
    VoidResult setValues(size_t idx, const core::StaticVector<T, Columns>& values)
    {
        Bitmap* bitmap   = this->bitmap();
        Array* array     = this->array();

        if (values[0].is_set())
        {
            auto array_values  = this->array_values(values);
            size_t array_idx  = this->array_idx(idx);

            if (bitmap->symbol(idx))
            {
                MEMORIA_TRY_VOID(array->setValues(array_idx, array_values));
            }
            else {
                MEMORIA_TRY_VOID(array->insert(array_idx, array_values));

                bitmap->symbol(idx) = 1;

                MEMORIA_TRY_VOID(bitmap->reindex());
            }
        }
        else {
            size_t array_idx = this->array_idx(idx);

            if (bitmap->symbol(idx))
            {
                MEMORIA_TRY_VOID(array->remove(array_idx, array_idx + 1));

                bitmap->symbol(idx) = 0;
                MEMORIA_TRY_VOID(bitmap->reindex());
            }
            else {
                // Do nothing
            }
        }

        return VoidResult::of();
    }


    template <typename T>
    auto findGTForward(size_t block, const T& val) const
    {
        auto result = array()->find_gt(block, val);

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }

    template <typename T>
    auto findGTForward(size_t block, const Optional<T>& val) const
    {
        auto result = array()->find_gt(block, val.get());

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }

    template <typename T>
    auto findGEForward(size_t block, const T& val) const
    {
        auto result = array()->find_ge(block, val.value());

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }

    template <typename T>
    auto findForward(SearchType search_type, size_t block, const T& val) const
    {
        auto result = array()->findForward(search_type, block, val);

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }

    template <typename T>
    auto findForward(SearchType search_type, size_t block, const Optional<T>& val) const
    {
        auto result = array()->findForward(search_type, block, val.get());

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }


    template <typename T>
    auto findBackward(SearchType search_type, size_t block, const T& val) const
    {
        auto result = array()->findBackward(search_type, block, val);

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }

    template <typename T>
    auto findBackward(SearchType search_type, size_t block, const Optional<T>& val) const
    {
        auto result = array()->findBackward(search_type, block, val.get());

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }



    VoidResult reindex()
    {
        MEMORIA_TRY_VOID(bitmap()->reindex());
        return array()->reindex();
    }


    VoidResult splitTo(MyType* other, size_t idx)
    {
        Bitmap* bitmap = this->bitmap();

        size_t array_idx = this->array_idx(bitmap, idx);

        MEMORIA_TRY_VOID(bitmap->splitTo(other->bitmap(), idx));

        MEMORIA_TRY_VOID(array()->splitTo(other->array(), array_idx));

        return reindex();
    }

    VoidResult mergeWith(MyType* other) const
    {
        MEMORIA_TRY_VOID(bitmap()->mergeWith(other->bitmap()));
        return array()->mergeWith(other->array());
    }

    VoidResult removeSpace(size_t start, size_t end)
    {
        return remove(start, end);
    }

    VoidResult remove(size_t start, size_t end)
    {
        Bitmap* bitmap = this->bitmap();

        size_t array_start = array_idx(bitmap, start);
        size_t array_end = array_idx(bitmap, end);

        MEMORIA_TRY_VOID(bitmap->remove(start, end));

        return array()->remove(array_start, array_end);
    }

    template <typename T>
    VoidResult insert(size_t idx, const core::StaticVector<T, Blocks>& values)
    {
        Bitmap* bitmap  = this->bitmap();

        if (values[0].is_set())
        {
            MEMORIA_TRY_VOID(bitmap->insert(idx, 1));

            auto array_values  = this->array_values(values);
            size_t array_idx  = this->array_idx(bitmap, idx);

            Array* array = this->array();
            return array->insert(array_idx, array_values);
        }
        else {
            return bitmap->insert(idx, 0);
        }
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

    template <typename T>
    core::StaticVector<ArrayValue, Blocks> array_values(const core::StaticVector<Optional<T>, Blocks>& values)
    {
        core::StaticVector<ArrayValue, Blocks> tv;

        for (size_t b = 0;  b < Blocks; b++)
        {
            tv[b] = values[b].get();
        }

        return tv;
    }

    size_t array_idx(size_t global_idx) const
    {
        return array_idx(bitmap(), global_idx);
    }

    size_t array_idx(const Bitmap* bitmap, size_t global_idx) const
    {
        size_t rank = bitmap->rank(global_idx, 1);
        return rank;
    }


    size_t global_idx(size_t array_idx) const
    {
        return global_idx(bitmap(), array_idx);
    }

    size_t global_idx(const Bitmap* bitmap, size_t array_idx) const
    {
        auto result = bitmap->selectFw(1, array_idx + 1);
        return result.local_pos();
    }
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
