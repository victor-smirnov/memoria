
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

#include <memoria/v1/api/common/ctr_api_btfl.hpp>

#include <memoria/v1/prototypes/bt_fl/iodata/btfl_iodata_decl.hpp>

#include <memory>

namespace memoria {
namespace v1 {

using BTFLTestKeyT       = int64_t;
using BTFLTestValueT     = int8_t;
using BTFLTestColumnT    = int64_t;

template <int32_t DataStreams, int32_t Level>
using BTFLTestIOData = btfl::BTFLData<DataStreams, Level, BTFLTestKeyT, BTFLTestValueT, BTFLTestColumnT>;


template <int32_t DataStreams, PackedSizeType SizeType = PackedSizeType::VARIABLE>
class BTFLTestCtr {};

template <int32_t DataStreams, PackedSizeType LeafSizeType, typename Profile>
class CtrApi<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>: public CtrApiBTFLBase<BTFLTestCtr<DataStreams, LeafSizeType>, Profile> {

    using Base = CtrApiBTFLBase<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>;
    
    using typename Base::AllocatorT;
    using typename Base::CtrT;
    using typename Base::CtrPtr;

    using typename Base::Iterator;
    
public:
    MMA1_DECLARE_CTRAPI_BASIC_METHODS()

    Iterator begin();
    Iterator seekL0(int64_t pos);
};


template <int32_t DataStreams, PackedSizeType LeafSizeType, typename Profile>
class IterApi<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>: public IterApiBTFLBase<BTFLTestCtr<DataStreams, LeafSizeType>, Profile> {
    
    using Base = IterApiBTFLBase<BTFLTestCtr<DataStreams, LeafSizeType>, Profile>;
    
    using typename Base::IterT;
    using typename Base::IterPtr;
     
public:
    
    MMA1_DECLARE_ITERAPI_BASIC_METHODS()


    core::StaticVector<int64_t, DataStreams> insert_iodata(const BTFLTestIOData<DataStreams, DataStreams - 1>& iodata);

    BTFLTestIOData<DataStreams, 0> read_data0(int64_t length);
    BTFLTestIOData<DataStreams, 0> read_entries0(int64_t number);
    BTFLTestIOData<DataStreams, 0> read_entries0();

    int64_t rank(int32_t symbol);

    int64_t count_children();
    void to_child(int64_t child_idx);
    void to_parent(int64_t parent_idx);
    int64_t select_ge_fw(int64_t rank, int32_t level);

    core::StaticVector<int64_t, DataStreams + 1> remove_ge(int64_t size);

    template <int32_t Stream>
    auto key() const {
        return this->pimpl_->template key<Stream>();
    }

    bool next();
};
    
}}
