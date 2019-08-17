
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

#include <memoria/v1/api/common/ctr_api_btss.hpp>

#include <memoria/v1/api/vector/vector_input.hpp>

#include <memoria/v1/core/tools/span.hpp>

#include <memoria/v1/core/types/typehash.hpp>
#include <memoria/v1/api/datatypes/traits.hpp>

#include <memoria/v1/api/vector/vector_api_factory.hpp>
#include <memoria/v1/api/vector/vector_producer.hpp>
#include <memoria/v1/api/vector/vector_scanner.hpp>

#include <memory>
#include <vector>

namespace memoria {
namespace v1 {

template <typename Value>
struct VectorIterator {
    using ValueV = typename DataTypeTraits<Value>::ValueType;

    virtual ~VectorIterator() noexcept {}

    virtual ValueV value() const = 0;
    virtual bool is_end() const = 0;
    virtual void next() = 0;
};

    
template <typename Value, typename Profile> 
struct ICtrApi<Vector<Value>, Profile>: public CtrReferenceable<Profile> {
    MMA1_DECLARE_ICTRAPI();
};



}}
