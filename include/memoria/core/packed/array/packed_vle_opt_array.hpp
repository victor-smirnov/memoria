
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

#include <memoria/core/packed/array/packed_vle_array.hpp>
#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>

#include <memoria/core/packed/array/packed_vle_opt_array_so.hpp>
#include <memoria/core/packed/array/packed_vle_array.hpp>

#include <memoria/api/common/packed_api.hpp>

#include <type_traits>

namespace memoria {

template <
    typename V,
    int32_t Blocks_ = 1,
    int32_t Indexes_ = 0
>
struct PackedVLenElementOptArrayTypes {
    using Value = V;
    static constexpr int32_t Blocks = Blocks_;
    static constexpr int32_t Indexes = Indexes_;
};

template <typename Types> class PackedVLenElementOptArray;

template <typename V, int32_t Blocks = 1, int32_t Indexes = 0>
using PkdVLEOptArrayT = PackedVLenElementOptArray<PackedVLenElementOptArrayTypes<V, Blocks, Indexes>>;





template <typename Types_>
class PackedVLenElementOptArray: public PackedAllocator {
    using Base = PackedAllocator;
public:
    using MyType = PackedVLenElementOptArray;

    static constexpr uint32_t VERSION = 1;
    static constexpr int32_t Blocks = Types_::Blocks;
    static constexpr int32_t Indexes = Types_::Indexes;

    using Array     = PackedVLenElementArray<Types_>;
    using Bitmap    = PkdFSSeq<typename PkdFSSeqTF<1>::Type>;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                typename Array::FieldsList,
                typename Bitmap::FieldsList,
                ConstValue<uint32_t, VERSION>
    >;



    enum {BITMAP, ARRAY, STRUCTS_NUM__};


    using IndexValue = typename Array::ViewType;

    using ArrayValue  = typename Array::ViewType;

    using Value      = Optional<typename Array::ViewType>;
    using Values     = core::StaticVector<Value, Blocks>;

    using ExtData = DTTTypeDimensionsTuple<typename Array::DataType>;
    using SparseObject = PackedVLenElementOptArraySO<ExtData, MyType>;

    using Base::block_size;

    Bitmap* bitmap() {
        return this->template get<Bitmap>(BITMAP);
    }

    const Bitmap* bitmap() const {
        return this->template get<Bitmap>(BITMAP);
    }

    Array* array() {
        return this->template get<Array>(ARRAY);
    }

    const Array* array() const {
        return this->template get<Array>(ARRAY);
    }


    static int32_t empty_size()
    {
        int32_t parent_size = PackedAllocator::empty_size(STRUCTS_NUM__);
        return parent_size + Bitmap::empty_size() + Array::empty_size();
    }


    static int32_t block_size(int32_t capacity)
    {
        return Bitmap::packed_block_size(capacity) + Array::empty_size();
    }

    int32_t block_size(const MyType* other) const
    {
        return MyType::block_size(size() + other->size());
    }

    VoidResult init() noexcept
    {
        int32_t capacity = 0;
        MEMORIA_TRY_VOID(Base::init(block_size(capacity), STRUCTS_NUM__));

        int32_t bitmap_block_size = Bitmap::packed_block_size(capacity);

        MEMORIA_TRY(bitmap, allocateSpace<Bitmap>(BITMAP, bitmap_block_size));

        MEMORIA_TRY_VOID(bitmap->init(bitmap_block_size));

        MEMORIA_TRY(array, allocateSpace<Array>(ARRAY, Array::empty_size()));

        return array->init();
    }

    int32_t size() const
    {
        return bitmap()->size();
    }


    template <typename T>
    void max(core::StaticVector<T, Blocks>& accum) const
    {
        const Array* array = this->array();

        int32_t size = array->size();

        if (size > 0)
        {
            for (int32_t block = 0; block < Blocks; block++)
            {
                accum[block] = array->value(block, size - 1);
            }
        }
        else {
            for (int32_t block = 0; block < Blocks; block++)
            {
                accum[block] = Value();
            }
        }
    }



    const Value value(int32_t block, int32_t idx) const
    {
        const Bitmap* bitmap = this->bitmap();

        if (bitmap->symbol(idx) == 1)
        {
            int32_t array_idx = this->array_idx(bitmap, idx);
            return array()->value(block, array_idx);
        }
        else {
            return Value();
        }
    }

    Values get_values(int32_t idx) const
    {
        Values v;

        auto bitmap = this->bitmap();

        if (bitmap->symbol(idx) == 1)
        {
            auto array = this->array();
            int32_t array_idx = this->array_idx(idx);

            OptionalAssignmentHelper(v, array->get_values(array_idx));
        }

        return v;
    }


    template <typename T>
    VoidResult setValues(int32_t idx, const core::StaticVector<T, Blocks>& values) noexcept
    {
        Bitmap* bitmap   = this->bitmap();
        Array* array     = this->array();

        if (values[0].is_set())
        {
            auto array_values  = this->array_values(values);
            int32_t array_idx  = this->array_idx(idx);

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
            int32_t array_idx = this->array_idx(idx);

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
    auto findGTForward(int32_t block, const T& val) const
    {
        auto result = array()->find_gt(block, val);

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }

    template <typename T>
    auto findGTForward(int32_t block, const Optional<T>& val) const
    {
        auto result = array()->find_gt(block, val.get());

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }

    template <typename T>
    auto findGEForward(int32_t block, const T& val) const
    {
        auto result = array()->find_ge(block, val.value());

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }

    template <typename T>
    auto findForward(SearchType search_type, int32_t block, const T& val) const
    {
        auto result = array()->findForward(search_type, block, val);

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }

    template <typename T>
    auto findForward(SearchType search_type, int32_t block, const Optional<T>& val) const
    {
        auto result = array()->findForward(search_type, block, val.get());

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }


    template <typename T>
    auto findBackward(SearchType search_type, int32_t block, const T& val) const
    {
        auto result = array()->findBackward(search_type, block, val);

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }

    template <typename T>
    auto findBackward(SearchType search_type, int32_t block, const Optional<T>& val) const
    {
        auto result = array()->findBackward(search_type, block, val.get());

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }



    VoidResult reindex() noexcept
    {
        MEMORIA_TRY_VOID(bitmap()->reindex());
        return array()->reindex();
    }

    void check() const
    {
        bitmap()->check();
        array()->check();
    }


    VoidResult splitTo(MyType* other, int32_t idx) noexcept
    {
        Bitmap* bitmap = this->bitmap();

        int32_t array_idx = this->array_idx(bitmap, idx);

        MEMORIA_TRY_VOID(bitmap->splitTo(other->bitmap(), idx));

        MEMORIA_TRY_VOID(array()->splitTo(other->array(), array_idx));

        return reindex();
    }

    VoidResult mergeWith(MyType* other) noexcept
    {
        MEMORIA_TRY_VOID(bitmap()->mergeWith(other->bitmap()));

        return array()->mergeWith(other->array());
    }

    VoidResult removeSpace(int32_t start, int32_t end) noexcept
    {
        return remove(start, end);
    }

    VoidResult remove(int32_t start, int32_t end) noexcept
    {
        Bitmap* bitmap = this->bitmap();

        int32_t array_start = array_idx(bitmap, start);
        int32_t array_end = array_idx(bitmap, end);

        MEMORIA_TRY_VOID(bitmap->remove(start, end));

        return array()->remove(array_start, array_end);
    }

    template <typename T>
    VoidResult insert(int32_t idx, const core::StaticVector<T, Blocks>& values) noexcept
    {
        Bitmap* bitmap  = this->bitmap();

        if (values[0].is_set())
        {
            MEMORIA_TRY_VOID(bitmap->insert(idx, 1));

            auto array_values  = this->array_values(values);
            int32_t array_idx  = this->array_idx(bitmap, idx);

            Array* array = this->array();
            return array->insert(array_idx, array_values);
        }
        else {
            return bitmap->insert(idx, 0);
        }
    }




    template <typename SerializationData>
    VoidResult serialize(SerializationData& buf) const noexcept
    {
        MEMORIA_TRY_VOID(Base::serialize(buf));

        MEMORIA_TRY_VOID(bitmap()->serialize(buf));
        return array()->serialize(buf);
    }

    template <typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
    {
        MEMORIA_TRY_VOID(Base::deserialize(buf));

        MEMORIA_TRY_VOID(bitmap()->deserialize(buf));
        return array()->deserialize(buf);
    }



protected:

    template <typename T>
    core::StaticVector<ArrayValue, Blocks> array_values(const core::StaticVector<Optional<T>, Blocks>& values)
    {
        core::StaticVector<ArrayValue, Blocks> tv;

        for (int32_t b = 0;  b < Blocks; b++)
        {
            tv[b] = values[b].get();
        }

        return tv;
    }

    int32_t array_idx(int32_t global_idx) const
    {
        return array_idx(bitmap(), global_idx);
    }

    int32_t array_idx(const Bitmap* bitmap, int32_t global_idx) const
    {
        int32_t rank = bitmap->rank(global_idx, 1);
        return rank;
    }


    int32_t global_idx(int32_t array_idx) const
    {
        return global_idx(bitmap(), array_idx);
    }

    int32_t global_idx(const Bitmap* bitmap, int32_t array_idx) const
    {
        auto result = bitmap->selectFw(1, array_idx + 1);
        return result.local_pos();
    }
};

template <typename Types>
struct PackedStructTraits<PackedVLenElementOptArray<Types>> {
    using SearchKeyDataType = typename Types::Value;

    using AccumType = typename DataTypeTraits<SearchKeyDataType>::ViewType;
    using SearchKeyType = Optional<typename DataTypeTraits<SearchKeyDataType>::ViewType>;

    static constexpr PackedDataTypeSize DataTypeSize = PackedDataTypeSize::VARIABLE;

    static constexpr PkdSearchType KeySearchType = PkdSearchType::MAX;
    static constexpr int32_t Indexes = PackedVLenElementOptArray<Types>::Indexes;
};

}
