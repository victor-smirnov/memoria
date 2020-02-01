
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

#include <memoria/core/packed/array/packed_array_iterator.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/iovector/io_substream_base.hpp>

#include <memoria/core/datatypes/buffer/buffer_generic.hpp>

#include <memoria/core/packed/datatype_buffer/packed_datatype_buffer_fse_tools.hpp>
#include <memoria/core/packed/datatype_buffer/packed_datatype_buffer_vle_tools.hpp>

#include <memoria/core/packed/array/packed_array_iterator.hpp>

#include <algorithm>

namespace memoria {

template <typename ExtData, typename PkdStruct>
class PackedDataTypeBufferSO {
    const ExtData* ext_data_;
    PkdStruct* data_;

    using MyType = PackedDataTypeBufferSO;

public:
    static constexpr psize_t Columns = 1;
    static constexpr int32_t Dimensions = PkdStruct::Dimensions;
    static constexpr int32_t Indexes = PkdStruct::Indexes;

    using ViewType      = typename PkdStruct::ViewType;
    using DataType      = typename PkdStruct::DataType;
    using Accessor      = pdtbuf_::PkdBufViewAccessor<PackedDataTypeBufferSO>;
    using DataDimensionsTuple = DTTDataDimensionsTuple<DataType>;

    using ConstIterator = PkdRandomAccessIterator<Accessor>;

    using PkdStructT = PkdStruct;

    using DataLengths = core::StaticVector<psize_t, Dimensions>;

    using Values = core::StaticVector<ViewType, Columns>;


    PackedDataTypeBufferSO(): ext_data_(), data_() {}
    PackedDataTypeBufferSO(const ExtData* ext_data, PkdStruct* data):
        ext_data_(ext_data), data_(data)
    {}


    void setup() {
        ext_data_ = nullptr;
        data_ = nullptr;
    }

    void setup(const ExtData* ext_data, PkdStruct* data) {
        ext_data_ = ext_data;
        data_ = data;
    }

    void setup(const ExtData* ext_data) {
        ext_data_ = ext_data;
    }

    void setup(PkdStruct* data) {
        data_ = data;
    }

    ConstIterator begin(psize_t column = 0) const {
        return ConstIterator(Accessor(*this), 0, data_->size());
    }

    ConstIterator end(psize_t column = 0) const
    {
        psize_t size = data_->size();
        return ConstIterator(Accessor(*this), size, size);
    }

    operator bool() const {
        return data_ != nullptr;
    }

    const ExtData* ext_data() const {return ext_data_;}
    const PkdStruct* data() const {return data_;}
    PkdStruct* data() {return data_;}

    OpStatus reindex() {
        return OpStatus::OK;
    }

    DataLengths data_lengts(psize_t row, psize_t size) const
    {
        DataLengths lengths{};

        for_each_dimension([&](auto idx){
            data_->template dimension<idx>().lengths(lengths[idx], row, size);
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

        for (int32_t c = 0; c < meta.size(); c++)
        {
            handler->value("VALUES", BlockValueProviderFactory::provider(Columns, [&](int32_t column) {
                return access(c);
            }));
        }

        handler->endGroup();

        handler->endGroup();
        handler->endStruct();
    }

    void check() const {}

    psize_t size() const {
        return data_->metadata().size();
    }

    ViewType access(psize_t column, psize_t row) const
    {
        return access(row);
    }

    ViewType access(psize_t row) const
    {
        DataDimensionsTuple data = access_data(row);
        return DataTypeTraits<DataType>::make_view(*ext_data_, data);
    }

    DataDimensionsTuple access_data(psize_t row) const
    {
        DataDimensionsTuple data{};

        for_each_dimension([&](auto idx){
            auto dimension = data_->template dimension<idx>();
            dimension.set(std::get<idx>(data), row);
        });

        return data;
    }



    /*********************** API *********************/



    OpStatus splitTo(MyType& other, psize_t idx)
    {
        auto& meta = data_->metadata();
        MEMORIA_V1_ASSERT(other.size(), ==, 0);

        psize_t split_size = meta.size() - idx;
        DataLengths data_lengths = this->data_lengts(idx, split_size);

        if(isFail(other.insertSpace(0, split_size, data_lengths))) {
            return OpStatus::FAIL;
        }

        copyTo(other, idx, split_size, 0, data_lengths);

        if(isFail(removeSpace(idx, meta.size()))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }

    OpStatus mergeWith(MyType& other)
    {
        psize_t my_size     = this->size();
        psize_t other_size  = other.size();

        DataLengths data_lengths = this->data_lengts(0, other_size);

        if(isFail(other.insertSpace(other_size, my_size, data_lengths))) {
            return OpStatus::FAIL;
        }

        copyTo(other, 0, my_size, other_size, data_lengths);

        if(isFail(removeSpace(0, my_size))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }

    OpStatus removeSpace(psize_t room_start, psize_t room_end)
    {
        auto& meta = data_->metadata();
        psize_t size = meta.size();
        psize_t room_length = room_end - room_start;

        MEMORIA_V1_ASSERT(room_start, <=, size);
        MEMORIA_V1_ASSERT(room_end, <=, size);
        MEMORIA_V1_ASSERT(room_start, <=, room_end);

        OpStatus status{OpStatus::OK};

        for_each_dimension([&](auto dim_idx) {
            if (!isFail(status))
            {
                status = data_->template dimension<dim_idx>().remove_space(
                    room_start, room_length
                );
            }
        });

        meta.size() -= room_length;

        return status;
    }

    ViewType get_values(psize_t idx) const {
        return get_values(idx, 0);
    }

    ViewType get_values(psize_t idx, psize_t column) const {
        return access(idx);
    }


    template <typename T>
    OpStatus setValues(psize_t pos, const core::StaticVector<T, Columns>& values)
    {
        if (isFail(removeSpace(pos, pos + 1))) {
            return OpStatus::FAIL;
        }

        return insert(pos, values);
    }

    template <typename T>
    OpStatus insert(psize_t pos, const core::StaticVector<T, Columns>& values)
    {
        if (isFail(insert_from_fn(pos, 1, [&](psize_t row){
                return values[0];
        }))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }

    ViewType max(psize_t column) const
    {
        auto size = this->size();

        if (size > 0)
        {
            return access(size - 1);
        }
        else {
            MMA_THROW(RuntimeException()) << format_ex("Column {} is empty", column);
        }
    }

    template <typename T>
    void max(core::StaticVector<T, Columns>& accum) const
    {
        psize_t size = data_->size();

        if (size > 0)
        {
            accum[0] = access(size - 1);
        }
        else {
            accum[0] = T{};
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void max(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
        accum[0] = this->max(0);
    }


    void configure_io_substream(io::IOSubstream& substream) const
    {
        auto& view = io::substream_cast<typename PkdStruct::IOSubstreamView>(substream);
        view.configure(*this);
    }



    OpStatusT<int32_t> insert_io_substream(psize_t at, const io::IOSubstream& substream, psize_t start, psize_t size)
    {
        using IOBuffer = typename PkdStruct::GrowableIOSubstream;

        const IOBuffer& buffer = io::substream_cast<IOBuffer>(substream);

        DataLengths lengths = to_data_lengths(buffer.data_lengths(start, size));

        if(isFail(insertSpace(at, size, lengths))) {
            return OpStatus::FAIL;
        }

        for_each_dimension([&](auto dim_idx) {
            data_->template dimension<dim_idx>().copy_from_databuffer(
                    at, start, size, lengths[dim_idx], buffer
            );
        });

        return OpStatusT<int32_t>(at + size);
    }



    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    OpStatus _update_b(psize_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&& val)
    {
        if (isFail(removeSpace(pos, pos + 1))) {
            return OpStatus::FAIL;
        }

        return _insert_b<Offset>(pos, accum, std::forward<AccessorFn>(val));
    }

    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    OpStatus _insert_b(psize_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&& val)
    {
        if (isFail(insert_from_fn(pos, 1, [&](psize_t row){
                return val(0);
        }))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _remove(psize_t idx, BranchNodeEntryItem<T, Size>& accum)
    {
        return removeSpace(idx, idx + 1);
    }





    class FindResult {
        int32_t idx_;
    public:
        FindResult(int32_t idx): idx_(idx)
        {}

        FindResult(): idx_()
        {}

        ViewType prefix() {return ViewType{};}
        int32_t local_pos() const {return idx_;}
    };

    FindResult findGTForward(psize_t column, const ViewType& val) const
    {
        auto end = this->end();
        auto ii = std::upper_bound(begin(), end, val);

        if (ii != end)
        {
            return ii.pos();
        }

        return FindResult(data_->size());
    }

    FindResult findGEForward(psize_t column, const ViewType& val) const
    {
        auto end = this->end();
        auto ii = std::lower_bound(begin(), end, val);

        if (ii != end)
        {
            return ii.pos();
        }

        return FindResult(data_->size());
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

    psize_t estimate_insert_upsize(const ViewType& view) const
    {
        auto& meta = data_->metadata();

        psize_t total_size{};

        auto data = DataTypeTraits<DataType>::describe_data(&view);

        for_each_dimension([&](auto idx){
            total_size += data_->template dimension<idx>().estimate_insert_upsize(
                std::get<idx>(data), meta
            );
        });

        return total_size;
    }

    psize_t estimate_replace_upsize(psize_t idx, const ViewType& view) const
    {
        auto& meta = data_->metadata();

        psize_t total_size{};

        auto data = DataTypeTraits<DataType>::describe_data(&view);

        for_each_dimension([&](auto dim_idx){
            total_size += data_->template dimension<dim_idx>().estimate_replace_upsize(
                idx, std::get<dim_idx>(data), meta
            );
        });

        return total_size;
    }

    OpStatus replace(psize_t column, psize_t idx, const ViewType& view)
    {
        DataDimensionsTuple data = DataTypeTraits<DataType>::describe_data(&view);

        if (isFail(resize(idx, data))) {
            return OpStatus::FAIL;
        }

        for_each_dimension([&](auto dim_idx) {
            data_->template dimension<dim_idx>().replace_row(
                idx, std::get<dim_idx>(data)
            );
        });

        return OpStatus::OK;
    }

    OpStatus insert(int32_t idx, int32_t size, std::function<Values (int32_t)> provider)
    {
        return insert_from_fn(idx, size, [&](psize_t row){
            return provider(row)[0];
        });
    }



    template <typename AccessorFn>
    OpStatus insert_from_fn(psize_t row_at, psize_t size, AccessorFn&& elements)
    {
        DataLengths data_lengths{};

        for (size_t row = 0; row < size; row++)
        {
            auto elem = elements(row);
            auto data = DataTypeTraits<DataType>::describe_data(&elem);

            for_each_dimension([&](auto dim_idx){
                data_lengths[dim_idx] += data_length(std::get<dim_idx>(data));
            });
        }

        if (isFail(insertSpace(row_at, size, data_lengths))) {
            return OpStatus::FAIL;
        }

        for (psize_t row = 0; row < size; row++)
        {
            auto elem = elements(row);
            auto data = DataTypeTraits<DataType>::describe_data(&elem);

            for_each_dimension([&](auto dim_idx){
                data_->template dimension<dim_idx>().copy_from(
                    row_at + row, std::get<dim_idx>(data)
                );
            });
        }

        return OpStatus::OK;
    }



    OpStatus insert(psize_t idx, const ViewType& view)
    {
        static_assert(Columns == 1, "");

        core::StaticVector<ViewType, 1> vv;
        vv[0] = view;

        return insert(idx, vv);
    }

    OpStatus remove(psize_t idx)
    {
        return removeSpace(idx, idx + 1);
    }

private:

    template <typename T>
    static size_t data_length(const Span<const T>& span) {
        return span.length();
    }

    template <typename T>
    static psize_t data_length(const T*) {
        return 1;
    }


    void copyTo(MyType& other, psize_t copy_from, psize_t count, psize_t copy_to, const DataLengths& data_lengths) const
    {
        for_each_dimension([&](auto idx){
            auto dimension = data_->template dimension<idx>();
            dimension.copy_to(other.data(), copy_from, count, copy_to, data_lengths[idx]);
        });
    }


    OpStatus insertSpace(psize_t idx, psize_t room_length, const DataLengths& data_lengths)
    {
        auto& meta = data_->metadata();

        psize_t size = meta.size();

        MEMORIA_V1_ASSERT(idx, <=, size);
        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(room_length, >=, 0);

        psize_t extra_data{};

        for_each_dimension([&](auto dim_idx) {
            extra_data += data_->template dimension<dim_idx>().compute_new_size(
                    room_length, data_lengths[dim_idx]
            );
        });

        if(isFail(data_->resize(PkdStruct::base_size(extra_data)))) {
            return OpStatus::FAIL;
        }

        OpStatus status{OpStatus::OK};

        for_each_dimension([&](auto dim_idx) {
            if (!isFail(status))
            {
                status = data_->template dimension<dim_idx>().insert_space(
                    idx, room_length, data_lengths[dim_idx]
                );
            }
        });

        meta.size() += room_length;

        return status;
    }


    OpStatus resize(psize_t idx, const DataDimensionsTuple& new_value_sizes)
    {
        auto& meta = data_->metadata();

        psize_t size = meta.size();

        MEMORIA_V1_ASSERT(idx, <=, size);
        MEMORIA_V1_ASSERT(idx, >=, 0);

        OpStatus status{OpStatus::OK};

        for_each_dimension([&](auto dim_idx) {
            if (!isFail(status))
            {
                status = data_->template dimension<dim_idx>().resize_row(
                    idx, std::get<dim_idx>(new_value_sizes)
                );
            }
        });

        return status;
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
};




}
