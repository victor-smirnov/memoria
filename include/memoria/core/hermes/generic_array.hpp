
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

struct GenericArray: GenericObject {
    virtual uint64_t size() const = 0;
    virtual ObjectPtr get(uint64_t idx) const = 0;

    virtual void set(uint64_t idx, const ObjectPtr& value) = 0;

    virtual void push_back(const ObjectPtr& value)    = 0;
    virtual void remove(uint64_t start, uint64_t end) = 0;
};

template <typename>
class TypedGenericArray;

}}
