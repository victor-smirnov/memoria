
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
    using FindResult    = typename ArraySO::FindResult;

    using PkdStructT = PkdStruct;

    static constexpr size_t Columns = 1;
    static constexpr size_t Indexes = PkdStruct::Array::Indexes;

    PackedDataTypeOptBufferSO() :
        data_(), array_()
    {}

    PackedDataTypeOptBufferSO(ExtData* ext_data, PkdStruct* data) :
        data_(data), array_(ext_data, data->array())
    {}

    void setup() {
        array_.setup();
        data_ = nullptr;
    }

    void setup(ExtData* ext_data, PkdStruct* data)
    {
        array_.setup(ext_data, data->array());
        data_ = data;
    }

    void setup(ExtData* ext_data)  {
        array_.setup(ext_data);
    }

    void setup(PkdStruct* data)  {
        array_.setup(data->array());
        data_ = data;
    }

    operator bool() const  {
        return data_ != nullptr;
    }

    const ExtData* ext_data() const  {return array_.ext_data();}
    ExtData* ext_data()  {return array_.ext_data();}

    const PkdStruct* data() const  {return data_;}
    PkdStruct* data()  {return data_;}



    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startStruct();
        handler->startGroup("DATA_TYPE_OPT_BUFFER");

        data_->bitmap()->generateDataEvents(handler);
        array_.generateDataEvents(handler);

        handler->endGroup();
        handler->endStruct();
    }

    void check() const
    {
        data_->bitmap()->check();
        array_.check();

        auto rnk1 = data_->bitmap()->rank(1);
        auto array_size = array_.size();

        if (rnk1 != array_size)
        {
            MEMORIA_MAKE_GENERIC_ERROR(
                "PackedDataTypeOptBufferSO: Bitmap.rank(1) != array.size(): {} {}",
                rnk1,
                array_size
            ).do_throw();
        }
    }

    size_t size() const  {
        return data_->bitmap()->size();
    }


    /*********************** API *********************/

    size_t max_element_idx() const  {
        size_t array_idx = array_.size() - 1;
        size_t bm_idx = bitmap_idx(array_idx);
        return bm_idx;
    }

    Optional<ViewType> access(size_t column, size_t row) const
    {
        Bitmap* bitmap = data_->bitmap();
        if (bitmap->symbol(row))
        {
            size_t array_idx  = this->array_idx(row);
            return array_.access(column, array_idx);
        }
        else {
            return Optional<ViewType>{};
        }
    }

    VoidResult splitTo(MyType& other, size_t idx)
    {
        Bitmap* bitmap = data_->bitmap();

        size_t array_idx = this->array_idx(bitmap, idx);

        MEMORIA_TRY_VOID(bitmap->splitTo(other.data_->bitmap(), idx));

        refresh_array();
        other.refresh_array();

        MEMORIA_TRY_VOID(array_.splitTo(other.array_, array_idx));

        refresh_array();
        other.refresh_array();

        return reindex();
    }

    VoidResult mergeWith(MyType& other) const
    {
        MEMORIA_TRY_VOID(data_->bitmap()->mergeWith(other.data_->bitmap()));

        other.refresh_array();

        return array_.mergeWith(other.array_);
    }

    VoidResult removeSpace(size_t start, size_t end)
    {
        Bitmap* bitmap = data_->bitmap();

        size_t array_start = array_idx(bitmap, start);
        size_t array_end = array_idx(bitmap, end);

        MEMORIA_TRY_VOID(bitmap->remove(start, end));

        refresh_array();

        return array_.removeSpace(array_start, array_end);
    }

//    Optional<ViewType> get_values(size_t idx) const {
//        return get_values(idx, 0);
//    }

//    Optional<ViewType>
//    get_values(size_t idx, size_t column) const
//    {
//        auto bitmap = data_->bitmap();

//        if (bitmap->symbol(idx) == 1)
//        {
//            size_t array_idx = this->array_idx(idx);
//            return array_.get_values(array_idx);
//        }

//        return Optional<ViewType>{};
//    }


    template <typename AccessorFn>
    VoidResult insert_entries(size_t row_at, size_t size, AccessorFn&& elements)
    {
        MEMORIA_ASSERT_RTN(row_at, <=, this->size());

        Bitmap* bitmap = data_->bitmap();
        MEMORIA_TRY_VOID(bitmap->insert_entries(row_at, size, [&](size_t pos){
            return is_not_empty(elements(0, pos));
        }));

        refresh_array();

        size_t set_elements_num = bitmap->rank(row_at, row_at + size, 1);
        size_t array_row_at = array_idx(row_at);

        return array_.insert_entries(array_row_at, set_elements_num, [&](size_t col, size_t arr_idx)  {
            size_t bm_idx = bitmap_idx(bitmap, row_at + arr_idx);
            return elements(col, bm_idx - row_at).get();
        });
    }

    template <typename AccessorFn>
    VoidResult update_entries(size_t row_at, size_t size, AccessorFn&& elements)
    {
        MEMORIA_TRY_VOID(remove_entries(row_at, size));
        return insert_entries(row_at, size, std::forward<AccessorFn>(elements));
    }

    VoidResult remove_entries(size_t row_at, size_t size)
    {
        return removeSpace(row_at, row_at + size);
    }



    FindResult findGTForward(size_t column, const ViewType& val) const
    {
        FindResult res = array_.findGTForward(column, val);

        size_t bmp_idx = this->bitmap_idx(res.local_pos());

        res.set_local_pos(bmp_idx);

        return res;
    }



    FindResult findGEForward(size_t column, const ViewType& val) const
    {
        FindResult res = array_.findGEForward(column, val);

        size_t bmp_idx = this->bitmap_idx(res.local_pos());

        res.set_local_pos(bmp_idx);

        return res;
    }

    auto findForward(SearchType search_type, size_t column, const ViewType& val) const
    {
        if (search_type == SearchType::GT)
        {
            return findGTForward(column, val);
        }
        else {
            return findGEForward(column, val);
        }
    }

    auto findForward(SearchType search_type, size_t column, const Optional<ViewType>& val) const
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
    VoidResult setValues(size_t idx, const core::StaticVector<T, Columns>& values)
    {
        if (values[0])
        {
            Bitmap* bitmap = data_->bitmap();

            auto array_values  = this->array_values(values);
            size_t array_idx  = this->array_idx(idx);

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
            size_t array_idx = this->array_idx(idx);

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
    VoidResult insert(size_t idx, const core::StaticVector<T, Columns>& values)
    {
        Bitmap* bitmap = data_->bitmap();

        if (values[0])
        {
            MEMORIA_TRY_VOID(bitmap->insert(idx, 1));

            refresh_array();

            auto array_values  = this->array_values(values);
            size_t array_idx  = this->array_idx(bitmap, idx);

            return array_.insert(array_idx, array_values);
        }
        else {
            auto status = bitmap->insert(idx, 0);
            MEMORIA_RETURN_IF_ERROR(status);
            refresh_array();
            return VoidResult::of();
        }
    }

    VoidResult reindex()
    {
        MEMORIA_TRY_VOID(data_->bitmap()->reindex());

        refresh_array();

        return array_.reindex();
    }

private:

    template <typename T>
    bool is_not_empty(const T& value) const  {
        return true;
    }

    template <typename T>
    bool is_not_empty(const Optional<T>& value) const  {
        return (bool)value;
    }


    void refresh_array() {
        array_.setup(data_->array());
    }

    template <typename T>
    core::StaticVector<ViewType, Columns> array_values(const core::StaticVector<Optional<T>, Columns>& values)
    {
        core::StaticVector<ViewType, Columns> tv;

        for (size_t b = 0;  b < Columns; b++)
        {
            tv[b] = values[b].get();
        }

        return tv;
    }

    size_t array_idx(size_t global_idx) const
    {
        return array_idx(data_->bitmap(), global_idx);
    }

    size_t bitmap_idx(size_t array_idx) const
    {
        return bitmap_idx(data_->bitmap(), array_idx);
    }

    size_t array_idx(const Bitmap* bitmap, size_t global_idx) const
    {
        size_t rank = bitmap->rank(global_idx, 1);
        return rank;
    }

    size_t bitmap_idx(const Bitmap* bitmap, size_t array_idx) const
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
