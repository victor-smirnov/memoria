
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


#include <memoria/core/types.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/types/typehash.hpp>

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <boost/pool/object_pool.hpp>

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

struct PoolBase {
    virtual ~PoolBase() noexcept = default;
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
};


template <typename T> struct SharedObjectPoolBase;

namespace detail {

template <typename T>
class ObjectPoolShared {
    int counter_;
public:
    SharedObjectPoolBase<T>* pool;
    T object;

    template <typename... Args>
    ObjectPoolShared(SharedObjectPoolBase<T>* pool, Args&&... args):
        counter_(1),
        pool(pool),
        object(std::forward<Args>(args)...)
    {}

    void ref() noexcept {
        counter_++;
    }

    bool unref() noexcept {
        return --counter_ == 0;
    }
};

}


template <typename T> class PoolSharedPtr;


template <typename T>
struct SharedObjectPoolBase: PoolBase {

    template <typename> friend class PoolSharedPtr;

protected:
    virtual void release(detail::ObjectPoolShared<T>* shared) noexcept = 0;
};


template <typename T>
class PoolSharedPtr {
    T* ptr_;
    detail::ObjectPoolShared<T>* shared_;
public:
    PoolSharedPtr() noexcept :
        ptr_(), shared_()
    {}

    PoolSharedPtr(detail::ObjectPoolShared<T>* shared) noexcept :
        ptr_(&shared->object), shared_(shared)
    {}

    PoolSharedPtr(PoolSharedPtr&& other) noexcept :
        ptr_(other.ptr_),
        shared_(other.shared_)
    {
        other.shared_ = nullptr;
    }

    PoolSharedPtr(const PoolSharedPtr& other) noexcept :
        ptr_(other.ptr_),
        shared_(other.shared_) {
        if (other.shared_) {
            other.shared_->ref();
        }
    }

    ~PoolSharedPtr() noexcept {
        release();
    }

    T* operator->() const noexcept {
        return ptr_;
    }

    T* get() const noexcept {
        return ptr_;
    }

    void reset() noexcept {
        release();
    }

    bool operator==(const PoolSharedPtr& other) const noexcept {
        return shared_ == other.shared_;
    }

    PoolSharedPtr& operator=(const PoolSharedPtr& other) noexcept {
        if (&other != this) {
            release();

            ptr_ = other.ptr_;
            shared_ = other.shared_;

            if (shared_) shared_->ref();
        }
        return *this;
    }

    PoolSharedPtr& operator=(PoolSharedPtr&& other) noexcept {
        if (&other != this) {
            release();

            ptr_ = other.ptr_;
            shared_ = other.shared_;

            other.shared_ = nullptr;
        }
        return *this;
    }

private:
    void release() noexcept {
        if (shared_) {
            if (shared_->unref()) {
                shared_->pool->release(shared_);
            }
        }
    }
};



template <typename ObjectType>
class HeavyObjectPool: public PoolBase {

    using MyType = HeavyObjectPool;

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
    HeavyObjectPool(): head_() {}

    virtual ~HeavyObjectPool() noexcept
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

        return UniquePtr(ptr_cast<ObjectType>(&descr->object_), [&, this](void* object_ptr) {
            Descriptor* descr = ptr_cast<Descriptor>(object_ptr);
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
class SharedObjectPool: public SharedObjectPoolBase<T> {
    boost::object_pool<detail::ObjectPoolShared<T>> alloc_;
public:

    template <typename... Args>
    PoolSharedPtr<T> allocate(Args&&... args) noexcept {
        return alloc_.construct(this, std::forward<Args>(args)...);
    }

protected:
    virtual void release(detail::ObjectPoolShared<T>* shared) noexcept {
        alloc_.destroy(shared);
    }
};

template <typename T>
class UniqueObjectPool: public PoolBase {
    boost::object_pool<T> alloc_;
public:

    template <typename... Args>
    PoolUniquePtr<T> allocate(Args&&... args) noexcept {
        auto ptr = alloc_.malloc();
        return PoolUniquePtr<T> {
            new (ptr) detail::ObjectPoolShared<T>(this, std::forward<Args>(args)...),
            &UniqueObjectPool::release
        };
    }

protected:
    void release(void* shared) noexcept {
        alloc_.destroy(shared);
        alloc_.free(shared);
    }
};


template <typename T>
struct PoolT {};

class ObjectPools {
    using PoolMap = std::unordered_map<std::type_index, PoolBase*>;
    PoolMap pools_;
public:
    ObjectPools() noexcept {}

    ~ObjectPools() noexcept
    {
        for (auto e: pools_)
        {
            delete e.second;
        }
    }

    template <typename PoolType>
    PoolType& get_instance(const PoolT<PoolType>&) noexcept
    {
        auto i = pools_.find(typeid(PoolType));
        if (i != pools_.end())
        {
            return *ptr_cast<PoolType>(i->second);
        }
        else {
            PoolType* pool = new PoolType();
            pools_[typeid(PoolType)] = pool;
            return *ptr_cast<PoolType>(pool);
        }
    }
};

ObjectPools& thread_local_pools();

template <typename T, typename... Args>
PoolSharedPtr<T> pool_allocate_shared(ObjectPools& pools, Args&&... args) {
    using PoolType = SharedObjectPool<T>;
    auto& pool = pools.get_instance(PoolT<PoolType>{});
    return pool.allocate(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
PoolUniquePtr<T> pool_allocate_unique(ObjectPools& pools, Args&&... args) {
    using PoolType = UniqueObjectPool<T>;
    auto& pool = pools.get_instance(PoolT<PoolType>{});
    return pool.allocate(std::forward<Args>(args)...);
}


template <typename T, typename... Args>
PoolSharedPtr<T> TL_pool_allocate_shared(Args&&... args) {
    return pool_allocate_shared<T>(thread_local_pools(), std::forward<Args>(args)...);
}

template <typename T, typename... Args>
PoolUniquePtr<T> TL_pool_allocate_unique(Args&&... args) {
    return pool_allocate_unique<T>(thread_local_pools(), std::forward<Args>(args)...);
}


}
