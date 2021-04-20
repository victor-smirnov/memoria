
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
#include <memoria/core/types.hpp>

#include <memory>
#include <vector>

namespace memoria {
    
template <int32_t BitsPerSymbol, bool Dense, typename Profile>
class CtrApi<Sequence<BitsPerSymbol, Dense>, Profile>: public CtrApiBTSSBase<Sequence<BitsPerSymbol, Dense>, Profile>  {
    using Base = CtrApiBTSSBase<Sequence<BitsPerSymbol, Dense>, Profile>;
public:

    using typename Base::CtrID;
    using typename Base::StoreT;
    using typename Base::CtrT;
    using typename Base::CtrPtr;

    using typename Base::Iterator;
    
    MMA_DECLARE_CTRAPI_BASIC_METHODS()
};


template <int32_t BitsPerSymbol, bool Dense, typename Profile>
class IterApi<Sequence<BitsPerSymbol, Dense>, Profile>: public IterApiBTSSBase<Sequence<BitsPerSymbol, Dense>, Profile> {
    
    using Base = IterApiBTSSBase<Sequence<BitsPerSymbol, Dense>, Profile>;
    
    using typename Base::IterT;
    using typename Base::IterPtr;
    
public:
    
    using DataValue = int;
    
    using Base::read;
    using Base::insert;
    
    MMA_DECLARE_ITERAPI_BASIC_METHODS()
    
};
    
}
