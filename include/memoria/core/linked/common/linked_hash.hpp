
// Copyright 2019-2022 Victor Smirnov
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


#include <memoria/core/types.hpp>
#include <memoria/core/tools/span.hpp>


#include <boost/utility/string_view.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

namespace memoria {

class Variant1 {};
class Variant1a {};

template <size_t Size = 4, typename Variant = Variant1> class FNVHasher;

template <>
class FNVHasher<4, Variant1> {
    uint32_t hash_{0x811c9dc5ul};
public:
    FNVHasher() {}

    void append(uint8_t value)
    {
        hash_ = hash_ * 0x01000193;
        hash_ = hash_ ^ value;
    }

    uint32_t hash() const {
        return hash_;
    }
};

template <>
class FNVHasher<8, Variant1> {
    uint64_t hash_{0xcbf29ce484222325};
public:
    FNVHasher() {}

    void append(uint8_t value)
    {
        hash_ = hash_ * 0x00000100000001B3;
        hash_ = hash_ ^ value;
    }

    uint64_t hash() const {
        return hash_;
    }
};


template <size_t Size, typename Variant>
void append(FNVHasher<Size, Variant>& hasher, uint8_t value) {
    hasher.append(value);
}

template <size_t Size, typename Variant>
void append(FNVHasher<Size, Variant>& hasher, int8_t value) {
    append(hasher, (uint8_t)value);
}

template <size_t Size, typename Variant>
void append(FNVHasher<Size, Variant>& hasher, bool value) {
    append(hasher, (uint8_t)value);
}

template <size_t Size, typename Variant>
void append(FNVHasher<Size, Variant>& hasher, uint16_t value)
{
    hasher.append(value & 0xFF);
    hasher.append((value >> 8) & 0xFF);
}

template <size_t Size, typename Variant>
void append(FNVHasher<Size, Variant>& hasher, int16_t value) {
    append(hasher, (uint16_t)value);
}

template <size_t Size, typename Variant>
void append(FNVHasher<Size, Variant>& hasher, uint32_t value)
{
    hasher.append(value & 0xFF);
    hasher.append((value >> 8) & 0xFF);
    hasher.append((value >> 16) & 0xFF);
    hasher.append((value >> 24) & 0xFF);
}

template <size_t Size, typename Variant>
void append(FNVHasher<Size, Variant>& hasher, int32_t value) {
    append(hasher, (uint32_t)value);
}

template <size_t Size, typename Variant>
void append(FNVHasher<Size, Variant>& hasher, uint64_t value)
{
    hasher.append(value & 0xFF);
    hasher.append((value >> 8) & 0xFF);
    hasher.append((value >> 16) & 0xFF);
    hasher.append((value >> 24) & 0xFF);
    hasher.append((value >> 32) & 0xFF);
    hasher.append((value >> 40) & 0xFF);
    hasher.append((value >> 48) & 0xFF);
    hasher.append((value >> 56) & 0xFF);
}

template <size_t Size, typename Variant>
void append(FNVHasher<Size, Variant>& hasher, int64_t value) {
    append(hasher, (uint64_t)value);
}

template <size_t Size, typename Variant>
void append(FNVHasher<Size, Variant>& hasher, const void* ptr)
{
    uint64_t value = value_cast<uint64_t>(ptr);
    append(hasher, value);
}

template <size_t Size, typename Variant>
void append(FNVHasher<Size, Variant>& hasher, char value) {
    append(hasher, (uint8_t)value);
}

template <size_t Size, typename Variant>
void append(FNVHasher<Size, Variant>& hasher, char16_t value) {
    append(hasher, (uint16_t)value);
}

template <size_t Size, typename Variant>
void append(FNVHasher<Size, Variant>& hasher, char32_t value) {
    append(hasher, (uint32_t)value);
}

template <size_t Size, typename Variant>
void append(FNVHasher<Size, Variant>& hasher, float value)
{
    uint32_t uint_value = value_cast<uint32_t>(value);
    append(hasher, uint_value);
}

template <size_t Size, typename Variant>
void append(FNVHasher<Size, Variant>& hasher, double value)
{
    uint64_t uint_value = value_cast<uint64_t>(value);
    append(hasher, uint_value);
}


template <size_t Size, typename Variant, typename T>
void append(FNVHasher<Size, Variant>& hasher, Span<T> span)
{
    for (const auto& value: span) {
        append(hasher, value);
    }
}

template <size_t Size, typename Variant, typename T>
void append(FNVHasher<Size, Variant>& hasher, boost::basic_string_view<T> view)
{
    for (const auto& value: view) {
        append(hasher, value);
    }
}

}
