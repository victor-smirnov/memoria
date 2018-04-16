
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

#include "btss_test_factory.hpp"

#include <memoria/v1/api/vector/vector_api.hpp>
#include <memoria/v1/core/container/ctr_impl_btss.hpp>

#include <memory>

namespace memoria {
namespace v1 {


template <PackedSizeType LeafSizeType, PackedSizeType BranchSizeType, typename Profile> 
std::vector<typename IterApi<BTSSTestCtr<LeafSizeType, BranchSizeType>, Profile>::DataValue> IterApi<BTSSTestCtr<LeafSizeType, BranchSizeType>, Profile>::read(size_t size)
{
    std::vector<DataValue> data;
    
    auto fn = [&](const DataValue& vv){
        data.emplace_back(vv);
    };
    
    BTSSAdaptorFn<DataValue, std::remove_reference_t<decltype(fn)>, CtrIOBuffer> consumer(fn);
    
    this->read(consumer);
    
    return data;
}


}}

