
// Copyright 2019 Victor Smirnov
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
#include <memoria/v1/api/datatypes/traits.hpp>

#include <typeinfo>

namespace memoria {
namespace v1 {

template <typename DataType, typename SelectorTag = typename DataTypeTraits<DataType>::DatumSelector>
class Datum;

class AnyDatum;

struct AnyDatumStorage
{
    virtual ~AnyDatumStorage() noexcept {}

    virtual U8String data_type_str() const = 0;
    virtual U8String to_sdn_string() const = 0;

    virtual const std::type_info& data_type_info() const noexcept = 0;
    virtual const char* data() const noexcept = 0;
    virtual void destroy() noexcept = 0;
    virtual bool equals(const AnyDatumStorage* other) const noexcept = 0;
};


}}
