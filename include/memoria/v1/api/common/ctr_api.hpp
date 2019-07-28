
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

#include <memoria/v1/core/tools/memory.hpp>
#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/container/allocator.hpp>
#include <memoria/v1/core/container/ctr_referenceable.hpp>

#include <memoria/v1/core/tools/pair.hpp>

#include <memoria/v1/profiles/common/common.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

namespace memoria {
namespace v1 {


template <typename CtrName, typename Profile = DefaultProfile<>>
struct ICtrApi: CtrReferenceable
{

};




template <typename CtrName, typename Profile = DefaultProfile<>> class CtrApi;
template <typename CtrName, typename Profile = DefaultProfile<>> class IterApi;

template <typename CtrName, typename Allocator, typename Profile> class SharedCtr;
template <typename CtrName, typename Profile> class SharedIter;

namespace _ {

template <typename Profile, int32_t Streams> struct ProfileCtrSizesTS: HasType<
    core::StaticVector<ProfileCtrSizeT<Profile>, Streams>
> {};

}

template <typename Profile, int32_t Streams> 
using ProfileCtrSizesT = typename _::ProfileCtrSizesTS<Profile, Streams>::Type;



template <typename Profile = DefaultProfile<>>
class CtrRef {
    using CtrPtrT = CtrSharedPtr<CtrReferenceable>;
    CtrPtrT ptr_;

    using CtrID = ProfileCtrID<Profile>;

public:
    CtrRef() {}
    
    CtrRef(const CtrPtrT& ptr): ptr_(ptr) {}
    CtrRef(CtrReferenceable* raw_ptr): ptr_(raw_ptr) {}
    
    void swap(CtrRef& other) {
        ptr_.swap(other.ptr_);
    }
    
    const CtrID& name() const {return ptr_->name();}
    
    template <typename CtrName, typename ProfileT>
    friend bool is_castable_to(const CtrRef<ProfileT>& ref);
    
    template <typename CtrName, typename ProfileT>
    friend CtrApi<CtrName, ProfileT> cast(const CtrRef<ProfileT>& ref);
    
    template <typename CtrName, typename ProfileT>
    friend class CtrApiBase;
};

template <typename CtrName, typename Profile>
bool is_castable_to(const CtrRef<Profile>& ref) 
{
    return ref.ptr_->is_castable_to(TypeHashV<CtrName>);
}

template <typename CtrName, typename Profile>
CtrApi<CtrName, Profile> cast(const CtrRef<Profile>& ref) 
{
    return CtrApi<CtrName, Profile>::cast_from(ref);
}


template <typename CtrName, typename Profile> 
class CtrApiBase {
public:    
    using AllocatorT = ProfileAllocatorType<Profile>;

    using CtrT       = SharedCtr<CtrName, AllocatorT, Profile>;
    using CtrPtr     = CtrSharedPtr<CtrT>;
    
    using CtrSizeT   = ProfileCtrSizeT<Profile>;
    
    using CtrID      = ProfileCtrID<Profile>;

protected:    
    CtrPtr pimpl_;
    
public:
    CtrApiBase(const CtrSharedPtr<AllocatorT>& allocator, int command, const CtrID& ctr_id);
    CtrApiBase(CtrPtr ptr);
    CtrApiBase();
    ~CtrApiBase();
    
    CtrApiBase(const CtrApiBase&);
    CtrApiBase(CtrApiBase&&);
    
    void operator=(const CtrApiBase& other);
    void operator=(CtrApiBase&& other);
    
    bool operator==(const CtrApiBase& other) const;
    operator bool() const;

    CtrPtr& ptr();
    const CtrPtr& ptr() const;
    
    CtrID name();
    static void init_profile_metadata();

    static ContainerOperationsPtr<Profile> continer_interface();

    static void do_link();

    void new_block_size(int size);
    int new_block_size();

    bool is_updatable() const;
    
    void reset();

    void drop();

    CtrID clone_ctr(const CtrID& new_id);
    CtrID clone_ctr();

    void cleanup();

    static std::string ctr_type_name() {
        return TypeNameFactory<CtrName>::name().to_u8().to_std_string();
    }

    int32_t metadata_links_num() const;

    CtrID get_metadata_link(int num) const;

    void set_metadata_link(int num, const CtrID& link_id);

    std::string get_descriptor_str() const;

    void set_descriptor_str(const std::string& str);

    const PairPtr& pair() const;
    PairPtr& pair();

    
    CtrRef<Profile> to_ref();
    operator CtrRef<Profile>() {return to_ref();}
    
    static CtrApi<CtrName, Profile> cast_from(const CtrRef<Profile>& ref);

    static std::shared_ptr<io::IOVector> create_iovector();
};


template <typename CtrName, typename Profile> 
class IterApiBase {
protected:
    
    using AllocatorT = ProfileAllocatorType<Profile>;
    using IterT      = SharedIter<CtrName, Profile>;
    using IterPtr    = CtrSharedPtr<IterT>;
    using CtrT       = SharedCtr<CtrName, AllocatorT, Profile>;
    using CtrPtr     = CtrSharedPtr<CtrT>;
    
    using CtrSizeT   = ProfileCtrSizeT<Profile>;
     
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
    
    const IterPtr& ptr() const;
    IterPtr& ptr();

    CtrApi<CtrName, Profile> ctr();
    
    Iterator clone();
    void reset();
    
    void dump();
    void dump_path();
    void dump_header();
    void check(std::ostream& out, const char* source);

    int32_t local_pos() const;

    const io::IOVector& iovector_view() const;

    const PairPtr& pair() const;
    PairPtr& pair();

    bool next_leaf();
};


template <typename CtrName, typename Profile>
struct CtrMetadataInitializer {
    CtrMetadataInitializer() {
        CtrApi<CtrName, Profile>::init_profile_metadata();
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
CtrMetadataInitializer<CtrName, Profile> init_##__VA_ARGS__ ;\
}



#define MMA1_INSTANTIATE_CTR_BTFL(CtrName, Profile, ...)\
template class IterApiBase<CtrName, Profile>;           \
template class IterApiBTFLBase<CtrName, Profile>;       \
template class CtrApiBase<CtrName, Profile>;            \
template class CtrApiBTFLBase<CtrName, Profile>;        \
template class CtrApi<CtrName, Profile>;                \
template class IterApi<CtrName, Profile>;               \
                                                        \
namespace {                                             \
CtrMetadataInitializer<CtrName, Profile> init_##__VA_ARGS__ ;\
}



#define MMA1_DECLARE_CTRAPI_BASIC_METHODS()                                            \
    CtrApi(const CtrSharedPtr<AllocatorT>& allocator, int32_t command, const CtrID& ctr_id):\
        Base(allocator, command, ctr_id) {}                                                 \
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
