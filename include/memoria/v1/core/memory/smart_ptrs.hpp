
// Copyright 2018 Victor Smirnov
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

#include <boost/smart_ptr.hpp>
#include <boost/smart_ptr/local_shared_ptr.hpp>
#include <boost/smart_ptr/make_local_shared.hpp>
#include <boost/smart_ptr/atomic_shared_ptr.hpp>

#include <memoria/v1/reactor/smart_ptr.hpp>

namespace memoria {
namespace v1 {

#ifdef MMA1_NO_REACTOR

static inline int32_t current_cpu() {return 0;}
static inline int32_t number_of_cpus() {return 1;}

template <typename T>
using SharedPtr = boost::shared_ptr<T>;

template <typename T>
using LocalSharedPtr = boost::local_shared_ptr<T>;

template <typename T, typename... Args>
auto MakeShared(Args&&... args) {
    return boost::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
auto MakeSharedAt(int cpu, Args&&... args) {
    return boost::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
auto MakeLocalShared(Args&&... args) {
    return boost::make_local_shared<T>(std::forward<Args>(args)...);
}

template <typename T, typename Allocator, typename... Args>
auto AllocateShared(const Allocator& alloc, Args&&... args) {
    return boost::allocate_shared<T>(alloc, std::forward<Args>(args)...);
}

template <typename T, typename Allocator, typename... Args>
auto AllocateSharedAt(int32_t cpu, const Allocator& alloc, Args&&... args) {
    return boost::allocate_shared<T>(alloc, std::forward<Args>(args)...);
}

template <typename T, typename Allocator, typename... Args>
auto AllocateLocalShared(const Allocator& alloc, Args&&... args) {
    return boost::allocate_local_shared<T>(alloc, std::forward<Args>(args)...);
}

template <typename T>
using EnableSharedFromThis = boost::enable_shared_from_this<T>;

using EnableSharedFromRaw = boost::enable_shared_from_raw<T>;

template <typename T>
using WeakPtr = boost::weak_ptr<T>;


template<typename T, typename U>
boost::shared_ptr<T> StaticPointerCast( const boost::shared_ptr<U>& r ) noexcept {
    return reactor::static_pointer_cast<T>(r);
}


template<typename T, typename U>
boost::shared_ptr<T> StaticPointerCast( boost::shared_ptr<U>&& r ) noexcept {
    return reactor::static_pointer_cast<T>(std::move(r));
}



template<typename T, typename U>
boost::shared_ptr<T> DynamicPointerCast( const boost::shared_ptr<U>& r ) noexcept {
    return reactor::dynamic_pointer_cast<T>(r);
}


template<typename T, typename U>
boost::shared_ptr<T> DynamicPointerCast( boost::shared_ptr<U>&& r ) noexcept {
    return reactor::dynamic_pointer_cast<T>(std::move(r));
}


template<typename T, typename U>
boost::shared_ptr<T> ReinterpretPointerCast( const boost::shared_ptr<U>& r ) noexcept {
    return reactor::reinterpret_pointer_cast<T>(r);
}


template<typename T, typename U>
boost::shared_ptr<T> ReinterpretPointerCast( boost::shared_ptr<U>&& r ) noexcept {
    return reactor::reinterpret_pointer_cast<T>(std::move(r));
}


template<typename T, typename U>
boost::shared_ptr<T> ConstPointerCast( const boost::shared_ptr<U>& r ) noexcept {
    return reactor::const_pointer_cast<T>(r);
}


template<typename T, typename U>
boost::shared_ptr<T> ConstPointerCast( boost::shared_ptr<U>&& r ) noexcept {
    return reactor::const_pointer_cast<T>(std::move(r));
}




template<typename T, typename U>
boost::local_shared_ptr<T> StaticPointerCast( const boost::local_shared_ptr<U>& r ) noexcept {
    return reactor::static_pointer_cast<T>(r);
}


template<typename T, typename U>
boost::local_shared_ptr<T> StaticPointerCast( boost::local_shared_ptr<U>&& r ) noexcept {
    return reactor::static_pointer_cast<T>(std::move(r));
}



template<typename T, typename U>
boost::local_shared_ptr<T> DynamicPointerCast( const boost::local_shared_ptr<U>& r ) noexcept {
    return reactor::dynamic_pointer_cast<T>(r);
}


template<typename T, typename U>
boost::local_shared_ptr<T> DynamicPointerCast( boost::local_shared_ptr<U>&& r ) noexcept {
    return reactor::dynamic_pointer_cast<T>(std::move(r));
}


template<typename T, typename U>
boost::local_shared_ptr<T> ReinterpretPointerCast( const boost::local_shared_ptr<U>& r ) noexcept {
    return reactor::reinterpret_pointer_cast<T>(r);
}


template<typename T, typename U>
boost::local_shared_ptr<T> ReinterpretPointerCast( boost::local_shared_ptr<U>&& r ) noexcept {
    return reactor::reinterpret_pointer_cast<T>(std::move(r));
}


template<typename T, typename U>
boost::local_shared_ptr<T> ConstPointerCast( const boost::local_shared_ptr<U>& r ) noexcept {
    return reactor::const_pointer_cast<T>(r);
}


template<typename T, typename U>
boost::local_shared_ptr<T> ConstPointerCast( boost::local_shared_ptr<U>&& r ) noexcept {
    return reactor::const_pointer_cast<T>(std::move(r));
}



#else

namespace reactor {

int32_t current_cpu();
int32_t number_of_cpus();

}

template <typename T>
using SharedPtr = reactor::shared_ptr<T>;

template <typename T>
using LocalSharedPtr = reactor::local_shared_ptr<T>;

template <typename T, typename... Args>
auto MakeShared(Args&&... args) {
    return reactor::make_shared_at<T>(reactor::current_cpu(), std::forward<Args>(args)...);
}

template <typename T, typename... Args>
auto MakeSharedAt(int32_t cpu, Args&&... args) {
    return reactor::make_shared_at<T>(cpu, std::forward<Args>(args)...);
}

template <typename T, typename... Args>
auto MakeLocalShared(Args&&... args) {
    return reactor::make_local_shared<T>(std::forward<Args>(args)...);
}

template <typename T, typename Allocator, typename... Args>
auto AllocateShared(const Allocator& alloc, Args&&... args) {
    return reactor::allocate_shared_at<T>(reactor::current_cpu(), alloc, std::forward<Args>(args)...);
}

template <typename T, typename Allocator, typename... Args>
auto AllocateLocalShared(int32_t cpu, const Allocator& alloc, Args&&... args) {
    return reactor::allocate_shared_at<T>(cpu, alloc, std::forward<Args>(args)...);
}

template <typename T, typename Allocator, typename... Args>
auto AllocateLocalShared(const Allocator& alloc, Args&&... args) {
    return reactor::allocate_local_shared<T>(alloc, std::forward<Args>(args)...);
}

template <typename T>
using EnableSharedFromThis = reactor::enable_shared_from_this<T>;

using EnableSharedFromRaw = reactor::enable_shared_from_raw;

template <typename T>
using WeakPtr = reactor::weak_ptr<T>;



template<typename T, typename U>
reactor::shared_ptr<T> StaticPointerCast( const reactor::shared_ptr<U>& r ) noexcept {
    return reactor::static_pointer_cast<T>(r);
}


template<typename T, typename U>
reactor::shared_ptr<T> StaticPointerCast( reactor::shared_ptr<U>&& r ) noexcept {
    return reactor::static_pointer_cast<T>(std::move(r));
}



template<typename T, typename U>
reactor::shared_ptr<T> DynamicPointerCast( const reactor::shared_ptr<U>& r ) noexcept {
    return reactor::dynamic_pointer_cast<T>(r);
}


template<typename T, typename U>
reactor::shared_ptr<T> DynamicPointerCast( reactor::shared_ptr<U>&& r ) noexcept {
    return reactor::dynamic_pointer_cast<T>(std::move(r));
}


template<typename T, typename U>
reactor::shared_ptr<T> ReinterpretPointerCast( const reactor::shared_ptr<U>& r ) noexcept {
    return reactor::reinterpret_pointer_cast<T>(r);
}


template<typename T, typename U>
reactor::shared_ptr<T> ReinterpretPointerCast( reactor::shared_ptr<U>&& r ) noexcept {
    return reactor::reinterpret_pointer_cast<T>(std::move(r));
}


template<typename T, typename U>
reactor::shared_ptr<T> ConstPointerCast( const reactor::shared_ptr<U>& r ) noexcept {
    return reactor::const_pointer_cast<T>(r);
}


template<typename T, typename U>
reactor::shared_ptr<T> ConstPointerCast( reactor::shared_ptr<U>&& r ) noexcept {
    return reactor::const_pointer_cast<T>(std::move(r));
}




template<typename T, typename U>
reactor::local_shared_ptr<T> StaticPointerCast( const reactor::local_shared_ptr<U>& r ) noexcept {
    return reactor::static_pointer_cast<T>(r);
}


template<typename T, typename U>
reactor::local_shared_ptr<T> StaticPointerCast( reactor::local_shared_ptr<U>&& r ) noexcept {
    return reactor::static_pointer_cast<T>(std::move(r));
}



template<typename T, typename U>
reactor::local_shared_ptr<T> DynamicPointerCast( const reactor::local_shared_ptr<U>& r ) noexcept {
    return reactor::dynamic_pointer_cast<T>(r);
}


template<typename T, typename U>
reactor::local_shared_ptr<T> DynamicPointerCast( reactor::local_shared_ptr<U>&& r ) noexcept {
    return reactor::dynamic_pointer_cast<T>(std::move(r));
}


template<typename T, typename U>
reactor::local_shared_ptr<T> ReinterpretPointerCast( const reactor::local_shared_ptr<U>& r ) noexcept {
    return reactor::reinterpret_pointer_cast<T>(r);
}


template<typename T, typename U>
reactor::local_shared_ptr<T> ReinterpretPointerCast( reactor::local_shared_ptr<U>&& r ) noexcept {
    return reactor::reinterpret_pointer_cast<T>(std::move(r));
}


template<typename T, typename U>
reactor::local_shared_ptr<T> ConstPointerCast( const reactor::local_shared_ptr<U>& r ) noexcept {
    return reactor::const_pointer_cast<T>(r);
}


template<typename T, typename U>
reactor::local_shared_ptr<T> ConstPointerCast( reactor::local_shared_ptr<U>&& r ) noexcept {
    return reactor::const_pointer_cast<T>(std::move(r));
}


#endif

}}
