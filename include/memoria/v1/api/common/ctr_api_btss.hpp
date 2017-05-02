
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/tools/iobuffer/io_buffer.hpp>
#include <memoria/v1/core/tools/uuid.hpp>

#include <memoria/v1/core/container/allocator.hpp>

#include "../common/ctr_input_btss.hpp"
#include "../common/ctr_output_btss.hpp"

#include "ctr_api.hpp"


namespace memoria {
namespace v1 {

template <typename CtrName, typename Profile> 
class CtrApiBTSSBase {
protected:    
    using AllocatorT = IWalkableAllocator<ProfilePageType<Profile>>;
    using CtrT       = SharedCtr<CtrName, AllocatorT, Profile>;
    using CtrPtr     = std::shared_ptr<CtrT>;

    using Iterator   = IterApi<CtrName, Profile>;
    
    CtrPtr pimpl_;
    
public:
    CtrApiBTSSBase(const std::shared_ptr<AllocatorT>& allocator, int command, const UUID& name);
    CtrApiBTSSBase(CtrPtr ptr);
    ~CtrApiBTSSBase();
    
    CtrApiBTSSBase(const CtrApiBTSSBase&);
    CtrApiBTSSBase(CtrApiBTSSBase&&);
    
    void operator=(const CtrApiBTSSBase& other);
    void operator=(CtrApiBTSSBase&& other);
    
    bool operator==(const CtrApiBTSSBase& other) const;
    operator bool() const;
    
    Iterator begin();
    Iterator end();
    Iterator seek(int64_t pos);
    
    int64_t size();
    
    UUID name();
    const ContainerMetadataPtr& metadata();
    static void init();
    void new_page_size(int size);
};


template <typename CtrName, typename Profile> 
class IterApiBTSSBase {
protected:    
    
    using AllocatorT = IWalkableAllocator<ProfilePageType<Profile>>;
    using IterT     = SharedIter<CtrName, Profile>;
    using IterPtr   = std::shared_ptr<IterT>;
    using CtrT       = SharedCtr<CtrName, AllocatorT, Profile>;
    using CtrPtr     = std::shared_ptr<CtrT>;
     
    IterPtr pimpl_;
    
public:
    using Iterator  = IterApi<CtrName, Profile>;
    
    IterApiBTSSBase(IterPtr ptr);
    ~IterApiBTSSBase();
    
    IterApiBTSSBase(const IterApiBTSSBase&);
    IterApiBTSSBase(IterApiBTSSBase&&);
    
    bool operator==(const IterApiBTSSBase& other) const;
    bool operator!=(const IterApiBTSSBase& other) const;
    
    operator bool() const;
    
    void operator=(const IterApiBTSSBase& other);
    void operator=(IterApiBTSSBase&& other);
    
    CtrApi<CtrName, Profile> ctr();
    
    bool is_end() const;
    
    bool next();
    bool prev();
    
    void remove();
    int64_t remove(int64_t length);
    
    void dump();
    void dump_path();
    void check(std::ostream& out, const char* source);
    
    int64_t pos();
    int64_t skip(int64_t offset);
    
    Iterator clone();
    
    
    int64_t read(CtrIOBuffer& buffer, int64_t size = 10000000);
    int64_t read(bt::BufferConsumer<CtrIOBuffer>& consumer, int64_t size = 10000000);
    
    int64_t read(std::function<int32_t (CtrIOBuffer&, int32_t)> consumer, int64_t size = 10000000);
    
    
    int64_t insert(bt::BufferProducer<CtrIOBuffer>& producer);
    int64_t insert(std::function<int32_t (CtrIOBuffer&)> producer);
};


}
}
