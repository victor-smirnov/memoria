
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

#include <memoria/core/memory/smart_ptrs.hpp>

#include <memoria/core/tools/result.hpp>

#include <memory>

namespace memoria {

template <typename T>
using CtrSharedPtr = LocalSharedPtr<T>;

template <typename T>
using CtrSharedFromThis = EnableSharedFromThis<T>;

template <typename T, typename... Args>
auto ctr_make_shared(Args&&... args) {
    return MakeLocalShared<T>(std::forward<Args>(args)...);
}


template <typename T>
using SnpSharedPtr = SharedPtr<T>;

template <typename T>
using SnpSharedFromThis = EnableSharedFromThis<T>;


template <typename T, typename... Args>
auto snp_make_shared(Args&&... args) {
    auto snp = MakeLocalShared<T>(std::forward<Args>(args)...);
    return snp;
}


template <typename T, typename... Args>
auto snp_make_shared_init(Args&&... args) {
    auto snp = MakeLocalShared<T>(std::forward<Args>(args)...);
    snp->post_init().get_or_throw();
    return snp;
}



template <typename T>
using AllocSharedPtr = SharedPtr<T>;

template <typename T>
using AllocSharedFromThis = EnableSharedFromThis<T>;

template <typename T, typename... Args>
auto alloc_make_shared(Args&&... args) {
    return MakeLocalShared<T>(std::forward<Args>(args)...);
}



template <typename T, typename TT>
auto memoria_static_pointer_cast(const std::shared_ptr<TT>& ptr)
{
    return std::static_pointer_cast<T>(ptr);
}


template <typename T, typename TT>
auto memoria_static_pointer_cast(const LocalSharedPtr<TT>& ptr)
{
    return StaticPointerCast<T>(ptr);
}

template <typename T, typename TT>
auto memoria_static_pointer_cast(const SharedPtr<TT>& ptr)
{
    return StaticPointerCast<T>(ptr);
}


template <typename T, typename TT>
Result<SharedPtr<T>> memoria_static_pointer_cast(Result<SharedPtr<TT>>&& ptr) noexcept
{
    if (MMA_LIKELY(ptr.is_ok()))
    {
        return Result<SharedPtr<T>>::of(StaticPointerCast<T>(std::move(ptr).get()));
    }
    else {
        return std::move(ptr).transfer_error();
    }
}


template <typename T, typename TT>
Result<LocalSharedPtr<T>> memoria_static_pointer_cast(Result<LocalSharedPtr<TT>>&& ptr) noexcept
{
    if (MMA_LIKELY(ptr.is_ok()))
    {
        return Result<LocalSharedPtr<T>>::of(StaticPointerCast<T>(std::move(ptr).get()));
    }
    else {
        return std::move(ptr).transfer_error();
    }
}


template <typename T, typename TT>
Result<std::shared_ptr<T>> memoria_static_pointer_cast(Result<std::shared_ptr<TT>>&& ptr) noexcept
{
    if (MMA_LIKELY(ptr.is_ok()))
    {
        return Result<std::shared_ptr<T>>::of(std::static_pointer_cast<T>(std::move(ptr).get()));
    }
    else {
        return std::move(ptr).transfer_error();
    }
}

}
