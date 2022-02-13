
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

#include <memoria/core/packed/tools/packed_allocator.hpp>

#include <memoria/core/packed/datatype_buffer/packed_array_iterator.hpp>
#include <memoria/core/packed/datatype_buffer/packed_datatype_buffer_fse_tools.hpp>
#include <memoria/core/packed/datatype_buffer/packed_datatype_buffer_vle_tools.hpp>

#include <memoria/core/packed/datatype_buffer/packed_array_iterator.hpp>

#include <memoria/core/datatypes/buffer/buffer_generic.hpp>
#include <memoria/core/datatypes/datum.hpp>

#include <memoria/core/iovector/io_substream_base.hpp>
#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <algorithm>

namespace memoria {

namespace detail {

    template <typename BufferSO, DTOrdering Ordering = BufferSO::Ordering>
    struct PkdDTBufOrderingDispatcher;

    template <typename BufferSO>
    struct PkdDTBufOrderingDispatcher<BufferSO, DTOrdering::UNORDERED> {
        static void check(const BufferSO& buffer) {}
        static VoidResult reindex(BufferSO& buffer) {
            return VoidResult::of();
        }

        struct FindResult {};
    };

    template <typename BufferSO>
    struct PkdDTBufOrderingDispatcher<BufferSO, DTOrdering::MAX> {
        using DataType = typename BufferSO::DataType;
        using ViewType = DTTViewType<DataType>;

        class FindResult {
            size_t idx_;
        public:
            FindResult(size_t idx): idx_(idx)
            {}

            FindResult(): idx_()
            {}

            ViewType prefix() {return ViewType{};}
            size_t local_pos() const {return idx_;}

            void set_local_pos(size_t pos)  {
                idx_ = pos;
            }
        };


        static void check(const BufferSO& buffer){
            buffer.check_max();
        }

        static VoidResult reindex(BufferSO& buffer) {
            return VoidResult::of();
        }

        static auto find_fw_gt(const BufferSO& buffer, size_t column, const ViewType& val) {
            return buffer.find_gt_fw_max(column, val);
        }

        static auto find_fw_ge(const BufferSO& buffer, size_t column, const ViewType& val) {
            return buffer.find_ge_fw_max(column, val);
        }

        static ViewType sum(const BufferSO& buffer, size_t column, size_t row) {
            return buffer.sum_gen(column, row);
        }
    };

    template <typename BufferSO>
    struct PkdDTBufOrderingDispatcher<BufferSO, DTOrdering::SUM> {
        using DataType = typename BufferSO::DataType;
        using ViewType = DTTViewType<DataType>;


        class FindResult {
            size_t idx_;
            ViewType prefix_;
        public:
            FindResult(size_t idx, ViewType prefix):
                idx_(idx), prefix_(prefix)
            {}

            FindResult(): idx_(), prefix_()
            {}

            ViewType& prefix() {return prefix_;}
            const ViewType& prefix() const {return prefix_;}
            const FindResult& sub_prefix(const ViewType& view) {
                prefix_ -= view;
                return *this;
            }

            size_t local_pos() const {return idx_;}

            void set_local_pos(size_t pos) {
                idx_ = pos;
            }
        };



        static void check(const BufferSO& buffer) {
            buffer.check_sum();
        }

        static VoidResult reindex(BufferSO& buffer) {
            return buffer.reindex_sum();
        }

        static auto find_fw_gt(const BufferSO& buffer, size_t column, const ViewType& val) {
            return buffer.find_gt_fw_sum(column, val);
        }

        static auto find_fw_ge(const BufferSO& buffer, size_t column, const ViewType& val) {
            return buffer.find_ge_fw_sum(column, val);
        }

        static ViewType sum(const BufferSO& buffer, size_t column, size_t row) {
            return buffer.sum_sum(column, row);
        }
    };
}

template <typename ExtData, typename PkdStruct>
class PackedDataTypeBufferSO {
    ExtData* ext_data_;
    PkdStruct* data_;

    using MyType = PackedDataTypeBufferSO;

    template <typename, DTOrdering>
    friend struct detail::PkdDTBufOrderingDispatcher;

public:    
    static constexpr size_t Dimensions = PkdStruct::Dimensions;
    static constexpr size_t Indexes = PkdStruct::Indexes;
    static constexpr bool Indexed = PkdStruct::Indexed;

    static constexpr size_t Columns = PkdStruct::Columns;
    static constexpr DTOrdering Ordering = PkdStruct::Ordering;

    using ViewType      = typename PkdStruct::ViewType;
    using DataType      = typename PkdStruct::DataType;
    using Accessor      = pdtbuf_::PkdBufViewAccessor<PackedDataTypeBufferSO>;
    using DataDimensionsTuple = DTTDataDimensionsTuple<DataType>;

    using ConstIterator = PkdRandomAccessIterator<Accessor>;

    using PkdStructT = PkdStruct;

    using DataLengths = core::StaticVector<size_t, Dimensions>;

    using Values = core::StaticVector<ViewType, Columns>;

    using FindResult = typename detail::PkdDTBufOrderingDispatcher<MyType>::FindResult;

    PackedDataTypeBufferSO() : ext_data_(), data_() {}
    PackedDataTypeBufferSO(ExtData* ext_data, PkdStruct* data) :
        ext_data_(ext_data), data_(data)
    {}


    void setup()  {
        ext_data_ = nullptr;
        data_ = nullptr;
    }

    void setup(ExtData* ext_data, PkdStruct* data)  {
        ext_data_ = ext_data;
        data_ = data;
    }

    void setup(ExtData* ext_data)  {
        ext_data_ = ext_data;
    }

    void setup(PkdStruct* data)  {
        data_ = data;
    }

    ConstIterator begin(size_t column) const
    {
        return ConstIterator(Accessor(*this, column), 0, data_->size());
    }

    ConstIterator begin(size_t column, size_t row) const
    {
        return ConstIterator(Accessor(*this, column), row, data_->size());
    }

    ConstIterator end(size_t column) const
    {
        size_t size = data_->size();
        return ConstIterator(Accessor(*this, column), size, size);
    }

    operator bool() const {
        return data_ != nullptr;
    }

    const ExtData* ext_data() const  {return ext_data_;}
    ExtData* ext_data()  {return ext_data_;}

    const PkdStruct* data() const  {return data_;}
    PkdStruct* data()  {return data_;}

    constexpr static size_t index_span() {
        return 256/8;
    }

    MyType index() const {
        return MyType(ext_data_, data_->index());
    }

    VoidResult reindex() {
        return detail::PkdDTBufOrderingDispatcher<MyType>::reindex(*this);
    }

    DataLengths data_lengts(size_t column, size_t row, size_t size) const
    {
        DataLengths lengths{};

        for_each_dimension([&](auto idx){
            data_->template dimension<idx>(column).lengths(lengths[idx], row, size);
        });

        return lengths;
    }

    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        const auto& meta = data_->metadata();

        handler->startStruct();
        handler->startGroup("DATA_TYPE_BUFFER");

        handler->value("SIZE", &meta.size());

        handler->startGroup("DATA", meta.size());

        pdtbuf_::DTBufferPrintHelper<
            typename PkdStructT::DataType,
            BlockValueProviderFactory,
            Columns
        >::handle(this, handler);

        handler->endGroup();

        handler->endGroup();
        handler->endStruct();
    }

    void check() const {
        return detail::PkdDTBufOrderingDispatcher<MyType>::check(*this);
    }

    size_t size() const {
        return data_->metadata().size();
    }

    size_t columns() const {
        return Columns;
    }

    size_t max_element_idx() const {
        return size() - 1;
    }

    ViewType access(size_t column, size_t row) const
    {
        DataDimensionsTuple data = access_data(column, row);
        return DataTypeTraits<DataType>::make_view(*ext_data_, data);
    }

    ViewType access(size_t row) const
    {
        DataDimensionsTuple data = access_data(0, row);
        return DataTypeTraits<DataType>::make_view(*ext_data_, data);
    }

    DataDimensionsTuple access_data(size_t column, size_t row) const
    {
        DataDimensionsTuple data{};

        for_each_dimension([&](auto idx){
            auto dimension = data_->template dimension<idx>(column);
            dimension.set(std::get<idx>(data), row);
        });

        return data;
    }



    /*********************** API *********************/
    VoidResult splitTo(MyType& other, size_t idx)
    {
        auto& meta = data_->metadata();

        //MEMORIA_ASSERT_RTN(other.size(), ==, 0);

        size_t split_size = meta.size() - idx;

        for (size_t column = 0; column < Columns; column++) {

            DataLengths data_lengths = this->data_lengts(column, idx, split_size);

            MEMORIA_TRY_VOID(other.insertSpace(column, 0, split_size, data_lengths));

            copyTo(other, column, idx, split_size, 0, data_lengths);
        }

        other.data()->metadata().size() += split_size;

        MEMORIA_TRY_VOID(removeSpace(idx, meta.size()));

        return VoidResult::of();
    }

    VoidResult mergeWith(MyType& other) const
    {
        size_t my_size     = this->size();
        size_t other_size  = other.size();

        for (size_t column = 0; column < Columns; column++)
        {
            DataLengths data_lengths = this->data_lengts(column, 0, my_size);

            MEMORIA_TRY_VOID(other.insertSpace(column, other_size, my_size, data_lengths));

            copyTo(other, column, 0, my_size, other_size, data_lengths);
        }

        other.data()->metadata().size() += my_size;

        return VoidResult::of();
    }

    VoidResult remove(size_t room_start, size_t room_end) {
        return removeSpace(room_start, room_end);
    }

    VoidResult removeSpace(size_t room_start, size_t room_end)
    {
        auto& meta = data_->metadata();
        size_t size = meta.size();
        size_t room_length = room_end - room_start;

        MEMORIA_ASSERT_RTN(room_start, <=, size);
        MEMORIA_ASSERT_RTN(room_end, <=, size);
        MEMORIA_ASSERT_RTN(room_start, <=, room_end);

        for (size_t column = 0; column < Columns; column++)
        {
            MEMORIA_TRY_VOID(for_each_dimension_res([&](auto dim_idx) {
                return data_->template dimension<dim_idx>(column).remove_space(
                    room_start, room_length
                );
            }));
        }

        meta.size() -= room_length;

        return VoidResult::of();
    }

    VoidResult clear()
    {
        MEMORIA_TRY_VOID(data_->init());

        if (data_->allocatable().has_allocator())
        {
            auto alloc = data_->allocatable().allocator();
            size_t empty_size = PkdStruct::empty_size();
            MEMORIA_TRY_VOID(alloc->resize_block(this, empty_size));
        }

        return VoidResult::of();
    }

    ViewType get_values(size_t idx) const  {
        return get_values(idx, 0);
    }

    ViewType get_values(size_t idx, size_t column) const  {
        return access(idx);
    }


    template <typename T>
    VoidResult setValues(size_t pos, const core::StaticVector<T, Columns>& values)
    {
        MEMORIA_TRY_VOID(removeSpace(pos, pos + 1));
        return insert(pos, values);
    }

    template <typename T>
    VoidResult insert(size_t pos, const core::StaticVector<T, Columns>& values)
    {
        return insert_from_fn(pos, 1, [&](size_t column, size_t row){
            return values[column];
        });
    }



    void configure_io_substream(io::IOSubstream& substream) const
    {
        auto& view = io::substream_cast<typename PkdStruct::IOSubstreamView>(substream);
        view.configure(*this);
    }



    // FIXME: Adapt to multicolumn!
    Int32Result insert_io_substream(size_t at, const io::IOSubstream& substream, size_t start, size_t size)
    {
        static_assert(Columns == 1, "");

        using IOBuffer = typename PkdStruct::GrowableIOSubstream;
        const IOBuffer& buffer = io::substream_cast<IOBuffer>(substream);

        DataLengths lengths = to_data_lengths(buffer.data_lengths(start, size));

        MEMORIA_TRY_VOID(insertSpace(0, at, size, lengths));

        data_->metadata().size() += size;

        for_each_dimension([&](auto dim_idx) {
            data_->template dimension<dim_idx>(0).copy_from_databuffer(
                    at, start, size, lengths[dim_idx], buffer
            );
        });

        return Int32Result::of(static_cast<int32_t>(at + size));
    }






    FindResult findGTForward(size_t column, const ViewType& val) const
    {
        return detail::PkdDTBufOrderingDispatcher<MyType>::find_fw_gt(*this, column, val);
    }

    FindResult findGTForward(size_t column, size_t start, const ViewType& val) const
    {
        return find_gt_fw_sum(column, start, val);
    }

    FindResult findGEForward(size_t column, const ViewType& val) const
    {
        return detail::PkdDTBufOrderingDispatcher<MyType>::find_fw_ge(*this, column, val);
    }

    FindResult findGEForward(size_t column, size_t start, const ViewType& val) const
    {
        return find_ge_fw_sum(column, start, val);
    }



    FindResult findGTBackward(size_t column, const ViewType& val) const
    {
        return find_gt_bw_sum(column, val);
    }

    FindResult findGTBackward(size_t column, size_t start, const ViewType& val) const
    {
        return find_gt_bw_sum(column, start, val);
    }

    FindResult findGEBackward(size_t column, const ViewType& val) const
    {
        return find_ge_bw_sum(column, val);
    }

    FindResult findGEBackward(size_t column, size_t start, const ViewType& val) const
    {
        return find_ge_bw_sum(column, start, val);
    }




    FindResult findForward(SearchType search_type, size_t column, const ViewType& val) const
    {
        if (search_type == SearchType::GT)
        {
            return findGTForward(column, val);
        }
        else {
            return findGEForward(column, val);
        }
    }


    FindResult findForward(SearchType search_type, size_t column, size_t start, const ViewType& val) const
    {
        if (search_type == SearchType::GT)
        {
            return findGTForward(column, start, val);
        }
        else {
            return findGEForward(column, start, val);
        }
    }


    FindResult findForward(SearchType search_type, size_t column, const Optional<ViewType>& val) const
    {
        if (search_type == SearchType::GT)
        {
            return findGTForward(column, val.get());
        }
        else {
            return findGEForward(column, val.get());
        }
    }

    ViewType sum(size_t column) const {
        return sum(size());
    }


    ViewType sum(size_t column, size_t row) const {
        return detail::PkdDTBufOrderingDispatcher<MyType>::sum(*this, column, row);
    }

    ViewType sum(size_t column, size_t start, size_t end) const
    {
        ViewType v2 = sum(column, end);
        ViewType v1 = sum(column, start);
        return v2 - v1;
    }

    size_t estimate_insert_upsize(const ViewType& view) const
    {
        auto& meta = data_->metadata();

        size_t total_size{};

        auto data = DataTypeTraits<DataType>::describe_data(&view);

        for_each_dimension([&](auto idx){
            total_size += data_->template dimension<idx>(0).estimate_insert_upsize(
                std::get<idx>(data), meta
            );
        });

        return total_size;
    }

    size_t estimate_replace_upsize(size_t idx, const ViewType& view) const
    {
        auto& meta = data_->metadata();

        size_t total_size{};

        auto data = DataTypeTraits<DataType>::describe_data(&view);

        for_each_dimension([&](auto dim_idx){
            total_size += data_->template dimension<dim_idx>(0).estimate_replace_upsize(
                idx, std::get<dim_idx>(data), meta
            );
        });

        return total_size;
    }

    VoidResult replace(size_t column, size_t idx, const ViewType& view)
    {
        DataDimensionsTuple data = DataTypeTraits<DataType>::describe_data(&view);

        MEMORIA_TRY_VOID(resize(idx, data));



        VoidResult res = for_each_dimension_res([&](auto dim_idx) {
            return data_->template dimension<dim_idx>(0).replace_row(
                idx, std::get<dim_idx>(data)
            );
        });

        MEMORIA_RETURN_IF_ERROR(res);

        return VoidResult::of();
    }

    VoidResult insert(int32_t idx, int32_t size, std::function<Values (size_t)> provider)
    {
        return insert_from_fn(idx, size, [&](size_t column, size_t row){
            return provider(row)[column];
        });
    }

    template <typename AccessorFn>
    VoidResult insert_entries(size_t row_at, size_t size, AccessorFn&& elements)
    {
        MEMORIA_ASSERT_RTN(row_at, <=, this->size());

        for (size_t column = 0; column < Columns; column++)
        {
            DataLengths data_lengths{};

            for (size_t row = 0; row < size; row++)
            {
                auto elem = elements(column, row);
                auto data = DataTypeTraits<DataType>::describe_data(&elem);

                for_each_dimension([&](auto dim_idx){
                    data_lengths[dim_idx] += data_length(std::get<dim_idx>(data));
                });
            }

            MEMORIA_TRY_VOID(insertSpace(column, row_at, size, data_lengths));

            for (size_t row = 0; row < size; row++)
            {
                auto elem = elements(0, row);
                auto data = DataTypeTraits<DataType>::describe_data(&elem);

                for_each_dimension([&](auto dim_idx){
                    data_->template dimension<dim_idx>(column).copy_from(
                        row_at + row, std::get<dim_idx>(data)
                    );
                });
            }
        }

        data_->metadata().size() += size;

        return VoidResult::of();
    }

    template <typename AccessorFn>
    VoidResult update_entries(size_t row_at, size_t size, AccessorFn&& elements)
    {
        MEMORIA_ASSERT_RTN(row_at, <=, this->size());

        MEMORIA_TRY_VOID(removeSpace(row_at, row_at + size));
        return insert_entries(row_at, size, std::forward<AccessorFn>(elements));
    }

    VoidResult remove_entries(size_t row_at, size_t size)
    {
        return removeSpace(row_at, row_at + size);
    }



    template <typename AccessorFn>
    VoidResult insert_from_fn(size_t row_at, size_t size, AccessorFn&& elements)
    {
        for (size_t column = 0; column < Columns; column++)
        {
            DataLengths data_lengths{};

            for (size_t row = 0; row < size; row++)
            {
                auto elem = elements(column, row);
                auto data = DataTypeTraits<DataType>::describe_data(&elem);

                for_each_dimension([&](auto dim_idx){
                    data_lengths[dim_idx] += data_length(std::get<dim_idx>(data));
                });
            }

            MEMORIA_TRY_VOID(insertSpace(column, row_at, size, data_lengths));

            for (size_t row = 0; row < size; row++)
            {
                auto elem = elements(column, row);
                auto data = DataTypeTraits<DataType>::describe_data(&elem);

                for_each_dimension([&](auto dim_idx){
                    data_->template dimension<dim_idx>(column).copy_from(
                        row_at + row, std::get<dim_idx>(data)
                    );
                });
            }
        }

        data_->metadata().size() += size;

        return VoidResult::of();
    }



    VoidResult insert(size_t idx, const ViewType& view)
    {
        static_assert(Columns == 1, "");

        core::StaticVector<ViewType, 1> vv;
        vv[0] = view;

        return insert(idx, vv);
    }

    VoidResult remove(size_t idx)
    {
        return removeSpace(idx, idx + 1);
    }


    using Value = ViewType;

    Value sum_for_rank(size_t start, size_t end, size_t symbol, SeqOpType seq_op) const {
        switch (seq_op) {
            case SeqOpType::EQ : return sum_for_rank_eq(start, end, symbol);
            case SeqOpType::NEQ: return sum_for_rank_neq(start, end, symbol);
            case SeqOpType::LT : return sum_for_rank_lt(start, end, symbol);
            case SeqOpType::LE : return sum_for_rank_le(start, end, symbol);
            case SeqOpType::GT : return sum_for_rank_gt(start, end, symbol);
            case SeqOpType::GE : return sum_for_rank_ge(start, end, symbol);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)seq_op).do_throw();
        }
    }

    struct SelectResult {
        size_t idx;
        size_t size;
        Value rank;

        bool is_end() const {return idx >= size;}
    };

    SelectResult find_for_select_fw(size_t start, Value rank, size_t symbol, SeqOpType seq_op) const {
        switch (seq_op) {
            case SeqOpType::EQ : return find_for_select_fw_eq(start, rank, symbol);
            case SeqOpType::NEQ: return find_for_select_fw_fn(start, rank, symbol, FindFwNEQFn());
            case SeqOpType::LT : return find_for_select_fw_fn(start, rank, symbol, FindFwLTFn());
            case SeqOpType::LE : return find_for_select_fw_fn(start, rank, symbol, FindFwLEFn());
            case SeqOpType::GT : return find_for_select_fw_fn(start, rank, symbol, FindFwGTFn());
            case SeqOpType::GE : return find_for_select_fw_fn(start, rank, symbol, FindFwGEFn());
            case SeqOpType::EQ_NLT : return find_for_select_fw_nlt(start, rank, symbol);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)seq_op).do_throw();
        }
    }

    SelectResult find_for_select_bw(size_t start, Value rank, size_t symbol, SeqOpType seq_op) const {
        switch (seq_op) {
            case SeqOpType::EQ : return find_for_select_bw_eq(start, rank, symbol);
            case SeqOpType::NEQ: return find_for_select_bw_fn(start, rank, symbol, FindFwNEQFn());
            case SeqOpType::LT : return find_for_select_bw_fn(start, rank, symbol, FindFwLTFn());
            case SeqOpType::LE : return find_for_select_bw_fn(start, rank, symbol, FindFwLEFn());
            case SeqOpType::GT : return find_for_select_bw_fn(start, rank, symbol, FindFwGTFn());
            case SeqOpType::GE : return find_for_select_bw_fn(start, rank, symbol, FindFwGEFn());
            case SeqOpType::EQ_NLT : return find_for_select_bw_nlt(start, rank, symbol);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)seq_op).do_throw();
        }
    }

    Value sum_for_rank_eq(size_t start, size_t end, size_t symbol) const {
        return this->sum(symbol, start, end);
    }

    Value sum_for_rank_neq(size_t start, size_t end, size_t symbol) const
    {
        Value val{};

        for (size_t c = 0; c < Columns; c++) {
            val += symbol != c ? this->sum(symbol, start, end) : 0;
        }

        return val;
    }

    Value sum_for_rank_lt(size_t start, size_t end, size_t symbol) const
    {
        Value val{};

        for (size_t c = 0; c < symbol; c++) {
            val += this->sum(symbol, start, end);
        }

        return val;
    }

    Value sum_for_rank_gt(size_t start, size_t end, size_t symbol) const
    {
        Value val{};

        for (size_t c = symbol + 1; c < Columns; c++) {
            val += this->sum(symbol, start, end);
        }

        return val;
    }

    Value sum_for_rank_le(size_t start, size_t end, size_t symbol) const
    {
        Value val{};

        for (size_t c = 0; c <= symbol; c++) {
            val += this->sum(symbol, start, end);
        }

        return val;
    }

    Value sum_for_rank_ge(size_t start, size_t end, size_t symbol) const
    {
        Value val{};

        for (size_t c = symbol; c < Columns; c++) {
            val += this->sum(symbol, start, end);
        }

        return val;
    }


    SelectResult find_for_select_fw_eq(size_t start, Value rank, size_t symbol) const
    {
        auto res = this->find_gt_fw_sum(symbol, start, rank);
        return SelectResult{res.idx(), this->size(), res.prefix()};
    }



    SelectResult find_for_select_fw_nlt(size_t start, Value rank, size_t symbol) const
    {
        auto res_eq = find_for_select_fw_eq(start, rank, symbol);
        if (symbol > 0)
        {
            auto res_lt = find_for_select_fw_fn(start, rank, symbol, FindFwLTFn());
            if (res_lt.idx < res_eq.idx) {
                return res_lt;
            }
        }

        return res_eq;
    }

    struct FindFwNEQFn {
        Value sum(const MyType& tree, size_t idx, size_t symbol) const {
            Value tmp{};
            for (size_t c = 0; c < Columns; c++) {
                tmp += c != symbol ? tree.value(c, idx) : 0;
            }
            return tmp;
        }
    };

    struct FindFwLTFn {
        Value sum(const MyType& tree, size_t idx, size_t symbol) const {
            Value tmp{};
            for (size_t c = 0; c < symbol; c++) {
                tmp += tree.value(c, idx);
            }
            return tmp;
        }
    };

    struct FindFwLEFn {
        Value sum(const MyType& tree, size_t idx, size_t symbol) const {
            Value tmp{};
            for (size_t c = 0; c <= symbol; c++) {
                tmp += tree.value(c, idx);
            }
            return tmp;
        }
    };

    struct FindFwGTFn {
        Value sum(const MyType& tree, size_t idx, size_t symbol) const {
            Value tmp{};
            for (size_t c = symbol + 1; c < Columns; c++) {
                tmp += tree.value(c, idx);
            }
            return tmp;
        }
    };

    struct FindFwGEFn {
        Value sum(const MyType& tree, size_t idx, size_t symbol) const {
            Value tmp{};
            for (size_t c = symbol; c < Columns; c++) {
                tmp += tree.value(c, idx);
            }
            return tmp;
        }
    };

    template <typename Fn>
    SelectResult find_for_select_fw_fn(size_t start, Value rank, size_t symbol, Fn&& fn) const
    {
        Value prefix{};
        size_t size = this->size();

        for (size_t c = start; c < size; c++)
        {
            Value tmp = fn.sum(*this, c, symbol);

            if (rank < prefix + tmp) {
                return SelectResult{c, size, prefix};
            }
            else {
                prefix += tmp;
            }
        }

        return SelectResult{size, size, prefix};
    }







    template <typename Fn>
    SelectResult find_for_select_bw_fn(size_t start, Value rank, size_t symbol, Fn&& fn) const
    {
        Value prefix{};
        size_t size = this->size();

        for (size_t c = start; c >= 0; c--)
        {
            Value tmp = fn.sum(*this, c, symbol);

            if (rank < prefix + tmp) {
                return SelectResult{c, size, prefix};
            }
            else {
                prefix += tmp;
            }
        }

        return SelectResult{size, size, prefix};
    }

    SelectResult find_for_select_bw_eq(size_t start, Value rank, size_t symbol) const
    {
        auto res = this->find_gt_bw_sum(symbol, start, rank);
        return SelectResult{res.idx(), this->size(), res.prefix()};
    }



    SelectResult find_for_select_bw_nlt(size_t start, Value rank, size_t symbol) const
    {
        auto res_eq = find_for_select_bw_eq(start, rank, symbol);
        if (symbol > 0)
        {
            auto res_lt = find_for_select_bw_fn(start, rank, symbol, FindFwLTFn());

            if ((!res_lt.is_end()) && res_lt.idx > res_eq.idx) {
                return res_lt;
            }
        }

        return res_eq;
    }


private:

    FindResult find_gt_fw_sum(size_t column, size_t start, const ViewType& val) const
    {
        Datum<DataType> pfx = sum_sum(column, start);
        Datum<DataType> tgt = pfx.view() + val;

        return find_gt_fw_sum(column, tgt.view()).sub_prefix(pfx.view());
    }

    FindResult find_ge_fw_sum(size_t column, size_t start, const ViewType& val) const
    {
        Datum<DataType> pfx = sum_sum(column, start);
        Datum<DataType> tgt = pfx.view() + val;

        return find_ge_fw_sum(column, tgt.view()).sub_prefix(pfx.view());
    }

    FindResult find_gt_bw_sum(size_t column, const ViewType& val) const
    {
        return find_gt_bw_sum(column, size() - 1, val);
    }

    FindResult find_gt_bw_sum(size_t column, size_t start, const ViewType& val) const
    {
        Datum<DataType> total = sum_sum(column, start + 1);

        if (val < total.view())
        {
            Datum<DataType> tgt = total.view() - val;
            return find_ge_fw_sum(column, tgt.view());
        }
        else {
            return FindResult(start + 1, total.view());
        }
    }

    FindResult find_ge_bw_sum(size_t column, const ViewType& val) const
    {
        return find_ge_bw_sum(column, size() - 1, val);
    }

    FindResult find_ge_bw_sum(size_t column, size_t start, const ViewType& val) const
    {
        Datum<DataType> total = sum_sum(column, start + 1);

        if (val <= total.view())
        {
            Datum<DataType> tgt = total.view() - val;
            return find_gt_fw_sum(column, tgt.view());
        }
        else {
            return FindResult(size(), total.view());
        }
    }




    FindResult find_gt_fw_max(size_t column, const ViewType& val) const
    {
        auto end = this->end(column);
        auto ii = std::upper_bound(begin(column), end, val);

        if (ii != end)
        {
            return ii.pos();
        }

        return FindResult(data_->size());
    }

    FindResult find_ge_fw_max(size_t column, const ViewType& val) const
    {
        auto end = this->end(column);
        auto ii = std::lower_bound(begin(column), end, val);

        if (ii != end)
        {
            return ii.pos();
        }

        return FindResult(data_->size());
    }


    FindResult find_ge_fw_sum(size_t column, const ViewType& val) const
    {
        size_t idx{};
        ViewType prefix{};

        if (data_->has_index())
        {
            auto index = this->index();

            FindResult res = index.find_ge_fw_sum(column, val);

            if (res.local_pos() < index.size()) {
                idx = res.local_pos() * index_span();
                prefix = res.prefix();
            }
            else {
                res.set_local_pos(this->size());
                return res;
            }
        }

        auto ii = begin(column, idx);
        auto end = this->end(column);

        while (ii != end)
        {
            ViewType vv = *ii;
            if (val <= prefix + vv) {
                return FindResult(idx, prefix);
            }

            prefix += vv;
            ++ii;
            ++idx;
        }

        return FindResult(data_->size(), prefix);
    }


    FindResult find_gt_fw_sum(size_t column, const ViewType& val) const
    {
        size_t idx{};
        ViewType prefix{};

        if (data_->has_index())
        {
            auto index = this->index();

            FindResult res = index.find_gt_fw_sum(column, val);

            if (res.local_pos() < index.size()) {
                idx = res.local_pos() * index_span();
                prefix = res.prefix();
            }
            else {
                res.set_local_pos(this->size());
                return res;
            }
        }

        auto ii = begin(column, idx);
        auto end = this->end(column);

        while (ii != end)
        {
            ViewType vv = *ii;
            if (val < prefix + vv) {
                return FindResult(idx, prefix);
            }

            prefix += vv;
            ++ii;
            ++idx;
        }

        return FindResult(data_->size(), prefix);
    }



    ViewType sum_gen(size_t column, size_t idx) const
    {
        MEMORIA_ASSERT(idx, <=, size());
        MEMORIA_ASSERT(column, <=, Columns);

        auto ii = begin(column, idx);
        auto end = this->end(column);

        ViewType sum{};

        while (ii != end)
        {
            ViewType vv = *ii;
            sum += vv;

            ++ii;
        }

        return sum;
    }


    ViewType sum_sum(size_t column, size_t idx) const
    {
        MEMORIA_ASSERT(idx, <=, size());
        MEMORIA_ASSERT(column, <=, Columns);

        ViewType sum{};
        size_t base{};

        if (data_->has_index())
        {
            auto index = this->index();

            size_t index_span = this->index_span();
            size_t span = idx / index_span;

            sum = index.sum(column, span);
            base = span * index_span;
        }

        auto ii = begin(column, base);
        for (; base < idx; base++, ++ii)
        {
            ViewType vv = *ii;
            sum += vv;
        }

        return sum;
    }


    void check_max() const
    {
        for (size_t c = 0; c < Columns; c++) {
            for (size_t idx = 1; idx < size(); idx++)
            {
                if (access(c, idx - 1) > access(c, idx)) {
                    return MEMORIA_MAKE_GENERIC_ERROR(
                        "Buffer's content is not in the proper order at position {}, column {}: '{}' > '{}' ",
                                idx,
                                c,
                                access(c, idx - 1),
                                access(c, idx)
                    ).do_throw();
                }
            }
        }
    }

    void check_sum() const
    {
        if (data_->has_index())
        {
            auto index = this->index();
            index.check();

            size_t index_span   = this->index_span();
            size_t size         = this->size();
            size_t spans        = div_up(size, index_span);

            for (size_t c = 0; c < Columns; c++)
            {
                size_t base{};
                for (size_t span = 0; span < spans; span++)
                {
                    size_t limit = (base + index_span) <= size ? base + index_span : size;

                    Datum<DataType> sum{};
                    for (size_t idx = base; idx < limit; idx++, base += index_span)
                    {
                        ViewType ee = access(c, idx - 1);
                        sum = sum.view() + ee;
                    }

                    ViewType iv = index.access(c, span);

                    if (iv != sum.view()) {
                        return MEMORIA_MAKE_GENERIC_ERROR(
                                    "Buffer's content mismatch with the index, column {}, idc_c {}: '{}' != '{}' ",
                                    c,
                                    span,
                                    sum.view(),
                                    iv
                        ).do_throw();
                    }
                }
            }
        }
    }

    VoidResult reindex_sum()
    {
        size_t size = this->size();
        size_t index_span = this->index_span();

        if (size > index_span)
        {
            size_t spans = div_up(size, index_span);

            std::vector<DataTypeBuffer<DataType>> columns(Columns);

            for (size_t c = 0; c < Columns; c++)
            {
                size_t base{};
                for (size_t span = 0; span < spans; span++)
                {
                    size_t limit = (base + index_span) <= size ? base + index_span : size;

                    Datum<DataType> sum{};
                    for (size_t idx = base; idx < limit; idx++, base += index_span)
                    {
                        ViewType ee = access(c, idx - 1);
                        sum = sum.view() + ee;
                    }

                    columns[c].append_value(sum.view());
                }
            }

            MEMORIA_TRY_VOID(data_->create_index());
            MyType index = this->index();

            auto res = index.insert_from_fn(0, spans, [&](size_t column, size_t row){
               return columns[column][row];
            });

            MEMORIA_RETURN_IF_ERROR(res);
        }
        else {
            MEMORIA_TRY_VOID(data_->remove_index());
        }

        return VoidResult::of();
    }

    template <typename T>
    static size_t data_length(const Span<const T>& span)  {
        return span.length();
    }

    template <typename T>
    static size_t data_length(const T*)  {
        return 1;
    }


    void copyTo(MyType& other, size_t column, size_t copy_from, size_t count, size_t copy_to, const DataLengths& data_lengths) const
    {
        for_each_dimension([&](auto idx){
            auto dimension = data_->template dimension<idx>(column);
            dimension.copy_to(other.data(), copy_from, count, copy_to, data_lengths[idx]);
        });
    }


    VoidResult insertSpace(size_t column, size_t idx, size_t room_length, const DataLengths& data_lengths)
    {
        auto& meta = data_->metadata();

        size_t size = meta.size();

        MEMORIA_ASSERT_RTN(idx, <=, size);

        size_t extra_data{};

        for_each_dimension([&](auto dim_idx) {
            extra_data += data_->template dimension<dim_idx>(column).compute_new_size(
                    room_length, data_lengths[dim_idx]
            );
        });

        MEMORIA_TRY_VOID(data_->resize(PkdStruct::base_size(extra_data)));

        MEMORIA_TRY_VOID(for_each_dimension_res([&](auto dim_idx) {
            return data_->template dimension<dim_idx>(column).insert_space(
                idx, room_length, data_lengths[dim_idx]
            );
        }));

        return VoidResult::of();
    }


    VoidResult resize(size_t idx, const DataDimensionsTuple& new_value_sizes)
    {
        auto& meta = data_->metadata();

        size_t size = meta.size();

        MEMORIA_ASSERT_RTN(idx, <=, size);

        return for_each_dimension_res([&](auto dim_idx) {
            return data_->template dimension<dim_idx>(0).resize_row(
                    idx, std::get<dim_idx>(new_value_sizes)
            );
        });
    }

    template <typename... Types>
    DataLengths to_data_lengths(const std::tuple<Types...>& tuple)
    {
        DataLengths lengths{};

        for_each_dimension([&](auto idx){
            lengths[idx] = std::get<idx>(tuple);
        });

        return lengths;
    }

    template <typename Fn>
    static void for_each_dimension(Fn&& fn) {
        ForEach<0, Dimensions>::process_fn(fn);
    }

    template <typename Fn>
    static VoidResult for_each_dimension_res(Fn&& fn) {
        return ForEach<0, Dimensions>::process_res_fn(fn);
    }
};




}
