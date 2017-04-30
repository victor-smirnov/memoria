
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

#include "vector_input.hpp"

#include <memoria/v1/core/container/ctr_api_btss.hpp>
#include <memoria/v1/core/types/types.hpp>

#include <memory>
#include <vector>

namespace memoria {
namespace v1 {


    
template <typename Value, typename Profile> 
class CtrApi<Vector<Value>, Profile>: public CtrApiBTSSBase<Vector<Value>, Profile>  {
    using Base = CtrApiBTSSBase<Vector<Value>, Profile>;
    
    using typename Base::AllocatorT;
    using typename Base::CtrT;
    using typename Base::CtrPtr;

    using typename Base::Iterator;
    
public:
    MMA1_DECLARE_CTRAPI_BTSS_BASIC_METHODS()
    
    Iterator seek(BigInt pos);
};


template <typename Value, typename Profile> 
class IterApi<Vector<Value>, Profile>: public IterApiBTSSBase<Vector<Value>, Profile> {
    
    using Base = IterApiBTSSBase<Vector<Value>, Profile>;
    
    using typename Base::IterT;
    using typename Base::IterPtr;
     
public:
    
    using Base::read;
    using Base::insert;
    
    MMA1_DECLARE_ITERAPI_BTSS_BASIC_METHODS()
    
    Value value();
    
    void insert(const std::vector<Value>& data) 
    {
        using StdIter = typename std::vector<Value>::const_iterator;
        InputIteratorProvider<Value, StdIter, StdIter, CtrIOBuffer> provider(data.begin(), data.end());
        insert(provider);
    }
    
    std::vector<Value> read(size_t size);
};
    
}}
