
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

#include <memoria/v1/core/iovector/io_substream_array_fixed_size_base.hpp>
#include <memoria/v1/core/iovector/io_substream_array_vlen_base.hpp>



namespace memoria {
namespace v1 {
namespace io {

struct ColumnWise {};
struct ColumnWise1D {};
struct RowWise {};

//template <typename DataType, typename IOSubstreamTag> struct IOSubstreamApiTF;

template <typename Value, bool IsColumnWise, bool IsFixedSize>
struct IOSubstreamInterfaceTF;

template <typename Value>
struct IOSubstreamInterfaceTF<Value, true, true>: TypeDef<IOColumnwiseFixedSizeArraySubstream<Value>>
{};

template <typename Value>
struct IOSubstreamInterfaceTF<Value, false, true>: TypeDef<IORowwiseFixedSizeArraySubstream<Value>>
{};

template <typename Value>
struct IOSubstreamInterfaceTF<Value, true, false>: TypeDef<IOColumnwiseVLenArraySubstream<Value>>
{};

template <typename Value>
struct IOSubstreamInterfaceTF<Value, false, false>: TypeDef<IORowwiseVLenArraySubstream<Value>>
{};


}
}}
