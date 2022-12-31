
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

#include <memoria/core/packed/tools/packed_allocator.hpp>

#include <memoria/core/packed/datatype_buffer/packed_array_iterator.hpp>
#include <memoria/core/packed/datatype_buffer/packed_datatype_buffer_fse_tools.hpp>
#include <memoria/core/packed/datatype_buffer/packed_datatype_buffer_vle_tools.hpp>

#include <memoria/core/packed/datatype_buffer/packed_array_iterator.hpp>

#include <memoria/core/datatypes/buffer/buffer.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/memory/memory.hpp>

#include <memoria/api/common/ctr_batch_input.hpp>

#include <algorithm>

namespace memoria {

namespace detail {

    template <typename BufferSO, DTOrdering Ordering = BufferSO::Ordering>
    struct PkdDTBufOrderingDispatcher;

    template <typename BufferSO>
    struct PkdDTBufOrderingDispatcher<BufferSO, DTOrdering::UNORDERED> {
        static void check(const BufferSO& buffer) {}
        static void reindex(BufferSO& buffer) {
        }

        struct FindResult {};
        static size_t compute_index_block_size(size_t) {
            return 0;
        }
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
            size_t idx() const {return idx_;}


            void set_local_pos(size_t pos)  {
                idx_ = pos;
            }
        };


        static void check(const BufferSO& buffer){
            buffer.check_max();
        }

        static void reindex(BufferSO& buffer) {}

        static auto find_fw_gt(const BufferSO& buffer, size_t column, const ViewType& val) {
            return buffer.find_gt_fw_max(column, val);
        }

        static auto find_fw_ge(const BufferSO& buffer, size_t column, const ViewType& val) {
            return buffer.find_ge_fw_max(column, val);
        }

        static ViewType sum(const BufferSO& buffer, size_t column, size_t row) {
            return buffer.sum_gen(column, row);
        }

        static size_t compute_index_block_size(size_t elements) {
            return 0;
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

            const FindResult& add_prefix(const ViewType& view) {
                prefix_ += view;
                return *this;
            }

            size_t local_pos() const {return idx_;}
            size_t idx() const {return idx_;}

            void set_local_pos(size_t pos) {
                idx_ = pos;
            }

            void set_prefix(const ViewType& val) {
                prefix_ = val;
            }
        };



        static void check(const BufferSO& buffer) {
            buffer.check_sum();
        }

        static auto reindex(BufferSO& buffer) {
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

        static size_t compute_index_block_size(size_t elements) {
            return BufferSO::PkdStructT::compute_index_block_size(elements);
        }
    };


    template <typename StructSO, PackedDataTypeSize DatasizeType, DTOrdering Ordering>
    struct PkdDTBufDataSizeTypeDispatcher;

    template <typename StructSO>
    struct PkdDTBufDataSizeTypeDispatcherBase {
        using UpdateState = typename StructSO::UpdateState;

        template <typename Fn>
        static PkdUpdateStatus prepare_update(const StructSO&, size_t, size_t, UpdateState&, Fn&&) {
            return PkdUpdateStatus::SUCCESS;
        }

        static PkdUpdateStatus prepare_remove(const StructSO&, size_t, size_t, UpdateState&) {
            return PkdUpdateStatus::SUCCESS;
        }

        static auto commit_remove(StructSO& so, size_t start, size_t end, UpdateState&) {
            return so.do_commit_remove(start, end);
        }
    };

    template <typename StructSO, DTOrdering Ordering>
    struct PkdDTBufDataSizeTypeDispatcher<StructSO, PackedDataTypeSize::FIXED, Ordering>: PkdDTBufDataSizeTypeDispatcherBase<StructSO> {
        using UpdateState = typename StructSO::UpdateState;

        template <typename Fn>
        static auto commit_update(StructSO& so, size_t start, size_t size, UpdateState&, Fn&& fn) {
            return so.do_commit_update_fxd_max(start, size, std::forward<Fn>(fn));
        }

        template <typename Fn>
        static PkdUpdateStatus prepare_insert(const StructSO& so, size_t start, size_t size, UpdateState& update_state, Fn&& fn) {
            return so.do_prepare_insert_fxd_max(start, size, update_state, std::forward<Fn>(fn));
        }

        template <typename Fn>
        static auto commit_insert(StructSO& so, size_t start, size_t size, UpdateState& update_state, Fn&& fn) {
            return so.do_commit_update_fxd_max(start, size, update_state, std::forward<Fn>(fn));
        }

        static PkdUpdateStatus prepare_merge_with(const StructSO& so, const StructSO& other, UpdateState& update_state) {
            return so.do_prepare_merge_with_fxd(other, update_state);
        }
    };


    template <typename StructSO>
    struct PkdDTBufDataSizeTypeDispatcher<StructSO, PackedDataTypeSize::FIXED, DTOrdering::SUM>: PkdDTBufDataSizeTypeDispatcherBase<StructSO> {
        using UpdateState = typename StructSO::UpdateState;

        template <typename Fn>
        static auto commit_update(StructSO& so, size_t start, size_t size, UpdateState&, Fn&& fn) {
            return so.do_commit_update_fxd_sum(start, size, std::forward<Fn>(fn));
        }

        template <typename Fn>
        static PkdUpdateStatus prepare_insert(const StructSO& so, size_t start, size_t size, UpdateState& update_state, Fn&& fn) {
            return so.do_prepare_insert_fxd_sum(start, size, update_state, std::forward<Fn>(fn));
        }

        template <typename Fn>
        static auto commit_insert(StructSO& so, size_t start, size_t size, UpdateState& update_state, Fn&& fn) {
            return so.do_commit_insert_fxd_sum(start, size, update_state, std::forward<Fn>(fn));
        }

        static PkdUpdateStatus prepare_merge_with(const StructSO& so, const StructSO& other, UpdateState& update_state) {
            return so.do_prepare_merge_with_fxd(other, update_state);
        }
    };



    template <typename StructSO, DTOrdering Ordering>
    struct PkdDTBufDataSizeTypeDispatcher<StructSO, PackedDataTypeSize::VARIABLE, Ordering>: PkdDTBufDataSizeTypeDispatcherBase<StructSO> {
        using UpdateState = typename StructSO::UpdateState;

        template <typename Fn>
        static auto commit_update(StructSO& so, size_t start, size_t size, UpdateState& update_state, Fn&& fn) {
            return so.do_commit_update_var_max(start, size, std::forward<Fn>(fn));
        }

        template <typename Fn>
        static PkdUpdateStatus prepare_update(const StructSO& so, size_t start, size_t size, UpdateState& update_state, Fn&& fn) {
            return so.do_prepare_update_var_max(start, size, update_state, std::forward<Fn>(fn));
        }

        template <typename Fn>
        static PkdUpdateStatus prepare_insert(const StructSO& so, size_t start, size_t size, UpdateState& update_state, Fn&& fn) {
            return so.do_prepare_insert_var_max(start, size, update_state, std::forward<Fn>(fn));
        }

        template <typename Fn>
        static auto commit_insert(StructSO& so, size_t start, size_t size, UpdateState& update_state, Fn&& fn) {
            return so.do_commit_insert_var_max(start, size, update_state, std::forward<Fn>(fn));
        }

        static PkdUpdateStatus prepare_merge_with(const StructSO& so, const StructSO& other, UpdateState& update_state) {
            return so.do_prepare_merge_with_var(other, update_state);
        }
    };


    template <typename StructSO>
    struct PkdDTBufDataSizeTypeDispatcher<StructSO, PackedDataTypeSize::VARIABLE, DTOrdering::SUM>: PkdDTBufDataSizeTypeDispatcherBase<StructSO> {
        using UpdateState = typename StructSO::UpdateState;

        template <typename Fn>
        static auto commit_update(StructSO& so, size_t start, size_t size, UpdateState& update_state, Fn&& fn) {
            return so.do_commit_update_var_sum(start, size, update_state, std::forward<Fn>(fn));
        }

        template <typename Fn>
        static PkdUpdateStatus prepare_insert(const StructSO& so, size_t start, size_t size, UpdateState& update_state, Fn&& fn) {
            return so.do_prepare_insert_var_sum(start, size, update_state, std::forward<Fn>(fn));
        }

        template <typename Fn>
        static auto commit_insert(StructSO& so, size_t start, size_t size, UpdateState& update_state, Fn&& fn) {
            return so.do_commit_insert_var_sum(start, size, update_state, std::forward<Fn>(fn));
        }

        static PkdUpdateStatus prepare_merge_with(const StructSO& so, const StructSO& other, UpdateState& update_state) {
            return so.do_prepare_merge_with_var(other, update_state);
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

    template <typename, PackedDataTypeSize, DTOrdering>
    friend struct detail::PkdDTBufDataSizeTypeDispatcher;

    template <typename>
    friend struct detail::PkdDTBufDataSizeTypeDispatcherBase;

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

    using UpdateState = PkdStructEmptyUpdate<MyType>;

    static constexpr PackedDataTypeSize DataTypeSize = pdtbuf_::BufferSizeTypeSelector<
        typename DataTypeTraits<DataType>::DataDimensionsList
    >::DataTypeSize;

    static_assert (Ordering == DTOrdering::SUM ? DataTypeSize == PackedDataTypeSize::FIXED : true,
        "VAR datatypes do not yet supported for SUM buffers");


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
        return PkdStruct::IndexSpan;
    }

    MyType index() const {
        return MyType(ext_data_, data_->index());
    }

    void reindex() {
        return detail::PkdDTBufOrderingDispatcher<MyType>::reindex(*this);
    }

    bool check_capacity(size_t size) const {
        return true;
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

        if (data_->has_index()) {
            handler->startGroup("INDEX");
            index().generateDataEvents(handler);
            handler->endGroup();
        }

        handler->value("SIZE", &meta.size0());

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
        data_->check_blocks();
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

    // Only for FixedSize buffers
    Span<ViewType> span(size_t column)
    {
        auto dim = data_->template dimension<0>(column);
        ViewType* data = dim.data();
        return Span<ViewType>(data, size());
    }

    // Only for FixedSize buffers
    Span<const ViewType> span(size_t column) const
    {
        auto dim = data_->template dimension<0>(column);
        const ViewType* data = dim.data();
        return Span<const ViewType>(data, size());
    }

    ViewType access(size_t column, size_t row) const
    {
        DataDimensionsTuple data = access_data(column, row);
        return DataTypeTraits<DataType>::make_view(*ext_data_, data);
    }

    ViewType value(size_t column, size_t row) const {
        return access(column, row);
    }

    core::StaticVector<ViewType, Columns> access(size_t row) const
    {
        core::StaticVector<ViewType, Columns> vv;

        for (size_t c = 0; c < Columns; c++)
        {
            DataDimensionsTuple data = access_data(c, row);
            vv[c] = DataTypeTraits<DataType>::make_view(*ext_data_, data);
        }

        return vv;
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
    void split_to(MyType& other, size_t idx)
    {
        auto& meta = data_->metadata();

        size_t split_size = meta.size() - idx;

        for (size_t column = 0; column < Columns; column++)
        {
            DataLengths data_lengths = this->data_lengts(column, idx, split_size);
            other.insertSpace(column, 0, split_size, data_lengths);
            copyTo(other, column, idx, split_size, 0, data_lengths);
        }

        other.data()->metadata().add_size(split_size);
        other.reindex();

        UpdateState ss;
        commit_remove(idx, meta.size(), ss, true);
    }

    PkdUpdateStatus prepare_merge_with(const MyType& other, UpdateState& update_state) const {
        return detail::PkdDTBufDataSizeTypeDispatcher<MyType, DataTypeSize, Ordering>::
                prepare_merge_with(*this, other, update_state);
    }

    PkdUpdateStatus do_prepare_merge_with_fxd(const MyType& other, UpdateState& update_state) const
    {
        size_t my_size = size();
        size_t other_size = other.size();

        size_t required_block_size = PkdStruct::compute_block_size(my_size + other_size);
        size_t existing_block_size = other.data()->block_size();

        return update_state.allocator_state()->inc_allocated(existing_block_size, required_block_size);
    }

    PkdUpdateStatus do_prepare_merge_with_var(const MyType& other, UpdateState& update_state) const
    {
        size_t mem_size{};
        for_each_dimension([&](auto dim_idx) {
            for (size_t column = 0; column < Columns; column++)
            {
                size_t size = data_->template dimension<dim_idx>(column).compute_dimension_size_for_merge(
                    other.data()->template dimension<dim_idx>(column), data()->metadata(), other.data()->metadata()
                );

                mem_size += size;
            }
        });

        size_t index_block_size = compute_index_block_size(other.size() + this->size());
        size_t required_block_size = PkdStruct::base_size(mem_size + index_block_size);
        size_t existing_block_size = other.data()->block_size();

        return update_state.allocator_state()->inc_allocated(existing_block_size, required_block_size);
    }


    void commit_merge_with(MyType& other, UpdateState& update_state) const
    {
        size_t my_size     = this->size();
        size_t other_size  = other.size();

        for (size_t column = 0; column < Columns; column++)
        {
            DataLengths data_lengths = this->data_lengts(column, 0, my_size);
            other.insertSpace(column, other_size, my_size, data_lengths);
            copyTo(other, column, 0, my_size, other_size, data_lengths);
        }

        other.data()->metadata().add_size(my_size);
        other.reindex();
    }


    PkdUpdateStatus prepare_remove(size_t start, size_t end, UpdateState& update_state) const
    {
        return detail::PkdDTBufDataSizeTypeDispatcher<MyType, DataTypeSize, Ordering>::
                prepare_remove(*this, start, end, update_state);
    }

    PkdUpdateStatus do_prepare_remove(size_t start, size_t end, UpdateState& update_state) const
    {
        MEMORIA_ASSERT(start, <=, this->size());
        MEMORIA_ASSERT(end, <=, this->size());
        MEMORIA_ASSERT(start, <=, end);

        size_t data_size{};

        for (size_t column = 0; column < 1; column++)
        {
            for_each_dimension([&](auto dim_idx){
                data_size += data_->template dimension<dim_idx>(column).compute_dimension_size_for_remove(
                    start, end, data_->metadata()
                );
            });
        }

        size_t size = end - start;
        size_t index_block_size = compute_index_block_size(this->size() - size);
        size_t required_block_size = PkdStruct::base_size(data_size + index_block_size);
        size_t existing_block_size = data_->block_size();

        return update_state.allocator_state()->inc_allocated(existing_block_size, required_block_size);
    }

    void commit_remove(size_t room_start, size_t room_end, UpdateState&, bool do_reindex = true)
    {
        auto& meta = data_->metadata();
        size_t size = meta.size();
        size_t room_length = room_end - room_start;

        MEMORIA_ASSERT(room_start, <=, size);
        MEMORIA_ASSERT(room_end, <=, size);
        MEMORIA_ASSERT(room_start, <=, room_end);

        for (size_t column = 0; column < Columns; column++)
        {
            for_each_dimension([&](auto dim_idx) {
                return data_->template dimension<dim_idx>(column).remove_space(
                    room_start, room_length
                );
            });
        }

        meta.sub_size(room_length);

        if (do_reindex) {
            reindex();
        }
    }

    void clear()
    {
        data_->init();

        if (data_->allocatable().has_allocator())
        {
            auto alloc = data_->allocatable().allocator();
            size_t empty_size = PkdStruct::empty_size();
            alloc->resize_block(this, empty_size);
        }
    }

    template <typename T>
    void insert(size_t pos, const core::StaticVector<T, Columns>& values)
    {
        return insert_from_fn(pos, 1, [&](size_t column, size_t row){
            return values[column];
        });
    }

    template <typename DT>
    PkdUpdateStatus prepare_insert_io_substream(size_t at, const HermesDTBuffer<DT>& buffer, size_t start, size_t size, UpdateState& update_state)
    {
        static_assert(Columns == 1, "");
        MEMORIA_ASSERT(at, <=, this->size());

        size_t data_size{};

        for (size_t column = 0; column < 1; column++)
        {
            DataLengths data_lengths = to_data_lengths(buffer.data_lengths(start, size));

            for_each_dimension([&](auto dim_idx){
                data_size += data_->template dimension<dim_idx>(column).compute_dimension_size_for_insert(
                    size, data_lengths[dim_idx], data_->metadata()
                );
            });
        }

        size_t index_block_size = compute_index_block_size(size + this->size());
        size_t required_block_size = PkdStruct::base_size(data_size + index_block_size);
        size_t existing_block_size = data_->block_size();

        return update_state.allocator_state()->inc_allocated(existing_block_size, required_block_size);
    }

    template <typename DT>
    size_t commit_insert_io_substream(size_t at, const HermesDTBuffer<DT>& buffer, size_t start, size_t size, UpdateState&)
    {
        static_assert(Columns == 1, "");
        MEMORIA_ASSERT(at, <=, this->size());

        DataLengths lengths = to_data_lengths(buffer.data_lengths(start, size));

        insertSpace(0, at, size, lengths);

        for_each_dimension([&](auto dim_idx) {
            data_->template dimension<dim_idx>(0).copy_from_databuffer(
                    at, start, size, lengths[dim_idx], buffer
            );
        });

        data_->metadata().add_size(size);

        reindex();

        return static_cast<size_t>(at + size);
    }

    template <typename DT>
    PkdUpdateStatus prepare_insert_io_substream(size_t at, const DataTypeBuffer<DT>& buffer, size_t start, size_t size, UpdateState& update_state)
    {
        static_assert(Columns == 1, "");
        MEMORIA_ASSERT(at, <=, this->size());

        size_t data_size{};

        for (size_t column = 0; column < 1; column++)
        {
            DataLengths data_lengths = to_data_lengths(buffer.data_lengths(start, size));

            for_each_dimension([&](auto dim_idx){
                data_size += data_->template dimension<dim_idx>(column).compute_dimension_size_for_insert(
                    size, data_lengths[dim_idx], data_->metadata()
                );
            });
        }

        size_t index_block_size = compute_index_block_size(size + this->size());
        size_t required_block_size = PkdStruct::base_size(data_size + index_block_size);
        size_t existing_block_size = data_->block_size();

        return update_state.allocator_state()->inc_allocated(existing_block_size, required_block_size);
    }


    // FIXME: Adapt to multicolumn!
    template <typename DT>
    size_t commit_insert_io_substream(size_t at, const DataTypeBuffer<DT>& buffer, size_t start, size_t size, UpdateState&)
    {
        static_assert(Columns == 1, "");
        MEMORIA_ASSERT(at, <=, this->size());

        DataLengths lengths = to_data_lengths(buffer.data_lengths(start, size));

        insertSpace(0, at, size, lengths);

        for_each_dimension([&](auto dim_idx) {
            data_->template dimension<dim_idx>(0).copy_from_databuffer(
                    at, start, size, lengths[dim_idx], buffer
            );
        });

        data_->metadata().add_size(size);

        reindex();

        return static_cast<size_t>(at + size);
    }






    FindResult findGTForward(size_t column, const ViewType& val) const
    {
        return detail::PkdDTBufOrderingDispatcher<MyType>::find_fw_gt(*this, column, val);
    }

    FindResult find_fw_gt(size_t column, const ViewType& val) const
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

    FindResult find_fw_ge(size_t column, const ViewType& val) const
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
        return sum(column, size());
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


    template <typename AccessorFn>
    PkdUpdateStatus prepare_insert(size_t row_at, size_t size, UpdateState& update_state, AccessorFn&& elements) const
    {
        MEMORIA_ASSERT(row_at, <=, this->size());
        size_t data_size{};

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

            for_each_dimension([&](auto dim_idx){
                data_size += data_->template dimension<dim_idx>(column).compute_dimension_size_for_insert(
                    size, data_lengths[dim_idx], data_->metadata()
                );
            });
        }

        size_t index_block_size = compute_index_block_size(size + this->size());
        size_t required_block_size = PkdStruct::base_size(data_size + index_block_size);
        size_t existing_block_size = data_->block_size();

        return update_state.allocator_state()->inc_allocated(existing_block_size, required_block_size);
    }

    void commit_insert(size_t idx, size_t size, UpdateState& update_state, std::function<Values (size_t)> provider)
    {
        return insert_from_fn(idx, size, [&](size_t column, size_t row){
            return provider(row)[column];
        });
    }

    template <typename AccessorFn>
    void commit_insert(size_t row_at, size_t size, UpdateState& update_state, AccessorFn&& elements)
    {
        MEMORIA_ASSERT(row_at, <=, this->size());
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

            insertSpace(column, row_at, size, data_lengths);

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

        data_->metadata().add_size(size);
        reindex();
    }


    template <typename AccessorFn>
    PkdUpdateStatus prepare_update(size_t row_at, size_t size, UpdateState& update_state, AccessorFn&& fn) const {
        return detail::PkdDTBufDataSizeTypeDispatcher<MyType, DataTypeSize, Ordering>::
                prepare_update(*this, row_at, size, update_state, std::forward<AccessorFn>(fn));
    }

    template <typename AccessorFn>
    PkdUpdateStatus do_prepare_update_var_max(size_t row_at, size_t size, UpdateState& update_state, AccessorFn&& elements) const
    {
        MEMORIA_ASSERT(row_at + size, <=, this->size());
        size_t data_size{};

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

            for_each_dimension([&](auto dim_idx){
                data_size += data_->template dimension<dim_idx>(column).compute_dimension_size_for_update(
                    row_at, size, data_lengths[dim_idx], data_->metadata()
                );
            });
        }

        size_t required_block_size = PkdStruct::base_size(data_size);
        size_t existing_block_size = data_->block_size();

        return update_state.allocator_state()->inc_allocated(existing_block_size, required_block_size);
    }

    template <typename AccessorFn>
    void commit_update(size_t row_at, size_t size, UpdateState& update_state, AccessorFn&& elements)
    {
        MEMORIA_ASSERT(row_at + size, <=, this->size());

        return detail::PkdDTBufDataSizeTypeDispatcher<MyType, DataTypeSize, Ordering>::
                commit_update(*this, row_at, size, update_state, std::forward<AccessorFn>(elements));
    }


    template <typename AccessorFn>
    void do_commit_update_fxd_sum(size_t row_at, size_t size, AccessorFn&& elements)
    {
        do_commit_update_fxd_max(row_at, size, std::forward<AccessorFn>(elements));
        reindex();
    }

    template <typename AccessorFn>
    void do_commit_update_fxd_max(size_t row_at, size_t size, AccessorFn&& elements)
    {
        for (size_t column = 0; column < Columns; column++) {
            for (size_t row = 0; row < size; row++)
            {
                auto elem = elements(column, row);
                auto data = DataTypeTraits<DataType>::describe_data(&elem);

                for_each_dimension([&](auto dim_idx){
                    data_->template dimension<dim_idx>(column).replace_row(
                        row_at + row, std::get<dim_idx>(data)
                    );
                });
            }
        }
    }

    template <typename AccessorFn>
    void do_commit_update_var_max(size_t row_at, size_t size, AccessorFn&& elements)
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

            for_each_dimension([&](auto dim_idx){
                data_->template dimension<dim_idx>(column).resize_block(
                    row_at, size, data_lengths[dim_idx]
                );
            });

            for_each_dimension([&](auto dim_idx){
                auto dim = data_->template dimension<dim_idx>(column);

                for (size_t row = 0; row < size; row++)
                {
                    auto elem = elements(column, row);
                    auto data = DataTypeTraits<DataType>::describe_data(&elem);

                    dim.replace_row(
                        row_at + row, std::get<dim_idx>(data)
                    );
                }
            });
        }
    }


    template <typename AccessorFn>
    void insert_from_fn(size_t row_at, size_t size, AccessorFn&& elements)
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

            insertSpace(column, row_at, size, data_lengths);

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

        data_->metadata().add_size(size);
        reindex();
    }


    void insert(size_t idx, const ViewType& view)
    {
        static_assert(Columns == 1, "");

        core::StaticVector<ViewType, 1> vv;
        vv[0] = view;

        return insert(idx, vv);
    }

    void remove(size_t idx)
    {
        UpdateState ss;
        return commit_remove(idx, idx + 1, ss);
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
            val += symbol != c ? this->sum(c, start, end) : 0;
        }

        return val;
    }

    Value sum_for_rank_lt(size_t start, size_t end, size_t symbol) const
    {
        Value val{};

        for (size_t c = 0; c < symbol; c++) {
            val += this->sum(c, start, end);
        }

        return val;
    }

    Value sum_for_rank_gt(size_t start, size_t end, size_t symbol) const
    {
        Value val{};

        for (size_t c = symbol + 1; c < Columns; c++) {
            val += this->sum(c, start, end);
        }

        return val;
    }

    Value sum_for_rank_le(size_t start, size_t end, size_t symbol) const
    {
        Value val{};

        for (size_t c = 0; c <= symbol; c++) {
            val += this->sum(c, start, end);
        }

        return val;
    }

    Value sum_for_rank_ge(size_t start, size_t end, size_t symbol) const
    {
        Value val{};

        for (size_t c = symbol; c < Columns; c++) {
            val += this->sum(c, start, end);
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

        for (size_t c = start + 1; c > 0; c--)
        {
            Value tmp = fn.sum(*this, c - 1, symbol);

            if (rank < prefix + tmp) {
                return SelectResult{c - 1, size, prefix};
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

    MMA_MAKE_UPDATE_STATE_METHOD

private:

    FindResult find_gt_fw_sum(size_t column, size_t start, const ViewType& val) const
    {
        auto pfx = sum_sum(column, start);
        auto tgt = pfx + val;

        return find_gt_fw_sum(column, tgt).sub_prefix(pfx);
    }

    FindResult find_ge_fw_sum(size_t column, size_t start, const ViewType& val) const
    {
        auto pfx = sum_sum(column, start);
        auto tgt = pfx + val;

        return find_ge_fw_sum(column, tgt).sub_prefix(pfx);
    }

    FindResult find_gt_bw_sum(size_t column, const ViewType& val) const
    {
        return find_gt_bw_sum(column, size() - 1, val);
    }

    FindResult find_gt_bw_sum(size_t column, size_t start, const ViewType& val) const
    {
        auto total = sum_sum(column, start + 1);

        if (val < total)
        {
            auto tgt = total - val;
            FindResult res =  find_ge_fw_sum(column, tgt);

            auto prefix = sum_sum(column, res.local_pos() + 1);

            res.set_prefix(total - prefix);

            return res;
        }
        else {
            return FindResult(start + 1, total);
        }
    }

    FindResult find_ge_bw_sum(size_t column, const ViewType& val) const
    {
        return find_ge_bw_sum(column, size() - 1, val);
    }

    FindResult find_ge_bw_sum(size_t column, size_t start, const ViewType& val) const
    {
        auto total = sum_sum(column, start + 1);

        if (val <= total)
        {
            auto tgt = total - val;
            FindResult res = find_gt_fw_sum(column, tgt);

            auto prefix = sum_sum(column, res.local_pos() + 1);

            res.set_prefix(total - prefix);

            return res;
        }
        else {
            return FindResult(start + 1, total);
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

            if (index.size() == 0) {
                return;
            }

            size_t index_span   = this->index_span();
            size_t size         = this->size();
            size_t spans        = div_up(size, index_span);

            for (size_t c = 0; c < Columns; c++)
            {
                size_t base{};
                for (size_t span = 0; span < spans; span++, base += index_span)
                {
                    size_t limit = (base + index_span) <= size ? base + index_span : size;

                    DTTViewType<DataType> sum{};
                    for (size_t idx = base; idx < limit; idx++)
                    {
                        ViewType ee = access(c, idx);
                        sum = sum + ee;
                    }

                    ViewType iv = index.access(c, span);

                    if (iv != sum) {
                        MEMORIA_MAKE_GENERIC_ERROR(
                                    "Buffer's content mismatch with the index, column {}, idc_c {}: '{}' != '{}' ",
                                    c,
                                    span,
                                    sum,
                                    iv
                        ).do_throw();
                    }
                }
            }
        }
    }

    void reindex_sum()
    {
        size_t size = this->size();
        size_t index_span = this->index_span();

        if (size > index_span)
        {
            size_t spans = div_up(size, index_span);

            std::vector<
                    IterSharedPtr<DataTypeBuffer<DataType>>
            > columns(Columns);

            for (size_t c = 0; c < Columns; c++)
            {
                columns[c] = TL_get_reusable_shared_instance<DataTypeBuffer<DataType>>();

                size_t base{};
                for (size_t span = 0; span < spans; span++, base += index_span)
                {
                    size_t limit = (base + index_span) <= size ? base + index_span : size;

                    DTTViewType<DataType> sum{};
                    for (size_t idx = base; idx < limit; idx++)
                    {
                        ViewType ee = access(c, idx);
                        sum = sum + ee;
                    }

                    columns[c]->append(sum);
                }
            }

            data_->create_index();
            MyType index = this->index();

            index.insert_from_fn(0, spans, [&](size_t column, size_t row){
                return columns[column]->operator[](row);
            });
        }
        else {
            data_->remove_index();
        }
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

    void insertSpace(size_t column, size_t idx, size_t room_length, const DataLengths& data_lengths)
    {
        auto& meta = data_->metadata();

        size_t size = meta.size();
        MEMORIA_ASSERT(idx, <=, size);

        for_each_dimension([&](auto dim_idx) {
            return data_->template dimension<dim_idx>(column).insert_space(
                idx, room_length, data_lengths[dim_idx]
            );
        });
    }


    void resize(size_t idx, const DataDimensionsTuple& new_value_sizes)
    {
        auto& meta = data_->metadata();

        size_t size = meta.size();

        MEMORIA_ASSERT(idx, <=, size);

        return for_each_dimension([&](auto dim_idx) {
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

    static size_t compute_index_block_size(size_t capacity) {
        return detail::PkdDTBufOrderingDispatcher<MyType>::compute_index_block_size(capacity);
    }
};


}
