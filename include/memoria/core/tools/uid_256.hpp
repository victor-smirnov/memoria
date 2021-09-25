
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

#include <boost/uuid/uuid.hpp>

#include <iostream>
#include <type_traits>

namespace memoria {

class UID256;

std::ostream& operator<<(std::ostream& out, const UID256& uuid);
std::istream& operator>>(std::istream& in, UID256& uuid);

class UID256 {
public:
    using AtomT = uint64_t;
    enum class Type: AtomT {
        TYPE0, // Empty UID256
        TYPE1, // Fully random (248 bit)
        TYPE2, // 176 bit random, 8 bit metadata payload, 64 bit counter.
        TYPE3, // 168 bit random, 16 bit metadata payload, 64 bit counter.
    };

    static constexpr size_t NUM_ATOMS = 4;
    static constexpr size_t ATOM_BITSIZE = sizeof(AtomT) * 8;
    static constexpr size_t TYPE_BITSIZE = 8;

private:
    AtomT atoms_[NUM_ATOMS];

public:
    constexpr UID256() noexcept:
        atoms_{0,}
    {}

    template <typename... T>
    constexpr UID256(AtomT a0, T&&... vals)noexcept :
        atoms_{a0, static_cast<AtomT>(vals)...}
    {}

    UID256(Span<const AtomT> span) noexcept :
        atoms_{0,}
    {
        for (size_t c = 0; c < std::min(NUM_ATOMS, span.size()); c++) {
            atoms_[c] = span[c];
        }
    }

    AtomT top_atom() const noexcept {
        return atoms_[NUM_ATOMS - 1];
    }

    Type type() const noexcept {
        return static_cast<Type>(top_atom() >> (ATOM_BITSIZE - TYPE_BITSIZE));
    }

    AtomT atom(size_t c) const noexcept {
        return atoms_[c];
    }

    Span<AtomT> atoms() noexcept {
        return Span<AtomT>(atoms_, NUM_ATOMS);
    }

    Span<const AtomT> atoms() const noexcept {
        return Span<const AtomT>(atoms_, NUM_ATOMS);
    }

    AtomT counter() const noexcept {
        return atoms_[0];
    }

    AtomT metadata() const noexcept
    {
        Type tt = type();
        if (tt == Type::TYPE2) {
            return atoms_[1] & 0xFF;
        }
        else if (tt == Type::TYPE3) {
            return atoms_[1] & 0xFFFF;
        }
        return 0;
    }

    bool is_type0() const noexcept {
        return type() == Type::TYPE0;
    }

    bool is_type1() const noexcept {
        return type() == Type::TYPE1;
    }

    bool is_type2() const noexcept {
        return type() == Type::TYPE2;
    }

    bool is_type3() const noexcept {
        return type() == Type::TYPE3;
    }

    size_t ctz() const noexcept
    {
        size_t cnt{};
        for (size_t c = 0; c < NUM_ATOMS; c++) {
            cnt += atoms_[c] == 0;
        }
        return cnt;
    }

    size_t cte(const UID256& other) const noexcept
    {
        size_t cnt{};
        for (size_t c = 0; c < NUM_ATOMS; c++) {
            cnt += atoms_[c] == other.atoms_[c];
        }
        return cnt;
    }

    bool is_null() const noexcept {
        return ctz() == NUM_ATOMS;
    }

    bool is_set() const noexcept {
        return ctz() != NUM_ATOMS;
    }

    bool operator==(const UID256& uuid) const noexcept {
        return cte(uuid) == NUM_ATOMS;
    }

    bool operator!=(const UID256& uuid) const noexcept {
        return cte(uuid) < NUM_ATOMS;
    }


    bool operator<(const UID256& other) const noexcept
    {
        for (size_t c = 1; c <= NUM_ATOMS; c++)
        {
            AtomT my_atom = atoms_[NUM_ATOMS - c];
            AtomT other_atom = other.atoms_[NUM_ATOMS - c];

            if (my_atom < other_atom) {
                return true;
            }
            else if (my_atom > other_atom) {
                return false;
            }
        }

        return false;
    }

    bool operator<=(const UID256& other) const noexcept
    {
        for (size_t c = 1; c <= NUM_ATOMS; c++)
        {
            AtomT my_atom = atoms_[NUM_ATOMS - c];
            AtomT other_atom = other.atoms_[NUM_ATOMS - c];

            if (my_atom < other_atom) {
                return true;
            }
            else if (my_atom > other_atom) {
                return false;
            }
        }

        return true;
    }

    bool operator>(const UID256& other) const noexcept
    {
        for (size_t c = 1; c <= NUM_ATOMS; c++)
        {
            AtomT my_atom = atoms_[NUM_ATOMS - c];
            AtomT other_atom = other.atoms_[NUM_ATOMS - c];

            if (my_atom > other_atom) {
                return true;
            }
            else if (my_atom < other_atom) {
                return false;
            }
        }

        return false;
    }

    bool operator>=(const UID256& other) const noexcept
    {
        for (size_t c = 1; c <= NUM_ATOMS; c++)
        {
            AtomT my_atom = atoms_[NUM_ATOMS - c];
            AtomT other_atom = other.atoms_[NUM_ATOMS - c];

            if (my_atom > other_atom) {
                return true;
            }
            else if (my_atom < other_atom) {
                return false;
            }
        }

        return true;
    }

    constexpr operator bool() const noexcept {
        return ctz() != NUM_ATOMS;
    }

    constexpr bool isSet() const noexcept {
        return ctz() != NUM_ATOMS;
    }

    void clear() noexcept {
        for (uint64_t& atom: atoms()) {
            atom = 0;
        }
    }

    static UID256 parse(U8StringView view);
    static UID256 make_random();

    static UID256 make_type2(const UID256& parent, AtomT meta, AtomT value)
    {
        UID256 uuid = parent;

        uuid.set_type(Type::TYPE2);
        uuid.set_metadata2(meta);
        uuid.atoms_[0] = value;

        return uuid;
    }

    static UID256 make_type3(const UID256& parent, AtomT meta, AtomT value)
    {
        UID256 uuid = parent;

        uuid.set_type(Type::TYPE3);
        uuid.set_metadata3(meta);
        uuid.atoms_[0] = value;

        return uuid;
    }

    static UID256 make_type2(AtomT meta, AtomT value);
    static UID256 make_type3(AtomT meta, AtomT value);

    template <typename... T>
    static constexpr UID256 make(AtomT a0, T&&... atoms) noexcept {
        return UID256(a0, static_cast<AtomT>(atoms)...);
    }

    U8String to_u8() const;
    U16String to_u16() const;
    U8String to_cxx_decl() const;

    std::string str() const {
        return to_u8().to_std_string();
    }

    void set_atom(size_t idx, AtomT atom) noexcept {
        atoms_[idx] = atom;
    }

    void set_type(Type type) noexcept
    {
        AtomT mask = 1;
        mask <<= TYPE_BITSIZE;
        mask -= 1;
        mask <<= (ATOM_BITSIZE - TYPE_BITSIZE);

        atoms_[NUM_ATOMS - 1] &= ~mask;
        atoms_[NUM_ATOMS - 1] |= (static_cast<AtomT>(type) << (ATOM_BITSIZE - TYPE_BITSIZE));
    }

    void set_metadata2(AtomT meta) noexcept
    {
        AtomT mask = 0xFF;

        atoms_[1] &= ~mask;
        atoms_[1] |= meta & mask;
    }

    void set_metadata3(AtomT meta) noexcept
    {
        AtomT mask = 0xFFFF;

        atoms_[1] &= ~mask;
        atoms_[1] |= meta & mask;
    }

    void set_payload_4bit(size_t idx, AtomT code)
    {
        size_t start;
        size_t limit = ATOM_BITSIZE * NUM_ATOMS - TYPE_BITSIZE;

        Type tt = type();
        if (tt == Type::TYPE1) {
            start = 0;
        }
        else if (tt == Type::TYPE2) {
            start = 64 + 8;
        }
        else if (tt == Type::TYPE3) {
            start = 64 + 16;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid UID256 type: {}", static_cast<uint64_t>(tt)).do_throw();
        }

        limit -= start;

        if (idx * 4 <= limit - 4) {
            SetBits(atoms_, start + idx * 4, code, 4);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid UID256 payload quartet address: {}, limit: {}", idx, (limit - 4) >> 2).do_throw();
        }
    }

    AtomT get_payload_4bit(size_t idx) const
    {
        size_t start;
        size_t limit = ATOM_BITSIZE * NUM_ATOMS - TYPE_BITSIZE;

        Type tt = type();
        if (tt == Type::TYPE1) {
            start = 0;
        }
        else if (tt == Type::TYPE2) {
            start = 64 + 8;
        }
        else if (tt == Type::TYPE3) {
            start = 64 + 16;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid UID256 type: {}", static_cast<uint64_t>(tt)).do_throw();
        }

        limit -= start;

        if (idx * 4 <= limit - 4) {
            return GetBits(atoms_, start + idx * 4, 4);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid UID256 payload quartet address: {}, limit: {}", idx, (limit - 4) >> 2).do_throw();
        }
    }

    size_t payload_length() const noexcept
    {
        Type tt = type();
        size_t limit = ATOM_BITSIZE * NUM_ATOMS - TYPE_BITSIZE;

        if (tt == Type::TYPE2) {
            limit -= 64 + 8;
        }
        else if (tt == Type::TYPE3) {
            limit -= 64 + 16;
        }

        return limit >> 2;
    }

    AnyID as_any_id() const;
};


template <typename T>
struct TypeHash;

template <>
struct TypeHash<UID256>: UInt64Value<47385785683427ull> {};

inline InputStreamHandler& operator>>(InputStreamHandler& in, UID256& value)
{
    for (size_t c = 0; c < UID256::NUM_ATOMS; c++) {
        value.set_atom(c, in.readUInt64());
    }
    return in;
}

inline OutputStreamHandler& operator<<(OutputStreamHandler& out, const UID256& value)
{
    for (const auto& atom: value.atoms()) {
        out.write(atom);
    }
    return out;
}

template <typename T> struct FromString;

template <>
struct FromString<UID256> {
    static UID256 convert(U16StringRef str) {
        return convert(str.to_u8().to_std_string());
    }

    static UID256 convert(U8StringRef str)
    {
        return UID256::parse(str.data());
    }
};

MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(UID256, UID256);

template <>
struct DataTypeTraits<UID256>: SdnFixedSizeDataTypeTraits<UID256, UID256> {};

std::ostream& operator<<(std::ostream& out, const BlockIDValueHolder<UID256>& block_id_value) noexcept;

static inline OutputStreamHandler& operator<<(OutputStreamHandler& out, const CowBlockID<UID256>& value)
{
    for (const auto& atom: value.value().atoms()) {
        out.write(atom);
    }
    return out;
}


}


namespace std {

template <>
struct hash<memoria::UID256> {
    using argument_type	= memoria::UID256;
    using result_type = std::size_t;

    result_type operator()(const argument_type& uuid) const
    {
        uint64_t sum{};
        std::hash<uint64_t> hf;

        for (auto atom: uuid.atoms()) {
            sum ^= hf(atom);
        }

        return sum;
    }
};

}

namespace fmt {

template <>
struct formatter<memoria::UID256> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::UID256& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.to_u8());
    }
};

}
