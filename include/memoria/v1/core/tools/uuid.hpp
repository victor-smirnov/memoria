
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/stream.hpp>

#include <iostream>


namespace memoria {
namespace v1 {

class UUID;

std::ostream& operator<<(std::ostream& out, const UUID& uuid);
std::istream& operator>>(std::istream& in, UUID& uuid);

class UUID {
    UBigInt hi_;
    UBigInt lo_;
public:
    constexpr UUID(): hi_(), lo_() {}

    constexpr UUID(UBigInt hi, UBigInt lo): hi_(hi), lo_(lo) {}

    const UBigInt& hi() const {
        return hi_;
    }

    UBigInt& hi() {
        return hi_;
    }

    const UBigInt& lo() const {
        return lo_;
    }

    UBigInt& lo() {
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

    static constexpr UUID make(UBigInt hi, UBigInt lo) {
        return UUID(hi, lo);
    }
};



template <typename T>
struct TypeHash;

template <>
struct TypeHash<UUID>: UIntValue<741231> {};

struct UUIDKeyHash
{
    long operator() (const UUID &k) const { return k.hi() ^ k.lo(); }
};

struct UUIDKeyEq
{
    bool operator() (const UUID &x, const UUID &y) const { return x == y; }
};


inline InputStreamHandler& operator>>(InputStreamHandler& in, UUID& value) {
	value.hi() = in.readUBigInt();
	value.lo() = in.readUBigInt();
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
    static UUID convert(StringRef str)
    {
        return UUID::parse(str.c_str());
    }
};








}}
