
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

#include <memoria/v1/api/common/ctr_api_btss.hpp>

#include <memoria/v1/core/types/typehash.hpp>

#include <memory>


namespace memoria {
namespace v1 {

template <typename Key>
class Set {
    Key key_;
public:
    Set(Key key):
        key_(key)
    {}

    const Key& key() const {return key_;}
};
    
template <typename Key, typename Profile> 
class CtrApi<Set<Key>, Profile>: public CtrApiBTSSBase<Set<Key>, Profile> {
    using Base = CtrApiBTSSBase<Set<Key>, Profile>;
public:    
    using typename Base::CtrID;
    using typename Base::AllocatorT;
    using typename Base::CtrT;
    using typename Base::CtrPtr;

    using typename Base::Iterator;
    


    MMA1_DECLARE_CTRAPI_BASIC_METHODS()
    
    Iterator find(const Key& key);
    bool contains(const Key& key);
    bool remove(const Key& key);
    bool insert(const Key& key);
};


template <typename Key, typename Profile> 
class IterApi<Set<Key>, Profile>: public IterApiBTSSBase<Set<Key>, Profile> {
    
    using Base = IterApiBTSSBase<Set<Key>, Profile>;
    
    using typename Base::IterT;
    using typename Base::IterPtr;
     
public:
    using Base::insert;

    
    MMA1_DECLARE_ITERAPI_BASIC_METHODS()
    
    Key key() const;
    void insert(const Key& key);
};
    

template <typename Key>
struct TypeHash<Set<Key>>: UInt64Value<
    HashHelper<1101, TypeHashV<Key>>
> {};


template <typename Key>
struct DataTypeTraits<Set<Key>> {
    using CxxType   = EmptyType;
    using InputView = EmptyType;
    using Ptr       = EmptyType*;

    using Parameters = TL<Key>;

    static constexpr size_t MemorySize        = sizeof(EmptyType);
    static constexpr bool IsParametrised      = true;
    static constexpr bool HasTypeConstructors = false;

    static void create_signature(SBuf& buf, const Set<Key>& obj)
    {
        buf << "Set<";
        DataTypeTraits<Key>::create_signature(buf, obj.key());
        buf << ">";
    }

    static void create_signature(SBuf& buf)
    {
        buf << "Set<";
        DataTypeTraits<Key>::create_signature(buf);
        buf << ">";
    }
};

}}
