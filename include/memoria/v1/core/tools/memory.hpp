
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

#include <memory>

namespace memoria {
namespace v1 {

template <typename T>
using CtrSharedPtr = std::shared_ptr<T>;

template <typename T>
using CtrSharedFromThis = std::enable_shared_from_this<T>;

template <typename T, typename... Args>
auto ctr_make_shared(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}



template <typename T>
using SnpSharedPtr = std::shared_ptr<T>;

template <typename T>
using SnpSharedFromThis = std::enable_shared_from_this<T>;

template <typename T, typename... Args>
auto snp_make_shared(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
auto snp_make_shared_init(Args&&... args) {
    auto snp = std::make_shared<T>(std::forward<Args>(args)...);
    snp->post_init();
    return snp;
}


template <typename T>
using AllocSharedPtr = std::shared_ptr<T>;

template <typename T>
using AllocSharedFromThis = std::enable_shared_from_this<T>;

template <typename T, typename... Args>
auto alloc_make_shared(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}



template <typename T>
auto static_pointer_cast(const std::shared_ptr<T>& ptr)
{
    return std::static_pointer_cast<T>(ptr);
}


}}
