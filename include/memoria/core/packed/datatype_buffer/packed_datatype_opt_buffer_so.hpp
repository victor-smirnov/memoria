
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
#include <memoria/core/tools/assert.hpp>

#include <memoria/profiles/common/block_operations.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/core/packed/tools/packed_tools.hpp>

//#include <memoria/core/packed/array/packed_vle_array_so.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <algorithm>

namespace memoria {

template <typename ExtData, typename PkdStruct>
class PackedDataTypeOptBufferSO {

    PkdStruct* data_;

    using ArraySO = typename PkdStruct::Array::SparseObject;

    ArraySO array_;

    using MyType = PackedDataTypeOptBufferSO;

public:

    using Array         = typename PkdStruct::Array;
    using Bitmap        = typename PkdStruct::Bitmap;

    using ViewType      = typename Array::ViewType;
    using DataType      = typename Array::DataType;
//    using Accessor      = typename Array::Accessor;

    using FindResult    = typename ArraySO::FindResult;

//    using Iterator      = PkdRandomAccessIterator<Accessor>;
//    using ConstIterator = PkdRandomAccessIterator<Accessor>;

    using PkdStructT = PkdStruct;

    static constexpr psize_t Columns = 1;
    static constexpr int32_t Indexes = PkdStruct::Array::Indexes;

    PackedDataTypeOptBufferSO():
        data_(), array_()
    {}

    PackedDataTypeOptBufferSO(const ExtData* ext_data, PkdStruct* data):
        data_(data), array_(ext_data, data->array())
    {}

    void setup() {
        array_.setup();
        data_ = nullptr;
    }

    void setup(const ExtData* ext_data, PkdStruct* data)
    {
        array_.setup(ext_data, data->array());
        data_ = data;
    }

    void setup(const ExtData* ext_data) {
        array_.setup(ext_data);
    }

    void setup(PkdStruct* data) {
        array_.setup(data->array());
        data_ = data;
    }

    operator bool() const {
        return data_ != nullptr;
    }

    const ExtData* ext_data() const {return array_.ext_data();}
    const PkdStruct* data() const {return data_;}
    PkdStruct* data() {return data_;}



    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        handler->startStruct();
        handler->startGroup("DATA_TYPE_OPT_BUFFER");

        MEMORIA_TRY_VOID(data_->bitmap()->generateDataEvents(handler));
        MEMORIA_TRY_VOID(array_.generateDataEvents(handler));

        handler->endGroup();
        handler->endStruct();

        return VoidResult::of();
    }

    void check() const {
        data_->bitmap()->check();
    }

    psize_t size() const {
        return data_->bitmap()->size();
    }


    /*********************** API *********************/

    VoidResult splitTo(MyType& other, psize_t idx) noexcept
    {
        Bitmap* bitmap = data_->bitmap();

        psize_t array_idx = this->array_idx(bitmap, idx);

        MEMORIA_TRY_VOID(bitmap->splitTo(other.data_->bitmap(), idx));

        refresh_array();
        other.refresh_array();

        MEMORIA_TRY_VOID(array_.splitTo(other.array_, array_idx));

        refresh_array();
        other.refresh_array();

        return reindex();
    }

    VoidResult mergeWith(MyType& other) noexcept
    {
        MEMORIA_TRY_VOID(data_->bitmap()->mergeWith(other.data_->bitmap()));

        refresh_array();
        other.refresh_array();

        return array_.mergeWith(other.array_);
    }

    VoidResult removeSpace(psize_t start, psize_t end) noexcept
    {
        Bitmap* bitmap = data_->bitmap();

        int32_t array_start = array_idx(bitmap, start);
        int32_t array_end = array_idx(bitmap, end);

        MEMORIA_TRY_VOID(bitmap->remove(start, end));

        refresh_array();

        return array_.removeSpace(array_start, array_end);
    }

    Optional<ViewType> get_values(psize_t idx) const {
        return get_values(idx, 0);
    }

    Optional<ViewType>
    get_values(psize_t idx, psize_t column) const
    {
        auto bitmap = data_->bitmap();

        if (bitmap->symbol(idx) == 1)
        {
            psize_t array_idx = this->array_idx(idx);
            return array_.get_values(array_idx);
        }

        return Optional<ViewType>{};
    }


    Optional<ViewType> max(psize_t column) const
    {
        auto size = array_.size();

        if (size > 0)
        {
            return this->access(column, size - 1);
        }
        else {
            return Optional<ViewType>{};
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void max(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        for (int32_t column = 0; column < Columns; column++)
        {
            accum[column] = this->max(column);
        }
    }



    template <typename T>
    void max(core::StaticVector<T, Columns>& accum) const
    {
        psize_t size = array_.size();

        if (size > 0)
        {
            for (int32_t column = 0; column < Columns; column++)
            {
                accum[column] = array_.access(column, size - 1);
            }
        }
        else {
            for (int32_t column = 0; column < Columns; column++)
            {
                accum[column] = T{};
            }
        }
    }



//    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
//    VoidResult _update_b(psize_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&& val)
//    {
//        if (isFail(removeSpace(pos, pos + 1))) {
//            return VoidResult::FAIL;
//        }

//        return _insert_b<Offset>(pos, accum, std::forward<AccessorFn>(val));
//    }

//    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
//    VoidResult _insert_b(psize_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&& val)
//    {
//        if (isFail(Accessor::insert(*this, pos, 1, [&](psize_t column, psize_t row){
//            return val(column);
//        }))) {
//            return VoidResult::FAIL;
//        }

//        return VoidResult::OK;
//    }

//    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
//    VoidResult _remove(psize_t idx, BranchNodeEntryItem<T, Size>& accum)
//    {
//        return removeSpace(idx, idx + 1);
//    }



    FindResult findGTForward(psize_t column, const ViewType& val) const
    {
        FindResult res = array_.findGTForward(column, val);

        psize_t bmp_idx = this->bitmap_idx(res.local_pos());

        res.set_local_pos(bmp_idx);

        return res;
    }



    FindResult findGEForward(psize_t column, const ViewType& val) const
    {
        FindResult res = array_.findGEForward(column, val);

        psize_t bmp_idx = this->bitmap_idx(res.local_pos());

        res.set_local_pos(bmp_idx);

        return res;
    }

    auto findForward(SearchType search_type, psize_t column, const ViewType& val) const
    {
        if (search_type == SearchType::GT)
        {
            return findGTForward(column, val);
        }
        else {
            return findGEForward(column, val);
        }
    }

    auto findForward(SearchType search_type, psize_t column, const Optional<ViewType>& val) const
    {
        if (search_type == SearchType::GT)
        {
            return findGTForward(column, val.get());
        }
        else {
            return findGEForward(column, val.get());
        }
    }

    template <typename T>
    VoidResult setValues(int32_t idx, const core::StaticVector<T, Columns>& values) noexcept
    {
        if (values[0])
        {
            Bitmap* bitmap = data_->bitmap();

            auto array_values  = this->array_values(values);
            int32_t array_idx  = this->array_idx(idx);

            if (bitmap->symbol(idx))
            {
                MEMORIA_TRY_VOID(array_.setValues(array_idx, array_values));

                refresh_array();
            }
            else {
                bitmap = data_->bitmap();

                MEMORIA_TRY_VOID(array_.insert(array_idx, array_values));

                refresh_array();

                bitmap->symbol(idx) = 1;

                MEMORIA_TRY_VOID(bitmap->reindex());
            }
        }
        else {
            Bitmap* bitmap = data_->bitmap();
            int32_t array_idx = this->array_idx(idx);

            if (bitmap->symbol(idx))
            {
                MEMORIA_TRY_VOID(array_.removeSpace(array_idx, array_idx + 1));

                refresh_array();
                bitmap = data_->bitmap();

                bitmap->symbol(idx) = 0;
                MEMORIA_TRY_VOID(bitmap->reindex());
            }
            else {
                // Do nothing
            }
        }

        refresh_array();

        return VoidResult::of();
    }

    template <typename T>
    VoidResult insert(int32_t idx, const core::StaticVector<T, Columns>& values) noexcept
    {
        Bitmap* bitmap = data_->bitmap();

        if (values[0])
        {
            MEMORIA_TRY_VOID(bitmap->insert(idx, 1));

            refresh_array();

            auto array_values  = this->array_values(values);
            int32_t array_idx  = this->array_idx(bitmap, idx);

            return array_.insert(array_idx, array_values);
        }
        else {
            auto status = bitmap->insert(idx, 0);
            MEMORIA_RETURN_IF_ERROR(status);
            refresh_array();
            return VoidResult::of();
        }
    }

    VoidResult reindex() noexcept
    {
        MEMORIA_TRY_VOID(data_->bitmap()->reindex());

        refresh_array();

        return array_.reindex();
    }

private:

    void refresh_array() noexcept {
        array_.setup(data_->array());
    }

    template <typename T>
    core::StaticVector<ViewType, Columns> array_values(const core::StaticVector<Optional<T>, Columns>& values)
    {
        core::StaticVector<ViewType, Columns> tv;

        for (int32_t b = 0;  b < Columns; b++)
        {
            tv[b] = values[b].get();
        }

        return tv;
    }

    psize_t array_idx(psize_t global_idx) const
    {
        return array_idx(data_->bitmap(), global_idx);
    }

    psize_t bitmap_idx(psize_t array_idx) const
    {
        return bitmap_idx(data_->bitmap(), array_idx);
    }

    psize_t array_idx(const Bitmap* bitmap, psize_t global_idx) const
    {
        psize_t rank = bitmap->rank(global_idx, 1);
        return rank;
    }

    psize_t bitmap_idx(const Bitmap* bitmap, psize_t array_idx) const
    {
        auto select_res = bitmap->selectFw(1, array_idx + 1);

        if (select_res.is_found()) {
            return select_res.local_pos();
        }
        else {
            return bitmap->size();
        }
    }
};


}
