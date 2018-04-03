
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

#include <memoria/v1/core/memory/smart_ptrs.hpp>

#include <memory>

namespace memoria {
namespace v1 {

template <typename T>
using CtrSharedPtr = LocalSharedPtr<T>;

template <typename T>
using CtrSharedFromThis = EnableSharedFromThis<T>;

template <typename T, typename... Args>
auto ctr_make_shared(Args&&... args) {
    return MakeLocalShared<T>(std::forward<Args>(args)...);
}






template <typename T>
using SnpSharedPtr = LocalSharedPtr<T>;

template <typename T>
using SnpSharedFromThis = EnableSharedFromThis<T>;

template <typename T, typename... Args>
auto snp_make_shared(Args&&... args) {
    return MakeLocalShared<T>(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
auto snp_make_shared_init(Args&&... args) {
    auto snp = MakeLocalShared<T>(std::forward<Args>(args)...);
    snp->post_init();
    return snp;
}


template <typename T>
using AllocSharedPtr = LocalSharedPtr<T>;

template <typename T>
using AllocSharedFromThis = EnableSharedFromThis<T>;

template <typename T, typename... Args>
auto alloc_make_shared(Args&&... args) {
    return MakeLocalShared<T>(std::forward<Args>(args)...);
}



template <typename T>
auto static_pointer_cast(const std::shared_ptr<T>& ptr)
{
    return std::static_pointer_cast<T>(ptr);
}


template <typename T>
auto static_pointer_cast(const LocalSharedPtr<T>& ptr)
{
    return MakeLocalShared<T>(ptr);
}

}}
