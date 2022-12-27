
// Copyright 2016 Victor Smirnov
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

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/strings/format.hpp>

#include <memoria/core/tools/stream.hpp>
#include <memoria/core/tools/any_id.hpp>

#include <boost/uuid/uuid.hpp>

#include <iostream>
#include <type_traits>

namespace memoria {

class UUID;

std::ostream& operator<<(std::ostream& out, const UUID& uuid);
std::istream& operator>>(std::istream& in, UUID& uuid);

class UUID {
    uint64_t hi_;
    uint64_t lo_;

public:
    constexpr UUID() noexcept:
        hi_(), lo_()
    {}

    constexpr UUID(uint64_t hi, uint64_t lo) noexcept: hi_(hi), lo_(lo) {}

    const uint64_t& hi() const noexcept {
        return hi_;
    }

    uint64_t& hi() noexcept{
        return hi_;
    }

    const uint64_t& lo() const noexcept {
        return lo_;
    }

    uint64_t& lo() noexcept {
        return lo_;
    }

    bool is_null() const noexcept {
        return lo_ == 0 && hi_ == 0;
    }

    bool is_set() const noexcept {
        return lo_ != 0 || hi_ != 0;
    }

    bool operator==(const UUID& uuid) const noexcept {
        return hi_ == uuid.hi_ && lo_ == uuid.lo_;
    }

    bool operator!=(const UUID& uuid) const noexcept {
        return hi_ != uuid.hi_ || lo_ != uuid.lo_;
    }


    bool operator<(const UUID& other) const noexcept {
        return hi_ < other.hi_ || (hi_ == other.hi_ && lo_ < other.lo_);
    }

    bool operator<=(const UUID& other) const noexcept {
        return hi_ < other.hi_ || (hi_ == other.hi_ && lo_ <= other.lo_);
    }

    bool operator>(const UUID& other) const noexcept {
        return hi_ > other.hi_ || (hi_ == other.hi_ && lo_ > other.lo_);
    }

    bool operator>=(const UUID& other) const noexcept {
        return hi_ > other.hi_ || (hi_ == other.hi_ && lo_ >= other.lo_);
    }

    constexpr operator bool() const noexcept {
        return hi_ != 0 || lo_ != 0;
    }

    constexpr bool isSet() const noexcept {
        return hi_ != 0 || lo_ != 0;
    }

    void clear() noexcept {
        hi_ = lo_ = 0;
    }

    static UUID make_random();
    static UUID make_time();
    static UUID parse(const char* in);

    static constexpr UUID make(uint64_t hi, uint64_t lo) noexcept {
        return UUID(hi, lo);
    }

    U8String to_u8() const;
    U16String to_u16() const;

    std::string str() const {
        return to_u8().to_std_string();
    }

    uint64_t version() const noexcept {
        return (hi_ >> 52) & 0xF;
    }

    uint64_t variant() const noexcept {
        return (lo_ >> 4) & 0x7;
    }

    void set_version(uint64_t version) noexcept {
        hi_ &= 0xFF0FFFFFFFFFFFFF;
        hi_ |= (version & 0xfull) << 52;
    }

    void set_variant(uint64_t variant) noexcept {
        lo_ &= 0xFFFFFFFFFFFFFF1F;
        lo_ |= (variant & 0x7ull) << 5;
    }

    AnyID as_any_id() const;
};

//static inline uint64_t unpack_uint64_t(const UUID& uuid) noexcept {
//    uint64_t main_val = uuid.lo() & 0xFFFFFFFFFFFFFF1Full;
//    uint64_t extra_bits = uuid.hi() & 0xe0ull;
//    return main_val | extra_bits;
//}

//static inline UUID uuid_pack_uint64_t(uint64_t value) noexcept
//{
//    uint64_t lo = value & 0xFFFFFFFFFFFFFF1Full;
//    uint64_t hi = (value & 0xe0ull) | 0x00F0000000000000; // Non-standard Version F of the UUID.

//    return UUID{hi, lo};
//}


template <typename T>
struct TypeHash;

template <>
struct TypeHash<UUID>: UInt64Value<741231> {};

inline InputStreamHandler& operator>>(InputStreamHandler& in, UUID& value) {
    value.hi() = in.readUInt64();
    value.lo() = in.readUInt64();
    return in;
}

inline OutputStreamHandler& operator<<(OutputStreamHandler& out, const UUID& value) {
    out.write(value.hi());
    out.write(value.lo());
    return out;
}

template <typename T> struct FromString;

template <>
struct FromString<UUID> {
    static UUID convert(U16StringRef str) {
        return convert(str.to_u8().to_std_string());
    }

    static UUID convert(U8StringRef str)
    {
        return UUID::parse(str.data());
    }
};

template <>
struct DataTypeTraits<UUID>: SdnFixedSizeDataTypeTraits<UUID, UUID> {};


std::ostream& operator<<(std::ostream& out, const BlockIDValueHolder<UUID>& block_id_value) noexcept;

static inline OutputStreamHandler& operator<<(OutputStreamHandler& out, const CowBlockID<UUID>& value) {
    out.write(value.value().hi());
    out.write(value.value().lo());
    return out;
}


}


namespace std {

template <>
struct hash<memoria::UUID> {
    using argument_type	= memoria::UUID;
    using result_type = std::size_t;

    result_type operator()(const argument_type& uuid) const {
        std::hash<uint64_t> hf;
        return hf(uuid.hi()) ^ hf(uuid.lo());
    }
};

}

namespace fmt {

template <>
struct formatter<memoria::UUID> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::UUID& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.to_u8());
    }
};

}
