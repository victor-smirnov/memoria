
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/tools/stream.hpp>

#include <boost/uuid/uuid.hpp>

#include <iostream>


namespace memoria {
namespace v1 {

class UUID;

std::ostream& operator<<(std::ostream& out, const UUID& uuid);
std::istream& operator>>(std::istream& in, UUID& uuid);

class UUID {
    uint64_t hi_;
    uint64_t lo_;
public:
    constexpr UUID(): hi_(), lo_() {}

    constexpr UUID(uint64_t hi, uint64_t lo): hi_(hi), lo_(lo) {}

    const uint64_t& hi() const {
        return hi_;
    }

    uint64_t& hi() {
        return hi_;
    }

    const uint64_t& lo() const {
        return lo_;
    }

    uint64_t& lo() {
        return lo_;
    }

    bool is_null() const {
        return lo_ == 0 && hi_ == 0;
    }

    bool is_set() const {
        return lo_ != 0 || hi_ != 0;
    }

    bool operator==(const UUID& uuid) const {
        return hi_ == uuid.hi_ && lo_ == uuid.lo_;
    }

    bool operator!=(const UUID& uuid) const {
        return hi_ != uuid.hi_ || lo_ != uuid.lo_;
    }


    bool operator<(const UUID& other) const {
        return hi_ < other.hi_ || (hi_ == other.hi_ && lo_ < other.lo_);
    }

    bool operator<=(const UUID& other) const {
        return hi_ < other.hi_ || (hi_ == other.hi_ && lo_ <= other.lo_);
    }

    bool operator>(const UUID& other) const {
        return hi_ > other.hi_ || (hi_ == other.hi_ && lo_ > other.lo_);
    }

    bool operator>=(const UUID& other) const {
        return hi_ > other.hi_ || (hi_ == other.hi_ && lo_ >= other.lo_);
    }

    bool isSet() const {
        return hi_ != 0 || lo_ != 0;
    }

    void clear() {
        hi_ = lo_ = 0;
    }

    static UUID make_random();
    static UUID make_time();
    static UUID parse(const char* in);

    static constexpr UUID make(uint64_t hi, uint64_t lo) {
        return UUID(hi, lo);
    }

    U8String to_u8() const;

    std::string str() const {
        return to_u8().to_std_string();
    }
};



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


}}

namespace std {

template <>
struct hash<memoria::v1::UUID> {
    using argument_type	= memoria::v1::UUID;
    using result_type = std::size_t;

    result_type operator()(const argument_type& uuid) const {
        std::hash<uint64_t> hf;
        return hf(uuid.hi()) ^ hf(uuid.lo());
    }
};

}
