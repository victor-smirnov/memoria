
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

#include <absl/types/span.h>
#include <ostream>


namespace memoria {
namespace v1 {

template <typename T>
using Span = absl::Span<T>;

template <typename T>
std::ostream& operator<<(std::ostream& out, Span<const T> span)
{
    bool first_iter = true;

    out << "[";

    for (auto& vv: span)
    {
        if (!first_iter)
        {
            out << ", ";
        }
        else {
            first_iter = false;
        }

        out << vv;
    }

    out << "]";

    return out;
}


template <> struct TypeHash<Span<int8_t>>: UInt64Value<49> {};
template <> struct TypeHash<Span<const int8_t>>: UInt64Value<50> {};
template <> struct TypeHash<Span<uint8_t>>: UInt64Value<51> {};
template <> struct TypeHash<Span<const uint8_t>>: UInt64Value<52> {};

template <> struct TypeHash<Span<int16_t>>: UInt64Value<53> {};
template <> struct TypeHash<Span<const int16_t>>: UInt64Value<54> {};
template <> struct TypeHash<Span<uint16_t>>: UInt64Value<55> {};
template <> struct TypeHash<Span<const uint16_t>>: UInt64Value<56> {};

template <> struct TypeHash<Span<int32_t>>: UInt64Value<57> {};
template <> struct TypeHash<Span<const int32_t>>: UInt64Value<58> {};
template <> struct TypeHash<Span<uint32_t>>: UInt64Value<59> {};
template <> struct TypeHash<Span<const uint32_t>>: UInt64Value<60> {};

template <> struct TypeHash<Span<int64_t>>: UInt64Value<61> {};
template <> struct TypeHash<Span<const int64_t>>: UInt64Value<62> {};
template <> struct TypeHash<Span<uint64_t>>: UInt64Value<63> {};
template <> struct TypeHash<Span<const uint64_t>>: UInt64Value<64> {};

template <> struct TypeHash<Span<float>>: UInt64Value<65> {};
template <> struct TypeHash<Span<const float>>: UInt64Value<66> {};
template <> struct TypeHash<Span<double>>: UInt64Value<67> {};
template <> struct TypeHash<Span<const double>>: UInt64Value<68> {};

}}
