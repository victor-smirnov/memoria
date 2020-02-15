
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

    VoidResult check() const noexcept
    {
        MEMORIA_TRY_VOID(data_->bitmap()->check());
        MEMORIA_TRY_VOID(array_.check());

        auto rnk1 = data_->bitmap()->rank(1);
        auto array_size = array_.size();

        if (rnk1 != array_size)
        {
            return MEMORIA_MAKE_GENERIC_ERROR(
                "PackedDataTypeOptBufferSO: Bitmap.rank(1) != array.size(): {} {}",
                rnk1,
                array_size
            );
        }

        return VoidResult::of();
    }

    psize_t size() const {
        return data_->bitmap()->size();
    }


    /*********************** API *********************/

    psize_t max_element_idx() const noexcept {
        psize_t array_idx = array_.size() - 1;
        psize_t bm_idx = bitmap_idx(array_idx);
        return bm_idx;
    }

    Optional<ViewType> access(int32_t column, int32_t row) const noexcept
    {
        Bitmap* bitmap = data_->bitmap();
        if (bitmap->symbol(row))
        {
            int32_t array_idx  = this->array_idx(row);
            return array_.access(column, array_idx);
        }
        else {
            return Optional<ViewType>{};
        }
    }

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

    VoidResult mergeWith(MyType& other) const noexcept
    {
        MEMORIA_TRY_VOID(data_->bitmap()->mergeWith(other.data_->bitmap()));

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


    template <typename AccessorFn>
    VoidResult insert_entries(psize_t row_at, psize_t size, AccessorFn&& elements) noexcept
    {
        MEMORIA_ASSERT_RTN(row_at, <=, this->size());

        Bitmap* bitmap = data_->bitmap();
        MEMORIA_TRY_VOID(bitmap->insert_entries(row_at, size, [&](psize_t pos){
            return is_not_empty(elements(0, pos));
        }));

        refresh_array();

        psize_t set_elements_num = bitmap->rank(row_at, row_at + size, 1);
        psize_t array_row_at = array_idx(row_at);

        return array_.insert_entries(array_row_at, set_elements_num, [&](psize_t col, psize_t arr_idx) noexcept {
            psize_t bm_idx = bitmap_idx(bitmap, row_at + arr_idx);
            return elements(col, bm_idx - row_at).get();
        });
    }

    template <typename AccessorFn>
    VoidResult update_entries(psize_t row_at, psize_t size, AccessorFn&& elements) noexcept
    {
        MEMORIA_TRY_VOID(remove_entries(row_at, size));
        return insert_entries(row_at, size, std::forward<AccessorFn>(elements));
    }

    VoidResult remove_entries(psize_t row_at, psize_t size) noexcept
    {
        return removeSpace(row_at, row_at + size);
    }



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

    template <typename T>
    bool is_not_empty(const T& value) const noexcept {
        return true;
    }

    template <typename T>
    bool is_not_empty(const Optional<T>& value) const noexcept {
        return (bool)value;
    }


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

    psize_t bitmap_idx(psize_t array_idx) const noexcept
    {
        return bitmap_idx(data_->bitmap(), array_idx);
    }

    psize_t array_idx(const Bitmap* bitmap, psize_t global_idx) const
    {
        psize_t rank = bitmap->rank(global_idx, 1);
        return rank;
    }

    psize_t bitmap_idx(const Bitmap* bitmap, psize_t array_idx) const noexcept
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
