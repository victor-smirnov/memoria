
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

#include <memoria/v1/core/config.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/iobuffer/io_buffer.hpp>
#include <memoria/v1/core/tools/uuid.hpp>

#include <memoria/v1/core/container/allocator.hpp>

#include "../common/ctr_input_btss.hpp"
#include "../common/ctr_output_btss.hpp"

#include "ctr_api.hpp"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif


namespace memoria {
namespace v1 {

template <typename CtrName, typename Profile> 
class CtrApiBTFLBase: public CtrApiBase<CtrName, Profile> {
    using Base = CtrApiBase<CtrName, Profile>;
protected:    
    using typename Base::AllocatorT;
    using typename Base::CtrPtr;

    using Iterator = IterApi<CtrName, Profile>;
    
public:
    CtrApiBTFLBase(const std::shared_ptr<AllocatorT>& allocator, int command, const UUID& name):
        Base(allocator, command, name) 
    {}
        
    CtrApiBTFLBase(CtrPtr ptr): Base(ptr) {}
    
    CtrApiBTFLBase(const CtrApiBTFLBase& other): Base(other) {}
    CtrApiBTFLBase(CtrApiBTFLBase&& other): Base(std::move(other)) {}
    
    void operator=(const CtrApiBTFLBase& other) {Base::operator=(other);}
    void operator=(CtrApiBTFLBase&& other) {Base::operator=(std::move(other));}
    
    Iterator seq_begin();
    Iterator seq_end();
    int64_t  seq_size() const;
    Iterator seq_seek(int64_t pos);
    
    Iterator seq_select(int64_t rank, int32_t sym);
};


template <typename CtrName, typename Profile> 
class IterApiBTFLBase: public IterApiBase<CtrName, Profile> {
    using Base = IterApiBase<CtrName, Profile>;
protected:
    
    using typename Base::AllocatorT;
    using typename Base::IterT;
    using typename Base::IterPtr;
    using typename Base::CtrT;
    using typename Base::CtrPtr;
    
public:
    using Iterator = IterApi<CtrName, Profile>;
    
    IterApiBTFLBase(IterPtr ptr): Base(ptr) {}
    ~IterApiBTFLBase() {}
    
    IterApiBTFLBase(const IterApiBTFLBase& other): Base(other) {}
    IterApiBTFLBase(IterApiBTFLBase&& other): Base(std::move(other)) {}
    
    void operator=(const IterApiBTFLBase& other) {Base::operator=(other);}
    void operator=(IterApiBTFLBase&& other)      {Base::operator=(std::move(other));}
    
    int32_t symbol() const;
    
    bool seq_is_end() const;
    
    bool next_sym();
    bool prev_sym();
    
    int64_t remove_entries(int64_t length);
    
    int64_t seq_pos();
    int64_t skip_seq(int64_t offset);
    
    int64_t read_seq(CtrIOBuffer& buffer, int64_t size = std::numeric_limits<int64_t>::max());
    int64_t read_seq(bt::BufferConsumer<CtrIOBuffer>& consumer, int64_t size = std::numeric_limits<int64_t>::max());
    int64_t insert_subseq(bt::BufferProducer<CtrIOBuffer>& producer);
    
};


}
}
