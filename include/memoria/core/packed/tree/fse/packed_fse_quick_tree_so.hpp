
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

#include <memoria/profiles/common/block_operations.hpp>
#include <memoria/core/tools/static_array.hpp>

namespace memoria {

template <typename ExtData, typename PkdStruct>
class PackedFSEQuickTreeSO {
    ExtData* ext_data_;
    PkdStruct* data_;


    using MyType = PackedFSEQuickTreeSO;


public:
    using Values = typename PkdStruct::Values;
    using Value = typename PkdStruct::Value;
    using IndexValue = typename PkdStruct::IndexValue;

    using PkdStructT = PkdStruct;

    static constexpr size_t Blocks = PkdStruct::Blocks;

    PackedFSEQuickTreeSO() : ext_data_(), data_() {}
    PackedFSEQuickTreeSO(ExtData* ext_data, PkdStruct* data) :
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


    operator bool() const  {
        return data_ != nullptr;
    }

    const ExtData* ext_data() const  {return ext_data_;}
    ExtData* ext_data()  {return ext_data_;}

    const PkdStruct* data() const  {return data_;}
    PkdStruct* data()  {return data_;}

    VoidResult splitTo(MyType& other, size_t idx)
    {
        return data_->splitTo(other.data(), idx);
    }

    VoidResult mergeWith(MyType& other) const  {
        return data_->mergeWith(other.data());
    }

    VoidResult removeSpace(size_t room_start, size_t room_end)  {
        return data_->removeSpace(room_start, room_end);
    }


    VoidResult reindex()  {
        return data_->reindex();
    }


    const Value& access(size_t column, size_t row) const  {
        return data_->value(column, row);
    }

    Values access(size_t row) const  {
        return data_->access(row);
    }

    template <typename T>
    VoidResult setValues(size_t idx, const core::StaticVector<T, Blocks>& values)  {
        return data_->setValues(idx, values);
    }

    size_t size() const {
        return data_->size();
    }

    auto findForward(SearchType search_type, size_t block, size_t start, IndexValue val) const {
        return data_->findForward(search_type, block, start, val);
    }

    auto findGTForward(size_t block, size_t start, IndexValue val) const {
        return data_->findGTForward(block, start, val);
    }

    auto find_gt(size_t block, IndexValue val) const {
        return data_->find_ge(block, val);
    }

    auto find_gt(size_t block, size_t start, IndexValue val) const {
        return data_->find_ge(block, start, val);
    }

    auto findGTForward(size_t block, IndexValue val) const {
        return data_->findGTForward(block, val);
    }

    auto findGEForward(size_t block, size_t start, IndexValue val) const {
        return data_->findGEForward(block, start, val);
    }

    auto findGEForward(size_t block, IndexValue val) const {
        return data_->findGEForward(block, val);
    }

    auto find_ge(size_t block, IndexValue val) const {
        return data_->find_ge(block, val);
    }

    auto find_ge(size_t block, size_t start, IndexValue val) const {
        return data_->find_ge(block, start, val);
    }

    auto findBackward(SearchType search_type, size_t block, size_t start, IndexValue val) const {
        return data_->findBackward(search_type, block, start, val);
    }

    const Value& value(size_t block, size_t idx) const {
        return data_->value(block, idx);
    }

    template <typename T>
    void _add(size_t block, size_t start, size_t end, T& value) const
    {
        value += data_->sum(block, start, end);
    }



    template <typename T>
    void _sub(size_t block, size_t start, size_t end, T& value) const
    {
        value -= data_->sum(block, start, end);
    }

    template <typename... Args>
    auto sum(Args&&... args) const {
        return data_->sum(std::forward<Args>(args)...);
    }

    template <typename... Args>
    auto findNZLT(Args&&... args) const {
        return data_->findNZLT(std::forward<Args>(args)...);
    }


    void generateDataEvents(IBlockDataEventHandler* handler) const {
        return data_->generateDataEvents(handler);
    }

    void check() const {
        return data_->check();
    }

    template <typename AccessorFn>
    VoidResult insert_entries(psize_t row_at, psize_t size, AccessorFn&& elements)
    {
        return data_->insert_entries(row_at, size, std::forward<AccessorFn>(elements));
    }

    template <typename AccessorFn>
    VoidResult update_entries(psize_t row_at, psize_t size, AccessorFn&& elements)
    {
        MEMORIA_TRY_VOID(data_->removeSpace(row_at, row_at + size));
        return insert_entries(row_at, size, std::forward<AccessorFn>(elements));
    }

    template <typename AccessorFn>
    VoidResult remove_entries(psize_t row_at, psize_t size)
    {
        MEMORIA_TRY_VOID(data_->removeSpace(row_at, row_at + size));
        return data_->reindex();
    }

    auto iterator(size_t idx) const {
        return data_->iterator(idx);
    }


    auto iterator(size_t block, size_t idx) const {
        return data_->iterator(block, idx);
    }

    auto sum_for_rank(size_t start, size_t end, size_t symbol, SeqOpType seq_op) const {
        return data_->sum_for_rank(start, end, symbol, seq_op);
    }

    auto find_for_select_fw(size_t start, Value rank, size_t symbol, SeqOpType seq_op) const {
        return data_->find_for_select_fw(start, rank, symbol, seq_op);
    }

    template <typename T>
    VoidResult append(const core::StaticVector<T, Blocks>& values)
    {
        return data_->append(values);
    }

};




}
