
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

namespace memoria {
namespace v1 {

template <typename T>
using SharedPtr = boost::shared_ptr<T>;

template <typename T>
using LocalSharedPtr = boost::local_shared_ptr<T>;

template <typename... Args>
auto MakeShared(Args&&... args) {
    return boost::make_shared(std::forward<Args>(args)...);
}

template <typename... Args>
auto MakeLocalShared(Args&&... args) {
    return boost::make_local_shared(std::forward<Args>(args)...);
}

template <typename Allocator, typename... Args>
auto AllocateShared(const Allocator& alloc, Args&&... args) {
    return boost::allocate_shared(alloc, std::forward<Args>(args)...);
}

template <typename Allocator, typename... Args>
auto AllocateLocalShared(const Allocator& alloc, Args&&... args) {
    return boost::allocate_local_shared(alloc, std::forward<Args>(args)...);
}

}}
