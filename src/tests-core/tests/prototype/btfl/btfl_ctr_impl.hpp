
// Copyright 2017 Victor Smirnov
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

#include "btfl_ctr_api.hpp"

#include "container/btfl_test_factory.hpp"
#include <memoria/v1/core/container/ctr_impl_btfl.hpp>

#include <memory>

namespace memoria {
namespace v1 {

template <int32_t DataStreams, PackedDataTypeSize LeafSizeType, typename Profile>
typename CtrApi<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>::Iterator CtrApi<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>::begin()
{
    return this->pimpl_->seq_begin();
}

template <int32_t DataStreams, PackedDataTypeSize LeafSizeType, typename Profile>
core::StaticVector<int64_t, DataStreams> IterApi<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>::insert_iodata(const BTFLTestIOData<DataStreams, DataStreams - 1>& iodata)
{
    return this->pimpl_->insert_iodata(iodata);
}


template <int32_t DataStreams, PackedDataTypeSize LeafSizeType, typename Profile>
typename CtrApi<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>::Iterator CtrApi<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>::seekL0(int64_t pos)
{
    return this->pimpl_->seq_seekL0(pos);
}


template <int32_t DataStreams, PackedDataTypeSize LeafSizeType, typename Profile>
BTFLTestIOData<DataStreams, 0> IterApi<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>::read_data0(int64_t length)
{
    return this->pimpl_->template readData<0>(length);
}


template <int32_t DataStreams, PackedDataTypeSize LeafSizeType, typename Profile>
BTFLTestIOData<DataStreams, 0> IterApi<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>::read_entries0(int64_t length)
{
    return this->pimpl_->template readEntries<0>(length);
}


template <int32_t DataStreams, PackedDataTypeSize LeafSizeType, typename Profile>
BTFLTestIOData<DataStreams, 0> IterApi<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>::read_entries0()
{
    return this->pimpl_->template readEntries<0>();
}

template <int32_t DataStreams, PackedDataTypeSize LeafSizeType, typename Profile>
int64_t IterApi<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>::rank(int32_t symbol)
{
    return this->pimpl_->rank(symbol);
}




template <int32_t DataStreams, PackedDataTypeSize LeafSizeType, typename Profile>
int64_t IterApi<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>::count_children()
{
    return this->pimpl_->countChildren();
}

template <int32_t DataStreams, PackedDataTypeSize LeafSizeType, typename Profile>
void IterApi<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>::to_child(int64_t child_idx)
{
    return this->pimpl_->toChild(child_idx);
}


template <int32_t DataStreams, PackedDataTypeSize LeafSizeType, typename Profile>
void IterApi<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>::to_parent(int64_t parent_idx)
{
    return this->pimpl_->toParent(parent_idx);
}

template <int32_t DataStreams, PackedDataTypeSize LeafSizeType, typename Profile>
int64_t IterApi<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>::select_ge_fw(int64_t rank, int32_t level)
{
    return this->pimpl_->selectGEFw(rank, level);
}

template <int32_t DataStreams, PackedDataTypeSize LeafSizeType, typename Profile>
bool IterApi<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>::next()
{
    return this->pimpl_->next();
}


template <int32_t DataStreams, PackedDataTypeSize LeafSizeType, typename Profile>
core::StaticVector<int64_t, DataStreams + 1> IterApi<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>::remove_ge(int64_t size)
{
    return this->pimpl_->removeGE(size);
}

}}

