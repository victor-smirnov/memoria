
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



namespace memoria {
namespace v1 {

using CtrIOBuffer = DefaultIOBuffer;    
    
template <typename CtrName, typename Profile = DefaultProfile<>> class CtrApi;
template <typename CtrName, typename Profile = DefaultProfile<>> class IterApi;


template <typename CtrName, typename Allocator, typename Profile> class SharedCtr;
template <typename CtrName, typename Profile> class SharedIter;



template <typename CtrName, typename Profile>
struct CtrMetadataInitializer {
    CtrMetadataInitializer() {
        CtrApi<CtrName, Profile>::init();
    }
};

#define MMA1_INSTANTIATE_CTR_BTSS(CtrName, Profile, ...)\
template class IterApiBTSSBase<CtrName, Profile>;       \
template class CtrApiBTSSBase<CtrName, Profile>;        \
template class CtrApi<CtrName, Profile>;                \
template class IterApi<CtrName, Profile>;               \
                                                        \
namespace {                                             \
CtrMetadataInitializer<CtrName, Profile> init_##__VA_ARGS__;\
}



#define MMA1_DECLARE_CTRAPI_BASIC_METHODS()                                                 \
    CtrApi(const std::shared_ptr<AllocatorT>& allocator, Int command, const UUID& name);    \
    ~CtrApi();                                                                              \
                                                                                            \
    CtrApi(const CtrApi&);                                                                  \
    CtrApi(CtrApi&&);                                                                       \
                                                                                            \
    CtrApi& operator=(const CtrApi&);                                                       \
    CtrApi& operator=(CtrApi&&);                                                            \
                                                                                            \
    bool operator==(const CtrApi& other) const;                                             \
    operator bool() const;


#define MMA1_DECLARE_CTRAPI_BTSS_BASIC_METHODS()                                            \
    CtrApi(const std::shared_ptr<AllocatorT>& allocator, Int command, const UUID& name):    \
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
    IterApi(IterPtr);                               \
    ~IterApi();                                     \
                                                    \
    IterApi(const IterApi&);                        \
    IterApi(IterApi&&);                             \
                                                    \
    IterApi& operator=(const IterApi&);             \
    IterApi& operator=(IterApi&&);                  \
                                                    \
    bool operator==(const IterApi& other) const;    \
    operator bool() const;


#define MMA1_DECLARE_ITERAPI_BTSS_BASIC_METHODS()   \
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
