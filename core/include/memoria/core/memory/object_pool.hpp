
// Copyright 2011-2025 Victor Smirnov
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
#include <memoria/core/reflection/typehash.hpp>

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <boost/pool/object_pool.hpp>
#include <boost/smart_ptr/local_shared_ptr.hpp>
#include <boost/smart_ptr/make_local_shared.hpp>
#include <boost/smart_ptr/make_shared.hpp>

#include <memoria/core/tools/result.hpp>

#include <memoria/core/memory/shared_ptr.hpp>

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
    virtual U8String pool_type() = 0;
};


template <typename> class SharedPtr;
template <typename> class UniquePtr;
template <typename> class WeakPtr;

template <typename> class enable_shared_from_this;


namespace detail {

template <typename T>
class HasSharedFromThisMethod;

template <typename T, bool HasBase = HasSharedFromThisMethod<T>::Value>
struct SharedFromThisHelper;



template <typename T>
SharedPtr<T> make_shared_ptr_from(T*, SharedPtrHolder*) noexcept;

template <typename T>
UniquePtr<T> make_unique_ptr_from(T*, SharedPtrHolder*) noexcept;

}





template <typename T>
class UniquePtr {
    using RefHolder = SharedPtrHolder;

    T* ptr_;
    RefHolder* ref_holder_;

    template <typename> friend class SharedPtr;
    template <typename> friend class UniquePtr;

    //friend UniquePtr<T> detail::make_unique_ptr_from(T*, RefHolder*);

public:
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

    operator bool() const {
        return ref_holder_ != nullptr;
    }

    bool is_null() const noexcept {
        return ref_holder_ == nullptr;
    }

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

struct DoRef{};

template <typename T>
class SharedPtr {
    using RefHolder = SharedPtrHolder;

    T* ptr_;
    RefHolder* ref_holder_;

    // Public for now to make GCC happy
public:
    SharedPtr(T* ptr, RefHolder* holder) noexcept :
        ptr_(ptr), ref_holder_(holder)
    {}

    SharedPtr(T* ptr, RefHolder* holder, DoRef) noexcept :
        ptr_(ptr), ref_holder_(holder)
    {
        holder->ref_copy();
    }

private:

    template <typename> friend class UniquePtr;
    template <typename> friend class SharedPtr;
    template <typename> friend class WeakPtr;
    template <typename> class enable_shared_from_this;

    //friend SharedPtr<T> detail::make_shared_ptr_from(T*, RefHolder*);

public:
    using element_type = T;

    SharedPtr() noexcept : ptr_(), ref_holder_() {}

    template<typename U>
    SharedPtr(const SharedPtr<U>& other) noexcept:
        ptr_(other.ptr_),
        ref_holder_(other.ref_holder_)
    {
        if (MMA_LIKELY((bool)ref_holder_)) {
            ref_holder_->ref_copy();
        }
    }


    SharedPtr(const SharedPtr& other) noexcept:
        ptr_(other.ptr_),
        ref_holder_(other.ref_holder_)
    {
        if (MMA_LIKELY((bool)ref_holder_)) {
            ref_holder_->ref_copy();
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

            if (ref_holder_){
                ref_holder_->ref_copy();
            }
        }

        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (MMA_UNLIKELY(ref_holder_ != nullptr)) {
                ref_holder_->unref();
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


    template <typename U>
    bool operator==(const SharedPtr<U>& other) const noexcept {
        return ref_holder_ == other.ref_holder_;
    }

    template <typename U>
    bool operator==(const WeakPtr<U>& other) const noexcept;

    void reset() noexcept {
        if (ref_holder_) {
            ref_holder_->unref();
            ref_holder_ = nullptr;
        }
    }

    RefHolder* release_holder()
    {
        RefHolder* tmp = ref_holder_;
        ref_holder_ = nullptr;
        ptr_ = nullptr;
        return tmp;
    }

    friend void swap(SharedPtr& lhs, SharedPtr& rhs) {
        std::swap(lhs.ptr_, rhs.ptr_);
        std::swap(lhs.ref_holder_, rhs.ref_holder_);
    }

    auto use_count() const {
        if (ref_holder_) {
            return ref_holder_->refs();
        }
        else {
            return 0;
        }
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

    bool is_null() const noexcept {
        return ref_holder_ == nullptr;
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
class WeakPtr {
    using RefHolder = SharedPtrHolder;

    T* ptr_;
    RefHolder* ref_holder_;

    template <typename, bool>
    friend struct detail::SharedFromThisHelper;

    template <typename> friend class WeakPtr;

    WeakPtr(T* ptr, RefHolder* ref_holder):
        ptr_(ptr), ref_holder_(ref_holder)
    {}

public:
    WeakPtr(): ptr_(), ref_holder_() {}


    template<typename U>
    WeakPtr(const SharedPtr<U>& other) noexcept:
        ptr_(other.ptr_),
        ref_holder_(other.ref_holder_)
    {
        if (MMA_LIKELY((bool)ref_holder_)) {
            ref_holder_->ref_weak();
        }
    }


    WeakPtr(const WeakPtr& other) noexcept:
        ptr_(other.ptr_),
        ref_holder_(other.ref_holder_)
    {
        if (MMA_LIKELY((bool)ref_holder_)) {
            ref_holder_->ref_weak();
        }
    }


    WeakPtr(WeakPtr&& other) noexcept:
        ptr_(other.ptr_), ref_holder_(other.ref_holder_)
    {
        other.ref_holder_ = nullptr;
    }

    template<typename U>
    WeakPtr(WeakPtr<U>&& other) noexcept:
        ptr_(other.ptr_), ref_holder_(other.ref_holder_)
    {
        other.ref_holder_ = nullptr;
    }


    ~WeakPtr() noexcept {
        if (ref_holder_) {
            ref_holder_->unref_weak();
        }
    }


    WeakPtr& operator=(const SharedPtr<T>& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (ref_holder_) {
                ref_holder_->unref_weak();
            }

            ptr_ = other.ptr_;
            ref_holder_ = other.ref_holder_;
            ref_holder_->ref_weak();
        }

        return *this;
    }

    WeakPtr& operator=(const WeakPtr& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (ref_holder_) {
                ref_holder_->unref_weak();
            }

            ptr_ = other.ptr_;
            ref_holder_ = other.ref_holder_;
            ref_holder_->ref_weak();
        }

        return *this;
    }


    WeakPtr& operator=(WeakPtr&& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (MMA_UNLIKELY(ref_holder_ != nullptr)) {
                ref_holder_->unref_weak();
            }

            ptr_ = other.ptr_;
            ref_holder_ = other.ref_holder_;
            other.ref_holder_ = nullptr;
        }

        return *this;
    }


    bool operator==(const SharedPtr<T>& other) const noexcept {
        return ref_holder_ == other.ref_holder_;
    }

    bool operator==(const WeakPtr<T>& other) const noexcept {
        return ref_holder_ == other.ref_holder_;
    }

    void reset() noexcept {
        if (ref_holder_) {
            ref_holder_->unref_weak();
            ref_holder_ = nullptr;
        }
    }

    friend void swap(WeakPtr& lhs, WeakPtr& rhs) {
        std::swap(lhs.ptr_, rhs.ptr_);
        std::swap(lhs.ref_holder_, rhs.ref_holder_);
    }


    operator bool() const {
        return ref_holder_ != nullptr;
    }

    auto use_count() const {
        if (ref_holder_) {
            return ref_holder_->weak_refs();
        }
        else {
            return 0;
        }
    }

    SharedPtr<T> lock()
    {
        if (ref_holder_ && ref_holder_->ref_lock()) {
            return SharedPtr<T>(ptr_, ref_holder_);
        }
        else {
            return SharedPtr<T>();
        }
    }
};


template <typename T>
template <typename U>
bool SharedPtr<T>::operator==(const WeakPtr<U>& other) const noexcept {
    return ref_holder_ == other.ref_holder_;
}

template <typename T> class enable_shared_from_this;


namespace detail {

template <typename T>
SharedPtr<T> make_shared_ptr_from(T* obj, SharedPtrHolder* holder) noexcept {
    return SharedPtr<T>(obj, holder);
}

template <typename T>
UniquePtr<T> make_unique_ptr_from(T* obj, SharedPtrHolder* holder) noexcept {
    return UniquePtr<T>(obj, holder);
}





}


template <typename T>
class enable_shared_from_this {
    using RefHolder = SharedPtrHolder;

    mutable T* ptr_{};
    mutable RefHolder* holder_{};

    template <typename U>
    void init_shared_from_this(U* ptr, RefHolder* holder) noexcept {
        ptr_ = ptr;
        holder_ = holder;

        configure_refholder(holder_);
    }

    template <typename>
    friend class SimpleObjectPool;

    template <typename>
    friend class HeavyObjectPool;

    template <typename, bool> friend struct detail::SharedFromThisHelper;

protected:
    enable_shared_from_this() {}

    virtual void configure_refholder(RefHolder*) {}

public:
    SharedPtr<T> shared_from_this() const {
        if (holder_) {
            holder_->ref_lock();
            return detail::make_shared_ptr_from(ptr_, holder_);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("enable_shared_from_this is not initialized").do_throw();
        }
    }

protected:
    RefHolder* get_refholder() const {
        return holder_;
    }
};


namespace detail {

template <typename T>
struct SharedFromThisHelper<T, true> {
    static void initialize(T* obj, SharedPtrHolder* holder) noexcept {
        obj->init_shared_from_this(obj, holder);
    }
};

template <typename T>
struct SharedFromThisHelper<T, false> {
    static void initialize(T*, SharedPtrHolder*) noexcept {
    }
};


// SFINAE test
template <typename T>
class ObjectPoolLifecycleMethods
{
    typedef char one;
    struct two { char x[2]; };

    template <typename C> static one test1( decltype(&C::reset_state) ) ;
    template <typename C> static two test1(...);

    template <typename C> static one test2( decltype(&C::object_pool_init_state) ) ;
    template <typename C> static two test2(...);


    template <typename C> static one test3( decltype(&C::set_buffer) ) ;
    template <typename C> static two test3(...);

public:
    static constexpr bool HasReset      = sizeof(test1<T>(0)) == sizeof(char);
    static constexpr bool HasInit       = sizeof(test2<T>(0)) == sizeof(char);
    static constexpr bool HasSetBuffer  = sizeof(test3<T>(0)) == sizeof(char);
};




template <typename T>
class HasSharedFromThisMethod
{
    typedef char one;
    struct two { char x[2]; };

    template <typename C> static one test( decltype(&C::shared_from_this) ) ;
    template <typename C> static two test(...);

public:
    static constexpr bool Value = sizeof(test<T>(0)) == sizeof(char);
};


template <typename T, bool HasResetMethod = ObjectPoolLifecycleMethods<T>::HasReset>
struct HeavyObjectResetHelper {
    static void process(T*) noexcept {}
};

template <typename T>
struct HeavyObjectResetHelper<T, true> {
    static void process(T* obj) noexcept {
        obj->reset_state();
    }
};

template <typename T, bool HasResetMethod = ObjectPoolLifecycleMethods<T>::HasInit>
struct HeavyObjectInitHelper {
    static void process(T*) {}
};

template <typename T>
struct HeavyObjectInitHelper<T, true> {
    static void process(T* obj) {
        obj->object_pool_init_state();
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

    virtual U8String pool_type() {
        return TypeNameFactory<SimpleObjectPool>::name();
    }

    class RefHolder final: public SharedPtrHolder {
        alignas (T) std::byte object_storage_[sizeof(T)];
        boost::local_shared_ptr<SimpleObjectPool> pool_;

        template <typename> friend class SharedPtr;
        template <typename> friend class UniquePtr;
        template <typename> friend class SimpleObjectPool;

    public:
        template <typename... Args>
        RefHolder(boost::local_shared_ptr<SimpleObjectPool> pool, Args&&... args):
            pool_(std::move(pool))
        {
            new (object_storage_) T(std::forward<Args>(args)...);
        }

        T* ptr() noexcept {
            return std::launder(reinterpret_cast<T*>(object_storage_));
        }

        virtual ~RefHolder() noexcept  = default;

    protected:
        virtual void dispose() noexcept {
            if (MMA_UNLIKELY(is_put_to_list())) {
                put_to_list();
            }
            else {
                finalize_memory_object();
            }
        }

        virtual void destroy() noexcept {
            ptr()->~T();
        }

        void finalize_memory_object() {
            pool_->release(this);
        }
    };

private:

    boost::object_pool<RefHolder> alloc_;

    friend class RefHolder;
    template <typename> friend class SharedPtr;
    template <typename> friend class UniquePtr;
public:

    template <typename... Args>
    SharedPtr<T> allocate_shared(Args&&... args)
    {
        auto ptr = new (alloc_.malloc()) RefHolder(this->shared_from_this(), std::forward<Args>(args)...);

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
        auto ptr = new (alloc_.malloc()) RefHolder(this->shared_from_this(), std::forward<Args>(args)...);
        return detail::make_unique_ptr_from(ptr->ptr(), ptr);
    }

private:
    void release(RefHolder* holder) noexcept {
        alloc_.destroy(holder);
    }
};


template <typename T>
class HeavyObjectPool: public PoolBase, public boost::enable_shared_from_this<HeavyObjectPool<T>> {

    class Descriptor final: public SharedPtrHolder {
        T object_;
        Descriptor* next_;
        boost::local_shared_ptr<HeavyObjectPool> pool_;

        template <typename>
        friend class HeavyObjectPool;

    public:
        Descriptor(boost::local_shared_ptr<HeavyObjectPool> pool):
            object_(),
            next_(), pool_(std::move(pool))
        {}

        T* ptr() noexcept {
            return &object_;
        }

    protected:
        virtual void dispose() noexcept
        {
            if (MMA_UNLIKELY(is_put_to_list())) {
                put_to_list();
            }
            else {
                finalize_memory_object();
            }
        }

        virtual void destroy() noexcept {
            detail::HeavyObjectResetHelper<T>::process(this->ptr());
        }

        void finalize_memory_object() {
            pool_->release(this);
            pool_.reset();
        }
    };

    Descriptor* head_;

public:

    virtual U8String pool_type() {
        return TypeNameFactory<HeavyObjectPool>::name();
    }


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


    UniquePtr<T> get_unique_instance()
    {
        static_assert (
            !std::is_base_of_v<enable_shared_from_this<T>, T>,
            "Can't create instance of a unique object deriving from pool::enable_shared_from_this<>"
        );

        Descriptor* descr = get_or_create();

        auto ptr = detail::make_unique_ptr_from(descr->ptr(), descr);
        detail::HeavyObjectInitHelper<T>::process(descr->ptr());

        return ptr;
    }


    SharedPtr<T> get_shared_instance()
    {
        Descriptor* descr = get_or_create();

        detail::SharedFromThisHelper<T>::initialize(descr->ptr(), descr);

        auto ptr = detail::make_unique_ptr_from(descr->ptr(), descr);
        detail::HeavyObjectInitHelper<T>::process(descr->ptr());

        return ptr;
    }



private:
    void release(Descriptor* entry) noexcept
    {        
        entry->next_ = head_;
        head_ = entry;
    }

    Descriptor* get_or_create()
    {
        Descriptor* descr;

        if (head_)
        {
            descr        = head_;
            head_        = descr->next_;
            descr->next_ = nullptr;
            descr->pool_ = this->shared_from_this();
            descr->init_ref();
        }
        else {
            descr = new Descriptor(this->shared_from_this());
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
            return *reinterpret_cast<PoolType*>((i->second.get()));
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
pool::UniquePtr<T> get_reusable_unique_instance(ObjectPools& pools) {
    using PoolType = pool::HeavyObjectPool<T>;
    auto& pool = pools.get_instance(PoolT<PoolType>{});
    return pool.get_unique_instance();
}


template <typename T>
pool::SharedPtr<T> get_reusable_shared_instance(ObjectPools& pools) {
    using PoolType = pool::HeavyObjectPool<T>;
    auto& pool = pools.get_instance(PoolT<PoolType>{});
    return pool.get_shared_instance();
}





template <typename T, typename... Args>
pool::SharedPtr<T> TL_allocate_shared(Args&&... args) {
    return allocate_shared<T>(
        thread_local_pools(),
        std::forward<Args>(args)...
    );
}

template <typename T, typename... Args>
pool::UniquePtr<T> TL_allocate_unique(Args&&... args) {
    return allocate_unique<T>(
        thread_local_pools(),
        std::forward<Args>(args)...
    );
}

template <typename T>
pool::UniquePtr<T> TL_get_reusable_unique_instance() {
    return get_reusable_unique_instance<T>(
        thread_local_pools()
    );
}

template <typename T>
pool::SharedPtr<T> TL_get_reusable_shared_instance() {
    return get_reusable_shared_instance<T>(
        thread_local_pools()
    );
}

}
