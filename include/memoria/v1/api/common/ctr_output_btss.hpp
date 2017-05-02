
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/api/common/ctr_api.hpp>

#include <memoria/v1/core/tools/bignum/int64_codec.hpp>
#include <memoria/v1/core/types/types.hpp>

namespace memoria {
namespace v1 {

template <typename Value, typename OutputIterator, typename IOBuffer>
class OutputIteratorBTSSAdaptor: public bt::BufferConsumer<IOBuffer> {
    
    OutputIterator iter_;
    
public:
    OutputIteratorBTSSAdaptor(const OutputIterator& iter): iter_(iter) {}
    
    virtual int32_t process(IOBuffer& buffer, int32_t entries) {
        for (int32_t c = 0; c < entries; c++, iter_++) 
        {
            *iter_ = IOBufferAdapter<Value>::get(buffer);
        }
        
        return entries;
    }
};


template <typename Value, typename Fn, typename IOBuffer>
class BTSSAdaptorFn: public bt::BufferConsumer<IOBuffer> {
    
    Fn& fn_;
    
public:
    BTSSAdaptorFn(Fn& fn): fn_(fn) {}
    
    virtual int32_t process(IOBuffer& buffer, int32_t entries) {
        for (int32_t c = 0; c < entries; c++) 
        {
            fn_(IOBufferAdapter<Value>::get(buffer));
        }
        
        return entries;
    }
};


}}
