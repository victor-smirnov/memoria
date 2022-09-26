
// Copyright 2021 Victor Smirnov
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
#include <memoria/core/tools/stream.hpp>
#include <memoria/core/tools/result.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/strings/format.hpp>

#include <memoria/core/tools/stream.hpp>
#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/any_id.hpp>

#include <memoria/core/datatypes/traits.hpp>

#include <iostream>
#include <type_traits>

namespace memoria {

class UID64;

std::ostream& operator<<(std::ostream& out, const UID64& uuid);
std::istream& operator>>(std::istream& in, UID64& uuid);

class UID64 {
public:
    using AtomT = uint64_t;

    static constexpr size_t NUM_ATOMS = 1;
    static constexpr size_t ATOM_BITSIZE = sizeof(AtomT) * 8;
    static constexpr size_t METADATA_BITSIZE = 9;

private:
    AtomT atom_;

public:
    constexpr UID64() noexcept:
        atom_(0)
    {}

    constexpr UID64(AtomT id, AtomT metadata) noexcept :
        atom_(id << METADATA_BITSIZE)
    {
        set_metadata(metadata);
    }

    constexpr AtomT atom(size_t) const noexcept {
        return atom_;
    }

    constexpr AtomT atom() const noexcept {
        return atom_;
    }


    Span<AtomT> atoms() noexcept {
        return Span<AtomT>(&atom_, NUM_ATOMS);
    }

    Span<const AtomT> atoms() const noexcept {
        return Span<const AtomT>(&atom_, NUM_ATOMS);
    }

    constexpr AtomT value() const noexcept {
        return atom_ >> METADATA_BITSIZE;
    }

    constexpr AtomT metadata() const noexcept
    {
        AtomT mask = 1;
        mask <<= METADATA_BITSIZE;
        mask -= 1;

        return atom_ & mask;
    }


    bool is_null() const noexcept {
        return value() == 0;
    }

    bool is_set() const noexcept {
        return value() != 0;
    }

    bool operator==(const UID64& uuid) const noexcept {
        return value() == uuid.value();
    }

    bool operator!=(const UID64& uuid) const noexcept {
        return value() != uuid.value();
    }


    bool operator<(const UID64& other) const noexcept {
        return value() < other.value();
    }

    bool operator<=(const UID64& other) const noexcept {
        return value() <= other.value();
    }

    bool operator>(const UID64& other) const noexcept {
        return value() > other.value();
    }

    bool operator>=(const UID64& other) const noexcept
    {
        return value() >= other.value();
    }

    constexpr operator bool() const noexcept {
        return value() != 0;
    }

    constexpr bool isSet() const noexcept {
        return value() != 0;
    }

    void clear() noexcept {
        atom_ = 0;
    }

    static UID64 parse(U8StringView view);

    template <typename... T>
    static constexpr UID64 make(AtomT a0, T&&... atoms) noexcept {
        return UID64(a0, static_cast<AtomT>(atoms)...);
    }

    U8String to_u8() const;
    U16String to_u16() const;

    std::string str() const {
        return to_u8().to_std_string();
    }

    constexpr void set_atom(AtomT atom) noexcept {
        atom_ = atom;
    }


    constexpr void set_metadata(AtomT meta) noexcept
    {
        AtomT mask = 1;
        mask <<= METADATA_BITSIZE;
        mask -= 1;

        atom_ &= ~mask;
        atom_ |= meta & mask;
    }


    constexpr void set_value(AtomT value) noexcept {
        atom_ |= value << METADATA_BITSIZE;
    }

    AnyID as_any_id() const;

    constexpr static AtomT wrap(AtomT value, AtomT metadata) noexcept
    {
        AtomT mask = 1;
        mask <<= METADATA_BITSIZE;
        mask -= 1;

        return (value << METADATA_BITSIZE) | (metadata & mask);
    }

    static AtomT unwrap_value(AtomT atom) noexcept {
        return atom >> METADATA_BITSIZE;
    }

    static AtomT wrap_metadata(AtomT atom) noexcept
    {
        AtomT mask = 1;
        mask <<= METADATA_BITSIZE;
        mask -= 1;

        return atom & mask;
    }
};


template <typename T>
struct TypeHash;

template <>
struct TypeHash<UID64>: UInt64Value<ToSmallHash(24383823489828374ull)> {};

inline InputStreamHandler& operator>>(InputStreamHandler& in, UID64& value)
{
    value.set_atom(in.readUInt64());
    return in;
}

inline OutputStreamHandler& operator<<(OutputStreamHandler& out, const UID64& value)
{
    out.write(value.atom());
    return out;
}

template <typename T> struct FromString;

template <>
struct FromString<UID64> {
    static UID64 convert(U16StringRef str) {
        return convert(str.to_u8().to_std_string());
    }

    static UID64 convert(U8StringRef str)
    {
        return UID64::parse(str.data());
    }
};

MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(UID64, UID64);

template <>
struct DataTypeTraits<UID64>: SdnFixedSizeDataTypeTraits<UID64, UID64> {};

std::ostream& operator<<(std::ostream& out, const BlockIDValueHolder<UID64>& block_id_value) noexcept;

static inline OutputStreamHandler& operator<<(OutputStreamHandler& out, const CowBlockID<UID64>& value)
{
    out.write(value.value().atom());
    return out;
}


}


namespace std {

template <>
struct hash<memoria::UID64> {
    using argument_type	= memoria::UID64;
    using result_type = std::size_t;

    result_type operator()(const argument_type& uuid) const
    {
        std::hash<uint64_t> hf;
        return hf(uuid.value());
    }
};

}

namespace fmt {

template <>
struct formatter<memoria::UID64> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::UID64& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.to_u8());
    }
};

}
