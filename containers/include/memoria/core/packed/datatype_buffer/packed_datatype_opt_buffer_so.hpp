
// Copyright 2019-2022 Victor Smirnov
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
    ExtData* ext_data_;

    using BitmapExtData = std::tuple<>;

    BitmapExtData bitmap_ext_data_;

    using MyType = PackedDataTypeOptBufferSO;
public:

    using Array         = typename PkdStruct::Array;
    using ArraySO       = typename Array::SparseObject;

    using Bitmap        = typename PkdStruct::Bitmap;
    using BitmapSO      = typename Bitmap::SparseObject;
    using SymbolsRunT   = typename BitmapSO::SymbolsRunT;

    using ViewType      = typename Array::ViewType;
    using DataType      = typename Array::DataType;
    using FindResult    = typename ArraySO::FindResult;
    using Value         = typename PkdStruct::Value;
    using Values        = typename PkdStruct::Values;

    using ArrayUS  = typename ArraySO::UpdateState;
    using BitmapUS = typename BitmapSO::UpdateState;

    class UpdateState: public PkdStructUpdateBase<MyType> {
        ArrayUS array_update_state_;
        BitmapUS bitmap_update_state_;
    public:
        UpdateState() {}

        ArrayUS& array_update_state()   {return array_update_state_; }
        BitmapUS& bitmap_update_state() {return bitmap_update_state_;}

        void configure() {
            array_update_state_.set_allocator_state(this->allocator_state());
            bitmap_update_state_.set_allocator_state(this->allocator_state());
        }
    };

    using PkdStructT = PkdStruct;

    static constexpr size_t Columns = 1;
    static constexpr size_t Indexes = PkdStruct::Array::Indexes;

    PackedDataTypeOptBufferSO() :
        data_(), ext_data_()
    {}

    PackedDataTypeOptBufferSO(ExtData* ext_data, PkdStruct* data) :
        data_(data), ext_data_(ext_data)
    {}

    void setup() {
        data_ = nullptr;
        ext_data_ = nullptr;
    }

    void setup(ExtData* ext_data, PkdStruct* data)
    {
        ext_data_ = ext_data;
        data_ = data;
    }

    void setup(ExtData* ext_data)  {
        ext_data_ = ext_data;
    }

    void setup(PkdStruct* data)  {
        data_ = data;
    }

    operator bool() const  {
        return data_ != nullptr;
    }

    const ExtData* ext_data() const  {return ext_data_;}
    ExtData* ext_data()  {return ext_data_;}

    const PkdStruct* data() const  {return data_;}
    PkdStruct* data()  {return data_;}

    ArraySO array() const {
        return ArraySO(const_cast<ExtData*>(ext_data_), const_cast<Array*>(data_->array()));
    }

    ArraySO array() {
        return ArraySO(ext_data_, data_->array());
    }

    BitmapSO bitmap() const {
        return BitmapSO(const_cast<BitmapExtData*>(&bitmap_ext_data_), const_cast<Bitmap*>(data_->bitmap()));
    }

    BitmapSO bitmap() {
        return BitmapSO(&bitmap_ext_data_, data_->bitmap());
    }


    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startStruct();
        handler->startGroup("DATA_TYPE_OPT_BUFFER");

        bitmap().generateDataEvents(handler);
        array().generateDataEvents(handler);

        handler->endGroup();
        handler->endStruct();
    }

    void check() const
    {
        bitmap().check();
        array().check();

        auto rnk1 = bitmap().rank_eq(1);
        auto array_size = array().size();

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
        return bitmap().size();
    }


    /*********************** API *********************/

    size_t max_element_idx() const
    {
        size_t array_idx = array().size() - 1;
        size_t bm_idx = bitmap_idx(array_idx);
        return bm_idx;
    }

    const Value value(size_t block, size_t idx) const
    {
        BitmapSO bitmap = this->bitmap();

        if (bitmap.access(idx) == 1)
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

        if (bitmap.access(idx) == 1)
        {
            auto array = this->array();
            size_t array_idx = this->array_idx(idx);

            OptionalAssignmentHelper(v, array.get_values(array_idx));
        }

        return v;
    }


    Optional<ViewType> access(size_t column, size_t row) const
    {
        BitmapSO bitmap = this->bitmap();
        if (bitmap.access(row))
        {
            size_t array_idx  = this->array_idx(row);
            return array().access(column, array_idx);
        }
        else {
            return Optional<ViewType>{};
        }
    }

    void split_to(MyType& other, size_t idx)
    {
        BitmapSO bitmap = this->bitmap();

        size_t array_idx = this->array_idx(bitmap, idx);

        BitmapSO other_bitmap = other.bitmap();
        bitmap.split_to(other_bitmap, idx);

        ArraySO other_array = other.array();
        array().split_to(other_array, array_idx);

        return reindex();
    }

    PkdUpdateStatus prepare_merge_with(const MyType& other, UpdateState& update_state) const
    {
        update_state.configure();

        if (
            is_success(array().prepare_merge_with(other.array(), update_state.array_update_state())) &&
            is_success(bitmap().prepare_merge_with(other.bitmap(), update_state.bitmap_update_state()))
        ) {
            return PkdUpdateStatus::SUCCESS;
        }
        else {
            return PkdUpdateStatus::FAILURE;
        }
    }

    void commit_merge_with(MyType& other, UpdateState& update_state) const
    {
        update_state.configure();

        BitmapSO other_bitmap = other.bitmap();
        bitmap().commit_merge_with(other_bitmap, update_state.bitmap_update_state());

        ArraySO other_array = other.array();
        return array().commit_merge_with(other_array, update_state.array_update_state());
    }

    PkdUpdateStatus prepare_remove(size_t start, size_t end, UpdateState& update_state) const
    {
        update_state.configure();

        BitmapSO bitmap = this->bitmap();
        ArraySO array = this->array();

        size_t array_start = array_idx(bitmap, start);
        size_t array_end = array_idx(bitmap, end);

        if (is_success(array.prepare_remove(array_start, array_end, update_state.array_update_state()))) {
            return bitmap.prepare_remove(start, end, update_state.bitmap_update_state());
        }

        return PkdUpdateStatus::FAILURE;
    }

    void commit_remove(size_t start, size_t end, UpdateState& update_state)
    {
        update_state.configure();

        BitmapSO bitmap = this->bitmap();

        size_t array_start = array_idx(bitmap, start);
        size_t array_end = array_idx(bitmap, end);

        array().commit_remove(array_start, array_end, update_state.array_update_state());
        this->bitmap().commit_remove(start, end, update_state.bitmap_update_state());
    }

    template <typename AccessorFn>
    PkdUpdateStatus prepare_insert(size_t row_at, size_t size, UpdateState& update_state, AccessorFn&& elements)
    {
        update_state.configure();
        MEMORIA_ASSERT(row_at, <=, this->size());

        std::vector<SymbolsRunT> syms;
        size_t set_elements_num{};
        for (size_t i = 0; i < size; i++) {
            SymbolsRunT sym = is_not_empty(elements(0, i));
            set_elements_num += sym.symbol(0);
            syms.push_back(sym);
        }

        BitmapSO bitmap = this->bitmap();
        PkdUpdateStatus bm_status = bitmap.prepare_insert(row_at, update_state.bitmap_update_state(), to_const_span(syms));
        if (!is_success(bm_status)) {
            return PkdUpdateStatus::FAILURE;
        }

        //size_t set_elements_num = bitmap.rank_eq(row_at, row_at + size, 1);
        //size_t array_row_at = array_idx(row_at);

        size_t array_row_at = 0;
        PkdUpdateStatus arr_status = array().prepare_insert(array_row_at, set_elements_num, update_state.array_update_state(), [&](size_t col, size_t arr_idx)  {
            size_t bm_idx = bitmap_idx(bitmap, row_at + arr_idx);
            return elements(col, bm_idx - row_at).value();
        });

        return arr_status;
    }


    template <typename AccessorFn>
    void commit_insert(size_t row_at, size_t size, UpdateState& update_state, AccessorFn&& elements)
    {
        MEMORIA_ASSERT(row_at, <=, this->size());

        std::vector<SymbolsRunT> syms;
        BitmapSO bitmap = this->bitmap();
        bitmap.commit_insert(row_at, update_state.bitmap_update_state(), to_const_span(syms));

        size_t set_elements_num = bitmap.rank_eq(row_at, row_at + size, 1);
        size_t array_row_at = array_idx(row_at);

        array().commit_insert(array_row_at, set_elements_num, update_state.array_update_state(), [&](size_t col, size_t arr_idx)  {
            size_t bm_idx = bitmap_idx(bitmap, row_at + arr_idx);
            return elements(col, bm_idx - row_at).value();
        });
    }

    template <typename AccessorFn>
    PkdUpdateStatus prepare_update(size_t row_at, size_t size, UpdateState& update_state, AccessorFn&& elements) const
    {
        BitmapSO bitmap = this->bitmap();
        ArraySO  array  = this->array();

        update_state.configure();
        MEMORIA_ASSERT(row_at + size, <=, this->size());

        std::vector<SymbolsRunT> syms;
        std::vector<size_t> non_empty_pos;
        size_t non_empty{};
        for (size_t i = 0; i < size; i++)
        {
            SymbolsRunT run = is_not_empty(elements(0, i));
            syms.push_back(run);
            if (MMA_LIKELY(run.symbol(0))) {
                non_empty ++;
                non_empty_pos.push_back(i);
            }
        }

        PkdUpdateStatus bm_status = bitmap.prepare_update(row_at, update_state.bitmap_update_state(), to_const_span(syms));
        if (!is_success(bm_status)) {
            return PkdUpdateStatus::FAILURE;
        }

        size_t set_elements_num = bitmap.rank_eq(row_at, row_at + size, 1);
        size_t array_row_at = array_idx(row_at);

        PkdUpdateStatus arr_status_rem = array.prepare_remove(array_row_at, array_row_at + set_elements_num, update_state.array_update_state());
        if (is_success(arr_status_rem)){
            return array.prepare_insert(array_row_at, non_empty, update_state.array_update_state(), [&](size_t col, size_t arr_idx) {
                return elements(col, non_empty_pos[arr_idx]).value();
            });
        }

        return PkdUpdateStatus::FAILURE;
    }

    template <typename AccessorFn>
    void commit_update(size_t row_at, size_t size, UpdateState& update_state, AccessorFn&& elements)
    {
        MEMORIA_ASSERT(row_at + size, <=, this->size());

        BitmapSO bitmap = this->bitmap();

        size_t set_elements_num_old = bitmap.rank_eq(row_at, row_at + size, 1);
        size_t array_row_at = array_idx(row_at);

        std::vector<SymbolsRunT> syms; // This array is actually unused here
        bitmap.commit_update(row_at, update_state.bitmap_update_state(), to_const_span(syms));

        size_t set_elements_num_new = bitmap.rank_eq(row_at, row_at + size, 1);
        array().commit_remove(array_row_at, array_row_at + set_elements_num_old, update_state.array_update_state());

        array().commit_insert(array_row_at, set_elements_num_new, update_state.array_update_state(), [&](size_t col, size_t arr_idx)  {
            size_t bm_idx = bitmap_idx(bitmap, row_at + arr_idx);
            return elements(col, bm_idx - row_at).value();
        });
    }

    FindResult findGTForward(size_t column, const ViewType& val) const
    {
        FindResult res = array().findGTForward(column, val);

        size_t bmp_idx = this->bitmap_idx(res.local_pos());

        res.set_local_pos(bmp_idx);

        return res;
    }

    FindResult find_fw_gt(size_t column, const ViewType& val) const {
        return findGTForward(column, val);
    }



    FindResult findGEForward(size_t column, const ViewType& val) const
    {
        FindResult res = array().findGEForward(column, val);

        size_t bmp_idx = this->bitmap_idx(res.local_pos());

        res.set_local_pos(bmp_idx);

        return res;
    }

    FindResult find_fw_ge(size_t column, const ViewType& val) const {
        return findGEForward(column, val);
    }


    template <typename T>
    void setValues(size_t idx, const core::StaticVector<T, Columns>& values)
    {
        if (values[0])
        {
            BitmapSO bitmap = this->bitmap();

            auto array_values  = this->array_values(values);
            size_t array_idx  = this->array_idx(idx);

            if (bitmap.access(idx))
            {
                array().setValues(array_idx, array_values);
            }
            else {
                array().insert(array_idx, array_values);

                bitmap = data_->bitmap();
                bitmap.set_symbol(idx, 1);

                bitmap.reindex();
            }
        }
        else {
            BitmapSO bitmap = this->bitmap();
            size_t array_idx = this->array_idx(idx);

            if (bitmap.access(idx))
            {
                array().remove(array_idx, array_idx + 1);
                bitmap = data_->bitmap();

                bitmap->set_symbol(idx, 0);
                bitmap.reindex();
            }
            else {
                // Do nothing
            }
        }
    }

    template <typename T>
    void insert(size_t idx, const core::StaticVector<T, Columns>& values)
    {
        BitmapSO bitmap = this->bitmap();

        if (values[0])
        {
            bitmap.insert(idx, 1);

            auto array_values  = this->array_values(values);
            size_t array_idx  = this->array_idx(bitmap, idx);

            return array().insert(array_idx, array_values);
        }
        else {
            return bitmap.insert(idx, 0);
        }
    }

    void reindex()
    {
        bitmap().reindex();
        return array().reindex();
    }

    MMA_MAKE_UPDATE_STATE_METHOD

private:

    template <typename T>
    SymbolsRunT is_not_empty(const T& value) const  {
        return SymbolsRunT{1, 1, 1};
    }

    template <typename T>
    SymbolsRunT is_not_empty(const Optional<T>& value) const  {
        bool exists = (bool)value;
        return SymbolsRunT{1, exists, 1};
    }

    template <typename T>
    core::StaticVector<ViewType, Columns> array_values(const core::StaticVector<Optional<T>, Columns>& values)
    {
        core::StaticVector<ViewType, Columns> tv;

        for (size_t b = 0;  b < Columns; b++)
        {
            tv[b] = values[b].value();
        }

        return tv;
    }

    size_t array_idx(size_t global_idx) const
    {
        return array_idx(bitmap(), global_idx);
    }

    size_t bitmap_idx(size_t array_idx) const
    {
        return bitmap_idx(bitmap(), array_idx);
    }

    size_t array_idx(const BitmapSO& bitmap, size_t global_idx) const
    {
        size_t rank = bitmap.rank_eq(global_idx, 1);
        return rank;
    }

    size_t bitmap_idx(const BitmapSO& bitmap, size_t array_idx) const
    {
        auto select_res = bitmap.select_fw_eq(array_idx, 1);
        return select_res.idx;
    }
};


}
