
// Copyright 2011 Victor Smirnov
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
#include <memoria/v1/core/types/type2type.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/types/typehash.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <iostream>
#include <type_traits>
#include <cstddef>
#include <unordered_map>
#include <typeinfo>
#include <typeindex>



/**
 * Note. This simple list-based Object Pool is intended to be used inside iterators, because
 * its entries hold only classic pointer to ObjectPool structure. So users must guarantee that
 * all pool's entries are destroyed before pool is destroyed.
 *
 * This is true for containers/iterators. ObjectPool is intended to be used by iterators to reuse
 * heavy objects like Input/Output adapters. ObjectPool should be owned by container object, then a pool
 * will be deleted only after all iterators are already deleted.
 */


namespace memoria {
namespace v1 {

struct PoolBase {
    virtual ~PoolBase() noexcept {}
};


template <typename ObjectType>
class PoolUniquePtr: public PoolBase, public std::unique_ptr<ObjectType, std::function<void (void*)>> {
    using Base = std::unique_ptr<ObjectType, std::function<void (void*)>>;
public:
    PoolUniquePtr(ObjectType* tt, std::function<void (void*)> dtr):
        Base(tt, std::move(dtr))
    {}

    PoolUniquePtr(const PoolUniquePtr&) = delete;
    PoolUniquePtr() = default;
    PoolUniquePtr(PoolUniquePtr&& other): Base(std::move(other)) {}

    PoolUniquePtr& operator=(const PoolUniquePtr&) = delete;
    PoolUniquePtr& operator=(PoolUniquePtr&& other)
    {
        Base::operator=(std::move(other));
        return *this;
    }

    bool operator==(const PoolUniquePtr& other) const
    {
        return Base::operator==(other);
    }

    auto convert_to_ptr() {
        return new PoolUniquePtr(std::move(*this));
    }
};




template <typename ObjectType>
class ObjectPool: public PoolBase {

    using MyType = ObjectPool<ObjectType>;

    using UniquePtr = PoolUniquePtr<ObjectType>;
    using SharedPtr = std::shared_ptr<ObjectType>;

    struct Descriptor {
        ObjectType object_;

        Descriptor* next_;

        template <typename... Args>
        Descriptor(Args&&... args): object_(std::forward<Args>(args)...), next_() {}
    };

    Descriptor* head_;


public:
    ObjectPool(): head_() {}

    virtual ~ObjectPool()
    {
        while (head_)
        {
            auto tmp = head_;
            head_    = head_->next_;

            delete tmp;
        }
    }

    template <typename... Args>
    auto get_unique(Args&&... args)
    {
        Descriptor* descr;

        if (head_)
        {
            descr        = head_;
            head_        = descr->next_;
            descr->next_ = nullptr;
        }
        else {
            descr = new Descriptor(std::forward<Args>(args)...);
        }

        return UniquePtr(T2T<ObjectType*>(&descr->object_), [&, this](void* object_ptr) {
            Descriptor* descr = T2T<Descriptor*>(object_ptr);
            this->release(descr);
        });
    }

    template <typename... Args>
    auto get_shared(Args&&... args)
    {
        return SharedPtr(get_unique(std::forward<Args>(args)...));
    }


private:

    void release(Descriptor* entry)
    {
        entry->object_.clear();

        entry->next_ = head_;
        head_ = entry;
    }
};

template <typename T>
struct PoolT {};

class ObjectPools {
    using PoolMap = std::unordered_map<std::type_index, PoolBase*>;
    PoolMap pools_;
public:
    ObjectPools() {}

    ~ObjectPools()
    {
        for (auto e: pools_)
        {
            delete e.second;
        }
    }

    template <typename PoolType>
    PoolType& get_instance(const PoolT<PoolType>&)
    {
        auto i = pools_.find(typeid(PoolType));
        if (i != pools_.end())
        {
            return *T2T<PoolType*>(i->second);
        }
        else {
            PoolType* pool = new PoolType();
            pools_[typeid(PoolType)] = pool;
            return *T2T<PoolType*>(pool);
        }
    }
};




}}
