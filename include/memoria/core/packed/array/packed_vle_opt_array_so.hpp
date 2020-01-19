
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

#include <memoria/core/packed/array/packed_vle_array_so.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <algorithm>

namespace memoria {

template <typename ExtData, typename PkdStruct>
class PackedVLenElementOptArraySO {

    PkdStruct* data_;

    using ArraySO = typename PkdStruct::Array::SparseObject;

    ArraySO array_;

    using MyType = PackedVLenElementOptArraySO;

public:

    using Array         = typename PkdStruct::Array;
    using Bitmap        = typename PkdStruct::Bitmap;

    using ViewType      = typename Array::ViewType;
    using DataType      = typename Array::DataType;
    using DataSizeType  = typename Array::DataSizeType;
    using DataAtomType  = typename Array::DataAtomType;
    using Accessor      = typename Array::Accessor;

    using FindResult    = typename ArraySO::FindResult;

    using Iterator = PkdRandomAccessIterator<Accessor>;

    using ConstIterator = PkdRandomAccessIterator<Accessor>;

    using PkdStructT = PkdStruct;

    static constexpr psize_t Columns = PkdStruct::Array::Blocks;
    static constexpr int32_t Indexes = PkdStruct::Array::Indexes;

    PackedVLenElementOptArraySO():
        data_(), array_()
    {}

    PackedVLenElementOptArraySO(const ExtData* ext_data, PkdStruct* data):
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



    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startStruct();
        handler->startGroup("VLE_OPTMAX_ARRAY");

        data_->bitmap()->generateDataEvents(handler);
        data_->array()->generateDataEvents(handler);

        handler->endGroup();
        handler->endStruct();
    }

    void check() const {
        data_->bitmap()->check();
    }

    psize_t size() const {
        return data_->bitmap()->size();
    }


    /*********************** API *********************/

    OpStatus splitTo(MyType& other, psize_t idx)
    {
        Bitmap* bitmap = data_->bitmap();

        psize_t array_idx = this->array_idx(bitmap, idx);

        if(isFail(bitmap->splitTo(other.data_->bitmap(), idx))) {
            return OpStatus::FAIL;
        }

        refresh_array();

        if(isFail(array_.splitTo(other.array_, array_idx))) {
            return OpStatus::FAIL;
        }

        refresh_array();

        return reindex();
    }

    OpStatus mergeWith(MyType& other)
    {
        if(isFail(data_->bitmap()->mergeWith(other.data_->bitmap()))) {
            return OpStatus::FAIL;
        }

        refresh_array();

        return array_.mergeWith(other.array_);
    }

    OpStatus removeSpace(psize_t start, psize_t end)
    {
        Bitmap* bitmap = data_->bitmap();

        int32_t array_start = array_idx(bitmap, start);
        int32_t array_end = array_idx(bitmap, end);

        if(isFail(bitmap->remove(start, end))) {
            return OpStatus::FAIL;
        }

        refresh_array();

        return array_.removeSpace(array_start, array_end);
    }

    OptionalT<ViewType> get_values(psize_t idx) const {
        return get_values(idx, 0);
    }

    OptionalT<ViewType>
    get_values(psize_t idx, psize_t column) const
    {
        auto bitmap = data_->bitmap();

        if (bitmap->symbol(idx) == 1)
        {
            psize_t array_idx = this->array_idx(idx);
            return array_.get_values(array_idx);
        }

        return OptionalT<ViewType>{};
    }


    OptionalT<ViewType> max(psize_t column) const
    {
        auto size = array_.size();

        if (size > 0)
        {
            return access(column, size - 1);
        }
        else {
            return OptionalT<ViewType>();
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
//    OpStatus _update_b(psize_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&& val)
//    {
//        if (isFail(removeSpace(pos, pos + 1))) {
//            return OpStatus::FAIL;
//        }

//        return _insert_b<Offset>(pos, accum, std::forward<AccessorFn>(val));
//    }

//    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
//    OpStatus _insert_b(psize_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&& val)
//    {
//        if (isFail(Accessor::insert(*this, pos, 1, [&](psize_t column, psize_t row){
//            return val(column);
//        }))) {
//            return OpStatus::FAIL;
//        }

//        return OpStatus::OK;
//    }

//    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
//    OpStatus _remove(psize_t idx, BranchNodeEntryItem<T, Size>& accum)
//    {
//        return removeSpace(idx, idx + 1);
//    }



    FindResult findGTForward(psize_t column, const ViewType& val) const
    {
        return array_.findGTForward(column, val);
    }



    FindResult findGEForward(psize_t column, const ViewType& val) const
    {
        return array_.findGEForward(column, val);
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

    auto findForward(SearchType search_type, psize_t column, const OptionalT<ViewType>& val) const
    {
        if (search_type == SearchType::GT)
        {
            return findGTForward(column, val.value());
        }
        else {
            return findGEForward(column, val.value());
        }
    }

    template <typename T>
    OpStatus setValues(int32_t idx, const core::StaticVector<T, Columns>& values)
    {
        if (values[0].is_set())
        {
            Bitmap* bitmap = data_->bitmap();

            auto array_values  = this->array_values(values);
            int32_t array_idx  = this->array_idx(idx);

            if (bitmap->symbol(idx))
            {
                if(isFail(array_.setValues(array_idx, array_values))) {
                    return OpStatus::FAIL;
                }

                refresh_array();
            }
            else {
                bitmap = data_->bitmap();

                if(isFail(array_.insert(array_idx, array_values))) {
                    return OpStatus::FAIL;
                }

                refresh_array();

                bitmap->symbol(idx) = 1;

                if (isFail(bitmap->reindex())) {
                    return OpStatus::FAIL;
                }
            }
        }
        else {
            Bitmap* bitmap = data_->bitmap();
            int32_t array_idx = this->array_idx(idx);

            if (bitmap->symbol(idx))
            {
                if (isFail(array_.removeSpace(array_idx, array_idx + 1))) {
                    return OpStatus::FAIL;
                }

                refresh_array();
                bitmap = data_->bitmap();

                bitmap->symbol(idx) = 0;
                if (isFail(bitmap->reindex())) {
                    return OpStatus::FAIL;
                }
            }
            else {
                // Do nothing
            }
        }

        refresh_array();

        return OpStatus::OK;
    }

    template <typename T>
    OpStatus insert(int32_t idx, const core::StaticVector<T, Columns>& values)
    {
        Bitmap* bitmap = data_->bitmap();

        if (values[0].is_set())
        {
            if(isFail(bitmap->insert(idx, 1))) {
                return OpStatus::FAIL;
            }

            refresh_array();

            auto array_values  = this->array_values(values);
            int32_t array_idx  = this->array_idx(bitmap, idx);

            return array_.insert(array_idx, array_values);
        }
        else {
            auto status = bitmap->insert(idx, 0);
            refresh_array();
            return status;
        }
    }

    OpStatus reindex()
    {
        if(isFail(data_->bitmap()->reindex())) {
            return OpStatus::FAIL;
        }

        refresh_array();

        return array_.reindex();
    }

private:

    void refresh_array() {
        array_.setup(data_->array());
    }

    template <typename T>
    core::StaticVector<ViewType, Columns> array_values(const core::StaticVector<OptionalT<T>, Columns>& values)
    {
        core::StaticVector<ViewType, Columns> tv;

        for (int32_t b = 0;  b < Columns; b++)
        {
            tv[b] = values[b].value();
        }

        return tv;
    }

    psize_t array_idx(psize_t global_idx) const
    {
        return array_idx(data_->bitmap(), global_idx);
    }

    psize_t array_idx(const Bitmap* bitmap, psize_t global_idx) const
    {
        psize_t rank = bitmap->rank(global_idx, 1);
        return rank;
    }
};


}
