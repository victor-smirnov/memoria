
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/memory/object_pool.hpp>


#include <memoria/core/hermes/traits.hpp>
#include <memoria/core/hermes/common.hpp>



namespace memoria {
namespace hermes {

struct GenericMapEntry {
    virtual ~GenericMapEntry() noexcept = default;

    virtual Object key() const   = 0;
    virtual Object value() const = 0;

    virtual bool is_end() const = 0;
    virtual void next() = 0;
};

struct GenericMap: GenericObject {
    virtual uint64_t size() const = 0;
    virtual bool empty() const    = 0;

    virtual PoolSharedPtr<GenericMapEntry> iterator() const = 0;

    virtual Object get(U8StringView key) const = 0;
    virtual Object get(int32_t key) const = 0;
    virtual Object get(uint64_t key) const = 0;
    virtual Object get(uint8_t key) const = 0;

    virtual Object get(const Object& key) const = 0;

    MMA_NODISCARD virtual GenericMapPtr put(const Object& key, const Object& value) = 0;
    MMA_NODISCARD virtual GenericMapPtr remove(const Object& key) = 0;

    template <typename Fn>
    void for_each(Fn&& fn) const;
};

template <typename, typename>
class TypedGenericMap;


template <typename, typename>
class TypedGenericMapEntry;

}}
