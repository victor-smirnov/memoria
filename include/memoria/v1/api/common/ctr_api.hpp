
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
#include <memoria/v1/core/tools/memory.hpp>
#include <memoria/v1/core/container/allocator.hpp>
#include <memoria/v1/core/container/ctr_referenceable.hpp>

namespace memoria {
namespace v1 {

using CtrIOBuffer = DefaultIOBuffer;    
    
template <typename CtrName, typename Profile = DefaultProfile<>> class CtrApi;
template <typename CtrName, typename Profile = DefaultProfile<>> class IterApi;


template <typename CtrName, typename Allocator, typename Profile> class SharedCtr;
template <typename CtrName, typename Profile> class SharedIter;



template <typename Profile = DefaultProfile<>>
class CtrRef {
    using CtrPtrT = CtrSharedPtr<CtrReferenceable>;
    CtrPtrT ptr_;
public:
    CtrRef() {}
    
    CtrRef(const CtrPtrT& ptr): ptr_(ptr) {}
    CtrRef(CtrReferenceable* raw_ptr): ptr_(raw_ptr) {}
    
    void swap(CtrRef& other) {
        ptr_.swap(other.ptr_);
    }
    
    const UUID& name() const {return ptr_->name();}
    
    template <typename CtrName, typename ProfileT>
    friend bool is_castable_to(const CtrRef<ProfileT>& ref);
    
    template <typename CtrName, typename ProfileT>
    friend CtrApi<CtrName, ProfileT> cast(const CtrRef<ProfileT>& ref);
    
    template <typename CtrName, typename ProfileT>
    friend class CtrApiBase;
};

template <typename CtrName, typename Profile>
bool is_castable_to(const CtrRef<Profile>& ref) {
    return ref.ptr_->is_castable_to(TypeHash<CtrName>::Value);
}

template <typename CtrName, typename Profile>
CtrApi<CtrName, Profile> cast(const CtrRef<Profile>& ref) {
    return CtrApi<CtrName, Profile>::cast_from(ref);
}


template <typename CtrName, typename Profile> 
class CtrApiBase {
public:    
    using AllocatorT = IAllocator<ProfilePageType<Profile>>;
    using CtrT       = SharedCtr<CtrName, AllocatorT, Profile>;
    using CtrPtr     = CtrSharedPtr<CtrT>;

protected:    
    CtrPtr pimpl_;
    
public:
    CtrApiBase(const CtrSharedPtr<AllocatorT>& allocator, int command, const UUID& name);
    CtrApiBase(CtrPtr ptr);
    CtrApiBase();
    ~CtrApiBase();
    
    CtrApiBase(const CtrApiBase&);
    CtrApiBase(CtrApiBase&&);
    
    void operator=(const CtrApiBase& other);
    void operator=(CtrApiBase&& other);
    
    bool operator==(const CtrApiBase& other) const;
    operator bool() const;
    
    UUID name();
    const ContainerMetadataPtr& metadata();
    static void init();
    void new_page_size(int size);
    
    void reset();
    
    CtrRef<Profile> to_ref();
    operator CtrRef<Profile>() {return to_ref();}
    
    static CtrApi<CtrName, Profile> cast_from(const CtrRef<Profile>& ref);
};


template <typename CtrName, typename Profile> 
class IterApiBase {
protected:    
    
    using AllocatorT = IAllocator<ProfilePageType<Profile>>;
    using IterT      = SharedIter<CtrName, Profile>;
    using IterPtr    = CtrSharedPtr<IterT>;
    using CtrT       = SharedCtr<CtrName, AllocatorT, Profile>;
    using CtrPtr     = CtrSharedPtr<CtrT>;
     
    IterPtr pimpl_;
    
public:
    using Iterator  = IterApi<CtrName, Profile>;
    
    IterApiBase(IterPtr ptr);
    IterApiBase();
    ~IterApiBase();
    
    IterApiBase(const IterApiBase&);
    IterApiBase(IterApiBase&&);
    
    bool operator==(const IterApiBase& other) const;
    bool operator!=(const IterApiBase& other) const;
    
    operator bool() const;
    
    void operator=(const IterApiBase& other);
    void operator=(IterApiBase&& other);
    
    CtrApi<CtrName, Profile> ctr();
    
    Iterator clone();
    void reset();
    
    void dump();
    void dump_path();
    void check(std::ostream& out, const char* source);
};


template <typename CtrName, typename Profile>
struct CtrMetadataInitializer {
    CtrMetadataInitializer() {
        CtrApi<CtrName, Profile>::init();
    }
};

#define MMA1_INSTANTIATE_CTR_BTSS(CtrName, Profile, ...)\
template class IterApiBase<CtrName, Profile>;           \
template class IterApiBTSSBase<CtrName, Profile>;       \
template class CtrApiBase<CtrName, Profile>;            \
template class CtrApiBTSSBase<CtrName, Profile>;        \
template class CtrApi<CtrName, Profile>;                \
template class IterApi<CtrName, Profile>;               \
                                                        \
namespace {                                             \
CtrMetadataInitializer<CtrName, Profile> init_##__VA_ARGS__;\
}






#define MMA1_DECLARE_CTRAPI_BASIC_METHODS()                                            \
    CtrApi(const CtrSharedPtr<AllocatorT>& allocator, int32_t command, const UUID& name):   \
        Base(allocator, command, name) {}                                                   \
    ~CtrApi() {}                                                                            \
                                                                                            \
    CtrApi(const CtrApi& other): Base(other) {}                                             \
    CtrApi(const CtrPtr& ptr): Base(ptr) {}                                                 \
    CtrApi(CtrApi&& other): Base(std::move(other)) {}                                       \
                                                                                            \
    CtrApi& operator=(const CtrApi& other) {Base::operator=(other); return *this;}          \
    CtrApi& operator=(CtrApi&& other){Base::operator=(std::move(other)); return *this;}
    
    
    
    


#define MMA1_DECLARE_ITERAPI_BASIC_METHODS()        \
    IterApi(IterPtr ptr):Base(ptr) {}               \
    ~IterApi() {}                                   \
                                                    \
    IterApi(const IterApi& other): Base(other) {}   \
    IterApi(IterApi&& other):                       \
        Base(std::move(other)) {}                   \
                                                    \
    IterApi& operator=(const IterApi& other) {      \
        Base::operator=(other); return *this;       \
    }                                               \
    IterApi& operator=(IterApi&& other) {           \
        Base::operator=(std::move(other));          \
        return *this;                               \
    }

}
}
