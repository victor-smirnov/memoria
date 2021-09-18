
// Copyright 2011-2021 Victor Smirnov
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
#include <memoria/core/strings/u8_string.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/tools/any_id.hpp>
#include <memoria/core/types/typehash.hpp>
#include <memoria/core/datatypes/traits.hpp>

namespace memoria {

class AnyID;

template <typename ValueHolder_>
class CowBlockID {
    ValueHolder_ holder_;
public:
    using ValueHolder = ValueHolder_;

    CowBlockID() noexcept = default;
    explicit CowBlockID(ValueHolder value) noexcept:
        holder_(value)
    {}

    ValueHolder& value() noexcept {
        return holder_;
    }

    const ValueHolder& value() const noexcept {
        return holder_;
    }

    void set_value(const ValueHolder& value) noexcept {
        holder_ = value;
    }

    bool operator==(const CowBlockID& other) const noexcept {
        return holder_ == other.holder_;
    }

    bool operator!=(const CowBlockID& other) const noexcept {
        return holder_ != other.holder_;
    }

    bool operator<(const CowBlockID& other) const noexcept {
        return holder_ < other.holder_;
    }

    bool isSet() const noexcept {
        return holder_;
    }

    bool is_set() const noexcept {
        return holder_;
    }

    bool is_null() const noexcept {
        return !isSet();
    }

    operator bool() const noexcept {
        return (bool)holder_;
    }

    U8String to_u8() const {
        return U8String("COW<") + holder_.to_u8() + ">";
    }

    AnyID as_any_id() const {
        return AnyID{std::make_unique<DefaultAnyIDImpl<CowBlockID>>(*this)};
    }
};

template <typename VH>
std::ostream& operator<<(std::ostream& out, const CowBlockID<VH>& block_id) noexcept {
    out << block_id.value();
    return out;
}

template <typename ValueHolder>
struct TypeHash<CowBlockID<ValueHolder>>: UInt64Value <
    HashHelper<345630986034956, TypeHashV<ValueHolder>>
> {};


class UID64;
class UID256;

template<>
struct DataTypeTraits<CowBlockID<UID64>>: FixedSizeDataTypeTraits<CowBlockID<UID64>, UID64CowBlockID>
{
    static void create_signature(SBuf& buf, const CowBlockID<UID64>& obj) {
        buf << "UID64CowBlockID";
    }

    static void create_signature(SBuf& buf) {
        buf << "UID64CowBlockID";
    }
};



template <>
struct DataTypeTraits<CowBlockID<UID256>>: FixedSizeDataTypeTraits<CowBlockID<UID256>, UID256CowBlockID>
{
    static void create_signature(SBuf& buf, const CowBlockID<UID256>& obj) {
        buf << "UID256CowBlockID";
    }

    static void create_signature(SBuf& buf) {
        buf << "UID256CowBlockID";
    }
};


}

namespace std {

template <typename ValueHolder>
class hash<memoria::CowBlockID<ValueHolder>> {
public:
    size_t operator()(const memoria::CowBlockID<ValueHolder>& obj) const noexcept {
        return std::hash<ValueHolder>()(obj.value());
    }
};

template <typename ValueHolder>
void swap(memoria::CowBlockID<ValueHolder>& one, memoria::CowBlockID<ValueHolder>& two) noexcept {
    std::swap(one.value(), two.value());
}

}

namespace fmt {

template <typename ValueHolder>
struct formatter<memoria::CowBlockID<ValueHolder>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::CowBlockID<ValueHolder>& d, FormatContext& ctx)
    {
        std::stringstream ss;
        ss << d;
        return format_to(ctx.out(), "{}", ss.str());
    }
};

}
