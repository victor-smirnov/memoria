
// Copyright 2011-2022 Victor Smirnov
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
#include <boost/smart_ptr/local_shared_ptr.hpp>

#include <iostream>
#include <type_traits>
#include <cstddef>
#include <unordered_map>
#include <typeinfo>
#include <typeindex>



namespace memoria {
namespace pool {

struct PoolBase {
    virtual ~PoolBase() noexcept = default;
};


template <typename> class SharedPtr;
template <typename> class UniquePtr;

template <typename> class enable_shared_from_this;






namespace detail {

class ObjectPoolRefHolder {
    int64_t references_{1};


    template <typename> friend class memoria::pool::SharedPtr;
    template <typename> friend class memoria::pool::UniquePtr;

public:
    ObjectPoolRefHolder() {}

    virtual ~ObjectPoolRefHolder() noexcept = default;

protected:
    virtual void release() noexcept = 0;

    void ref() noexcept {
        references_++;
    }

    void unref() noexcept {
        if (MMA_UNLIKELY(--references_ == 0)) {
            release();
        }
    }

public:
    int64_t refs() const {
        return references_;
    }
};

template <typename T>
SharedPtr<T> make_shared_ptr_from(T*, ObjectPoolRefHolder*);

template <typename T>
UniquePtr<T> make_unique_ptr_from(T*, ObjectPoolRefHolder*);

}





template <typename T>
class UniquePtr {
    using RefHolder = detail::ObjectPoolRefHolder;

    T* ptr_;
    RefHolder* ref_holder_;

    template <typename> friend class SharedPtr;
    template <typename> friend class UniquePtr;



    friend UniquePtr<T> detail::make_unique_ptr_from(T*, RefHolder*);

    UniquePtr(T* ptr, RefHolder* holder) noexcept :
        ptr_(ptr), ref_holder_(holder)
    {}

public:
    using element_type = T;

    UniquePtr() noexcept : ptr_(), ref_holder_() {}

    template<typename U>
    UniquePtr(UniquePtr<U>&& other) noexcept:
        ptr_(other.ptr_), ref_holder_(other.ref_holder_)
    {
        other.ref_holder_ = nullptr;
    }


    UniquePtr(const UniquePtr&) = delete;

    ~UniquePtr() noexcept {
        if (ref_holder_) {
            ref_holder_->unref();
        }
    }

    UniquePtr& operator=(const UniquePtr&) = delete;
    UniquePtr& operator=(UniquePtr&& other) noexcept
    {
        if (&other != this) {
            if (ref_holder_) {
                ref_holder_->unref();
            }

            ptr_ = other.ptr_;
            ref_holder_ = other.ref_holder_;
            other.ref_holder_ = nullptr;
        }

        return *this;
    }

    bool operator==(const UniquePtr& other) const noexcept {
        return ref_holder_ == other.ref_holder_;
    }

    bool operator==(const SharedPtr<T>& other) const noexcept;

    void reset() noexcept {
        if (ref_holder_) {
            ref_holder_->unref();
            ref_holder_ = nullptr;
        }
    }

    friend void swap(UniquePtr& lhs, UniquePtr& rhs) {
        std::swap(lhs.ptr_, rhs.ptr_);
        std::swap(lhs.ref_holder_, rhs.ref_holder_);
    }

    T* operator->() {return ptr_;}
    T* operator->() const {return ptr_;}

    T& operator*() {return *ptr_;}
    T& operator*() const {return *ptr_;}

    T* get() {return ptr_;}
    T* get() const {return ptr_;}

    template<typename U>
    UniquePtr<U> static_cast_to() && {
        UniquePtr<U> pp(static_cast<U*>(ptr_), ref_holder_);
        ref_holder_ = nullptr;
        return pp;
    }

    template<typename U>
    UniquePtr<U> dynamic_cast_to() && {
        UniquePtr<U> pp(static_cast<U*>(ptr_), ref_holder_);
        ref_holder_ = nullptr;
        return pp;
    }

    template<typename U>
    UniquePtr<U> const_cast_to() && {
        UniquePtr<U> pp(const_cast<U*>(ptr_), ref_holder_);
        ref_holder_ = nullptr;
        return pp;
    }
};






template <typename T>
class SharedPtr {
    using RefHolder = detail::ObjectPoolRefHolder;

    T* ptr_;
    RefHolder* ref_holder_;

    SharedPtr(T* ptr, RefHolder* holder) noexcept :
        ptr_(ptr), ref_holder_(holder)
    {
        //ref_holder_->ref();
    }

    template <typename> friend class UniquePtr;
    template <typename> friend class SharedPtr;
    template <typename> class enable_shared_from_this;

    friend SharedPtr<T> detail::make_shared_ptr_from(T*, RefHolder*);

public:
    using element_type = T;

    SharedPtr() noexcept : ptr_(), ref_holder_() {}

    template<typename U>
    SharedPtr(const SharedPtr<U>& other) noexcept:
        ptr_(other.ptr_),
        ref_holder_(other.ref_holder_)
    {
        if (MMA_LIKELY((bool)ref_holder_)) {
            ref_holder_->ref();
        }
    }


    SharedPtr(const SharedPtr& other) noexcept:
        ptr_(other.ptr_),
        ref_holder_(other.ref_holder_)
    {
        if (MMA_LIKELY((bool)ref_holder_)) {
            ref_holder_->ref();
        }
    }


    SharedPtr(SharedPtr&& other) noexcept:
        ptr_(other.ptr_), ref_holder_(other.ref_holder_)
    {
        other.ref_holder_ = nullptr;
    }

    template<typename U>
    SharedPtr(SharedPtr<U>&& other) noexcept:
        ptr_(other.ptr_), ref_holder_(other.ref_holder_)
    {
        other.ref_holder_ = nullptr;
    }

    template<typename U>
    SharedPtr(UniquePtr<U>&& other) noexcept:
        ptr_(other.ptr_), ref_holder_(other.ref_holder_)
    {
        other.ref_holder_ = nullptr;
    }

    ~SharedPtr() noexcept {
        if (ref_holder_) {
            ref_holder_->unref();
        }
    }

    SharedPtr& operator=(const SharedPtr& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (ref_holder_) {
                ref_holder_->unref();
            }

            ptr_ = other.ptr_;
            ref_holder_ = other.ref_holder_;
            ref_holder_->ref();
        }

        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (MMA_UNLIKELY(ref_holder_ != nullptr)) {
                ref_holder_->release();
            }

            ptr_ = other.ptr_;
            ref_holder_ = other.ref_holder_;
            other.ref_holder_ = nullptr;
        }

        return *this;
    }

    SharedPtr& operator=(UniquePtr<T>&& other) noexcept {
        if (MMA_UNLIKELY(ref_holder_ != nullptr)) {
            ref_holder_->unref();
        }

        ptr_ = other.ptr_;
        ref_holder_ = other.ref_holder_;
        other.ref_holder_ = nullptr;

        return *this;
    }


    bool operator==(const UniquePtr<T>& other) const noexcept {
        return ref_holder_ == other.ref_holder_;
    }

    bool operator==(const SharedPtr<T>& other) const noexcept {
        return ref_holder_ == other.ref_holder_;
    }

    void reset() noexcept {
        if (ref_holder_) {
            ref_holder_->unref();
            ref_holder_ = nullptr;
        }
    }

    friend void swap(SharedPtr& lhs, SharedPtr& rhs) {
        std::swap(lhs.ptr_, rhs.ptr_);
        std::swap(lhs.ref_holder_, rhs.ref_holder_);
    }

    T* operator->() {return ptr_;}
    T* operator->() const {return ptr_;}

    T& operator*() {return *ptr_;}
    T& operator*() const {return *ptr_;}

    T* get() {return ptr_;}
    T* get() const {return ptr_;}

    operator bool() const {
        return ref_holder_ != nullptr;
    }

    template<typename U>
    SharedPtr<U> static_cast_to() && {
        SharedPtr<U> pp (static_cast<U*>(ptr_), ref_holder_);
        ref_holder_ = nullptr;
        return pp;
    }

    template<typename U>
    SharedPtr<U> dynamic_cast_to() && {
        SharedPtr<U> pp(dynamic_cast<U*>(ptr_), ref_holder_);
        ref_holder_ = nullptr;
        return pp;
    }

    template<typename U>
    SharedPtr<U> const_cast_to() && {
        SharedPtr<U> pp(const_cast<U*>(ptr_), ref_holder_);
        ref_holder_ = nullptr;
        return pp;
    }
};

template <typename T>
bool UniquePtr<T>::operator==(const SharedPtr<T>& other) const noexcept {
    return ref_holder_ == other.ref_holder_;
}

template <typename T> class enable_shared_from_this;


namespace detail {

template <typename T>
SharedPtr<T> make_shared_ptr_from(T* obj, ObjectPoolRefHolder* holder) {
    return SharedPtr<T>(obj, holder);
}

template <typename T>
UniquePtr<T> make_unique_ptr_from(T* obj, ObjectPoolRefHolder* holder) {
    return UniquePtr<T>(obj, holder);
}



template <typename T, bool HasBase = std::is_base_of_v<enable_shared_from_this<T>, T>>
struct SharedFromThisHelper;

}


template <typename T>
class enable_shared_from_this {
    using RefHolder = detail::ObjectPoolRefHolder;
    T* ptr_;
    RefHolder* holder_{};

    void init(T* ptr, RefHolder* holder) {
        ptr_ = ptr;
        holder_ = holder;
    }

    template <typename>
    friend class SimpleObjectPool;

    template <typename>
    friend class HeavyObjectPool;

    template <typename, bool> friend struct detail::SharedFromThisHelper;

public:
    enable_shared_from_this() {}

    SharedPtr<T> shared_from_this() {
        return detail::make_shared_ptr_from(ptr_, holder_);
    }
};


namespace detail {

template <typename T>
struct SharedFromThisHelper<T, true> {
    static void initialize(T* obj, ObjectPoolRefHolder* holder) {
        static_cast<enable_shared_from_this<T>*>(obj)->init(obj, holder);
    }
};

template <typename T>
struct SharedFromThisHelper<T, false> {
    static void initialize(T*, ObjectPoolRefHolder*) {
    }
};


// SFINAE test
template <typename T>
class HasObjectResetMethod
{
    typedef char one;
    struct two { char x[2]; };

    template <typename C> static one test( decltype(&C::reset_state) ) ;
    template <typename C> static two test(...);

public:
    static constexpr bool Value = sizeof(test<T>(0)) == sizeof(char);
};


template <typename T, bool HasResetMethod = HasObjectResetMethod<T>::Value>
struct HeavyObjectResetHelper {
    static void process(T*) {}
};

template <typename T>
struct HeavyObjectResetHelper<T, true> {
    static void process(T* obj) {
        obj->reset_state();
    }
};

}


template <typename T, typename U>
SharedPtr<T> memoria_static_pointer_cast(SharedPtr<U> ptr) {
    return std::move(ptr).template static_cast_to<T>();
}

template <typename T, typename U>
UniquePtr<T> memoria_static_pointer_cast(UniquePtr<U>&& ptr) {
    return std::move(ptr).template static_cast_to<T>();
}

template <typename T, typename U>
SharedPtr<T> memoria_dynamic_pointer_cast(SharedPtr<U> ptr) {
    return std::move(ptr).template dynamic_cast_to<T>();
}

template <typename T, typename U>
UniquePtr<T> memoria_dynamic_pointer_cast(UniquePtr<U>&& ptr) {
    return std::move(ptr).template dynamic_cast_to<T>();
}

template <typename T, typename U>
SharedPtr<T> memoria_const_pointer_cast(SharedPtr<U> ptr) {
    return std::move(ptr).template const_cast_to<T>();
}

template <typename T, typename U>
UniquePtr<T> memoria_const_pointer_cast(UniquePtr<U>&& ptr) {
    return std::move(ptr).template const_cast_to<T>();
}

template <typename T>
class SimpleObjectPool: public PoolBase, public boost::enable_shared_from_this<SimpleObjectPool<T>> {
public:

    class RefHolder: public detail::ObjectPoolRefHolder {
        T object_;
        boost::local_shared_ptr<SimpleObjectPool> pool_;

        template <typename> friend class SharedPtr;
        template <typename> friend class UniquePtr;
        template <typename> friend class SimpleObjectPool;

    public:
        template <typename... Args>
        RefHolder(boost::local_shared_ptr<SimpleObjectPool> pool, Args&&... args):
            object_(std::forward<Args>(args)...),
            pool_(std::move(pool))
        {}

        virtual ~RefHolder() noexcept  = default;

        T* ptr() noexcept {
            return &object_;
        }

    protected:
        virtual void release() noexcept {
            pool_->release(this);
        }
    };

private:

    boost::object_pool<RefHolder> alloc_;

    friend class RefHolder;
    template <typename> friend class SharedPtr;
    template <typename> friend class UniquePtr;
public:

    ~SimpleObjectPool() noexcept {

    }

    template <typename... Args>
    SharedPtr<T> allocate_shared(Args&&... args) {
        auto ptr = alloc_.construct(this->shared_from_this(), std::forward<Args>(args)...);

        detail::SharedFromThisHelper<T>::initialize(ptr->ptr(), ptr);

        return detail::make_shared_ptr_from(ptr->ptr(), ptr);
    }

    template <typename... Args>
    UniquePtr<T> allocate_unique(Args&&... args)
    {
        static_assert (
            !std::is_base_of_v<enable_shared_from_this<T>, T>,
            "Can't create instance of a unique object deriving from pool::enable_shared_from_this<>"
        );
        auto ptr = alloc_.construct(this->shared_from_this(), std::forward<Args>(args)...);
        return detail::make_unique_ptr_from(ptr->ptr(), ptr);
    }

private:
    void release(RefHolder* holder) noexcept {
        if (holder == (void*)0x626000000100) {
            int a = 0; a++;
        }

        alloc_.destroy(holder);
    }
};



template <typename T>
class HeavyObjectPool: public PoolBase, public boost::enable_shared_from_this<HeavyObjectPool<T>> {

    class Descriptor: public detail::ObjectPoolRefHolder {
        T object_;
        Descriptor* next_;
        boost::local_shared_ptr<HeavyObjectPool> pool_;

        template <typename>
        friend class HeavyObjectPool;

    public:

        Descriptor(boost::local_shared_ptr<HeavyObjectPool> pool, T&& value):
            object_(std::move(value)),
            next_(), pool_(std::move(pool))
        {}

        virtual void release() noexcept {
            pool_->release(this);
        }

        T* ptr() noexcept {
            return &object_;
        }
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


    UniquePtr<T> get_unique_instance(std::function<T ()> factory = []() {
        return T();
    })
    {
        static_assert (
            !std::is_base_of_v<enable_shared_from_this<T>, T>,
            "Can't create instance of a unique object deriving from pool::enable_shared_from_this<>"
        );

        Descriptor* descr = get_or_create(factory);

        return detail::make_unique_ptr_from(descr->ptr(), descr);
    }


    SharedPtr<T> get_shared_instance(std::function<T ()> factory = []() {
        return T();
    })
    {
        Descriptor* descr = get_or_create(factory);

        detail::SharedFromThisHelper<T>::initialize(descr->ptr(), descr);

        return detail::make_unique_ptr_from(descr->ptr(), descr);
    }



private:
    void release(Descriptor* entry) noexcept
    {
        detail::HeavyObjectResetHelper<T>::process(entry->ptr());
        entry->next_ = head_;
        head_ = entry;
    }

    Descriptor* get_or_create(std::function<T ()> factory)
    {

        Descriptor* descr;

        if (head_)
        {
            descr        = head_;
            head_        = descr->next_;
            descr->next_ = nullptr;
        }
        else {
            descr = new Descriptor(this->shared_from_this(), factory());
        }

        return descr;
    }
};

}

template <typename T>
struct PoolT {};

class ObjectPools {
    using PoolMap = std::unordered_map<std::type_index, boost::local_shared_ptr<pool::PoolBase>>;
    PoolMap pools_;
public:
    ObjectPools() noexcept {}
    ~ObjectPools() noexcept {}

    template <typename PoolType>
    PoolType& get_instance(const PoolT<PoolType>&) noexcept
    {
        auto i = pools_.find(typeid(PoolType));
        if (i != pools_.end()) {
            return *ptr_cast<PoolType>(i->second.get());
        }
        else {
            auto ptr = boost::make_shared<PoolType>();
            pools_[typeid(PoolType)] = ptr;
            return *ptr;
        }
    }
};


ObjectPools& thread_local_pools();

template <typename T, typename... Args>
pool::SharedPtr<T> allocate_shared(ObjectPools& pools, Args&&... args) {
    using PoolType = pool::SimpleObjectPool<T>;
    auto& pool = pools.get_instance(PoolT<PoolType>{});
    return pool.allocate_shared(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
pool::UniquePtr<T> allocate_unique(ObjectPools& pools, Args&&... args) {
    using PoolType = pool::SimpleObjectPool<T>;
    auto& pool = pools.get_instance(PoolT<PoolType>{});
    return pool.allocate_unique(std::forward<Args>(args)...);
}


template <typename T>
pool::UniquePtr<T> get_reusable_unique_instance(ObjectPools& pools, std::function<T ()> factory = []{return T();}) {
    using PoolType = pool::HeavyObjectPool<T>;
    auto& pool = pools.get_instance(PoolT<PoolType>{});
    return pool.get_unique_instance(factory);
}


template <typename T>
pool::SharedPtr<T> get_reusable_shared_instance(ObjectPools& pools, std::function<T ()> factory = []{return T();}) {
    using PoolType = pool::HeavyObjectPool<T>;
    auto& pool = pools.get_instance(PoolT<PoolType>{});
    return pool.get_shared_instance(factory);
}





template <typename T, typename... Args>
pool::SharedPtr<T> TL_allocate_shared(Args&&... args) {
    return allocate_shared(
        thread_local_pools(),
        std::forward<Args>(args)...
    );
}

template <typename T, typename... Args>
pool::UniquePtr<T> TL_allocate_unique(Args&&... args) {
    return allocate_unique(
        thread_local_pools(),
        std::forward<Args>(args)...
    );
}

template <typename T>
pool::UniquePtr<T> TL_get_reusable_unique_instance(std::function<T ()> factory = []{return T();}) {
    return get_reusable_unique_instance(
        thread_local_pools(),
        factory
    );
}

template <typename T>
pool::SharedPtr<T> TL_get_reusable_shared_instance(std::function<T ()> factory = []{return T();}) {
    return get_reusable_shared_instance(
        thread_local_pools(),
        factory
    );
}

}
