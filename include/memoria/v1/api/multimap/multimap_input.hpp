
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

#include <memoria/v1/core/tools/iobuffer/io_buffer.hpp>

#include <memoria/v1/core/exceptions/memoria.hpp>

#include <memory>
#include <tuple>
#include <exception>

namespace memoria {
namespace v1 {
    
template <typename IOBuffer, int32_t Symbols>
class SingleStreamProducerAdapter: public bt::BufferProducer<IOBuffer> {
    
    bt::BufferProducer<IOBuffer>& delegate_;
    int32_t symbol_;
    
public:
    SingleStreamProducerAdapter(bt::BufferProducer<IOBuffer>& delegate, int32_t symbol):
        delegate_(delegate),
        symbol_(symbol)
    {}
    
    virtual int32_t populate(IOBuffer& buffer) 
    {
        int32_t entries{};
        auto run_pos = buffer.pos();
        
        auto res = buffer.template putSymbolsRun<Symbols>(
            symbol_,
            IOBuffer::template getMaxSymbolsRunLength<Symbols>()
        );
        
        if (res) 
        {
            entries++;
            
            auto data_entries = delegate_.populate(buffer);
            if (data_entries > 0)
            {
                buffer.template updateSymbolsRun<Symbols>(run_pos, symbol_, data_entries);
                return entries + data_entries;
            }
            else if (data_entries < 0) 
            {
                buffer.template updateSymbolsRun<Symbols>(run_pos, symbol_, -data_entries);
                return -(entries - data_entries);
            }
            else {
                buffer.pos(run_pos);
                return -entries;
            }
        }
        else {
            throw Exception(MMA1_SRC, SBuf() << "Not enough capacity in IOBuffer: " << buffer.capacity());
        }
    }
};


template <typename Key, typename IOBuffer, int32_t Symbols>
class SingleEntryProducerAdapter: public bt::BufferProducer<IOBuffer> {
    
    const Key& key_;
    SingleStreamProducerAdapter<IOBuffer, Symbols> data_buffer_;
    
    int32_t key_sym_;
    
    bool key_processed_{};
    
public:
    SingleEntryProducerAdapter(const Key& key, bt::BufferProducer<IOBuffer>& values_buffer, int32_t key_sym):
        key_(key), data_buffer_(values_buffer, key_sym + 1), key_sym_(key_sym)
    {}
    
    virtual int32_t populate(IOBuffer& buffer)
    {
        int32_t entries{};
        
        if (MMA1_UNLIKELY(!key_processed_))
        {
            if (buffer.template putSymbolsRun<Symbols>(key_sym_, 1)) 
            {
                if (IOBufferAdapter<Key>::put(buffer, key_)) 
                {
                    entries += 2;
                }
                else {
                    throw Exception(MMA1_SRC, SBuf() << "Not enough capacity in IOBuffer: " << buffer.capacity());
                }
            }
            else {
                throw Exception(MMA1_SRC, SBuf() << "Not enough capacity in IOBuffer: " << buffer.capacity());
            }
        }
        
        auto ee = data_buffer_.populate(buffer);
        
        if (ee > 0) {
            return entries + ee;
        }
        else {
            return -entries + ee;
        }
    }
};

    
}}
