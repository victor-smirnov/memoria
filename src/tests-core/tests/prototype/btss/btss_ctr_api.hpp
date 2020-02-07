
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

#include <memoria/api/common/ctr_api_btss.hpp>


#include <memory>


namespace memoria {

template <PackedDataTypeSize LeafSizeType, PackedDataTypeSize BranchSizeType>
class BTSSTestCtr {};
    
template <PackedDataTypeSize LeafSizeType, PackedDataTypeSize BranchSizeType, typename Profile> 
class CtrApi<BTSSTestCtr<LeafSizeType, BranchSizeType>, Profile>: public CtrApiBTSSBase<BTSSTestCtr<LeafSizeType, BranchSizeType>, Profile> {

    using Base = CtrApiBTSSBase<BTSSTestCtr<LeafSizeType, BranchSizeType>, Profile>;
    
    using typename Base::AllocatorT;
    using typename Base::CtrT;
    using typename Base::CtrPtr;

    using typename Base::Iterator;
    
public:
    using DataValue = int64_t;

    MMA_DECLARE_CTRAPI_BASIC_METHODS()
};


template <PackedDataTypeSize LeafSizeType, PackedDataTypeSize BranchSizeType, typename Profile> 
class IterApi<BTSSTestCtr<LeafSizeType, BranchSizeType>, Profile>: public IterApiBTSSBase<BTSSTestCtr<LeafSizeType, BranchSizeType>, Profile> {
    
    using Base = IterApiBTSSBase<BTSSTestCtr<LeafSizeType, BranchSizeType>, Profile>;
    
    using typename Base::IterT;
    using typename Base::IterPtr;
     
public:
    using DataValue = int64_t;
    using Value = int64_t;
    
    using Base::read;
    using Base::insert;
    
    MMA_DECLARE_ITERAPI_BASIC_METHODS();
    
    template <typename T>
    auto insert(const std::vector<T>& data) 
    {
        using StdIter = typename std::vector<T>::const_iterator;
        InputIteratorProvider<DataValue, StdIter, StdIter, CtrIOBuffer, false> provider(data.begin(), data.end());
        return insert(provider);
    }
  
    std::vector<DataValue> read(size_t size);
};
    
}
