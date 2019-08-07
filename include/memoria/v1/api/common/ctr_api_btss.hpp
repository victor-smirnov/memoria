
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/container/container.hpp>


#include <memoria/v1/core/tools/uuid.hpp>

#include <memoria/v1/core/container/allocator.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

#include "ctr_api.hpp"

#include <limits>


namespace memoria {
namespace v1 {

template <typename Profile>
struct BTSSIterator {
    virtual ~BTSSIterator() noexcept {}

    virtual const io::IOVector& iovector_view() const = 0;
    virtual int32_t iovector_pos() const    = 0;

    virtual bool is_end() const         = 0;
    virtual bool next_leaf()            = 0;
    virtual bool next_entry()           = 0;
};



template <typename CtrName, typename Profile> 
class CtrApiBTSSBase: public CtrApiBase<CtrName, Profile> {
    using Base = CtrApiBase<CtrName, Profile>;
protected:    
    using typename Base::AllocatorT;
    using typename Base::CtrPtr;

    using Iterator = IterApi<CtrName, Profile>;

public:
    using CtrID = typename Base::CtrID;


    static constexpr int32_t DataStreams = 1;
    using CtrSizesT = ProfileCtrSizesT<Profile, DataStreams>;
    
    CtrApiBTSSBase(const AllocSharedPtr<AllocatorT>& allocator, int command, const CtrID& ctr_id):
        Base(allocator, command, ctr_id)
    {}
        
    CtrApiBTSSBase(CtrPtr ptr): Base(ptr) {}
    
    CtrApiBTSSBase(const CtrApiBTSSBase& other): Base(other) {}
    CtrApiBTSSBase(CtrApiBTSSBase&& other): Base(std::move(other)) {}
    
    void operator=(const CtrApiBTSSBase& other) {Base::operator=(other);}
    void operator=(CtrApiBTSSBase&& other) {Base::operator=(std::move(other));}
    
    Iterator begin();
    Iterator end();
    Iterator seek(int64_t pos);
    
    int64_t size();
};


template <typename CtrName, typename Profile> 
class IterApiBTSSBase: public IterApiBase<CtrName, Profile> {
    using Base = IterApiBase<CtrName, Profile>;
protected:    
    
    using typename Base::AllocatorT;
    using typename Base::IterT;
    using typename Base::IterPtr;
    using typename Base::CtrT;
    using typename Base::CtrPtr;
    
    using typename Base::CtrSizeT;
    
public:
    static constexpr int32_t DataStreams = CtrApiBTSSBase<CtrName, Profile>::DataStreams;
    using CtrSizesT = typename CtrApiBTSSBase<CtrName, Profile>::CtrSizesT;
    
    using Iterator  = IterApi<CtrName, Profile>;
    
    IterApiBTSSBase(IterPtr ptr): Base(ptr) {}
    ~IterApiBTSSBase() {}
    
    IterApiBTSSBase(const IterApiBTSSBase& other): Base(other) {}
    IterApiBTSSBase(IterApiBTSSBase&& other): Base(std::move(other)) {}
    
    void operator=(const IterApiBTSSBase& other) {Base::operator=(other);}
    void operator=(IterApiBTSSBase&& other)      {Base::operator=(std::move(other));}
    
    bool is_end() const;
    
    bool next();
    bool prev();
    
    void remove();
    int64_t remove(int64_t length);
    
    int64_t pos();
    int64_t skip(int64_t offset);

    int64_t read_to(io::IOVectorConsumer& producer, int64_t length = std::numeric_limits<CtrSizeT>::max());
    int64_t populate(io::IOVector& io_vector, int64_t length = std::numeric_limits<CtrSizeT>::max());

    int64_t insert(io::IOVectorProducer& producer, int64_t start = 0, int64_t length = std::numeric_limits<int64_t>::max());
    int64_t insert(io::IOVector& io_vector, int64_t start = 0, int64_t length = std::numeric_limits<int64_t>::max());
};


}
}
