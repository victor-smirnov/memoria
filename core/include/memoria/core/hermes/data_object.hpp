
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/memory/shared_ptr.hpp>
#include <memoria/core/arena/string.hpp>
#include <memoria/core/arena/hash_fn.hpp>

#include <memoria/core/hermes/object.hpp>
#include <memoria/core/hermes/common.hpp>
#include <memoria/core/hermes/traits.hpp>

#include <limits>

namespace memoria {


namespace hermes {
}

namespace arena {

namespace detail {

template <typename DT>
struct DTFNVFixedSizeHasherHelper;

template <>
struct DTFNVFixedSizeHasherHelper<BigInt> {
    static void hash_to(FNVHasher<8>& hasher, int64_t value) {
        for (size_t c = 0; c < 8; c++) {
            hasher.append((uint8_t)(value >> (c * 8)));
        }
    }
};


template <>
struct DTFNVFixedSizeHasherHelper<Double> {
    static void hash_to(FNVHasher<8>& hasher, double value) {
        uint64_t u64val = value_cast<uint64_t>(value);
        for (size_t c = 0; c < 8; c++) {
            hasher.append((uint8_t)(u64val >> (c * 8)));
        }
    }
};

template <typename DT>
struct StringifyViewHelper {
    static void stringify(std::ostream& out, const DTTViewType<DT>& view) {
        out << view;
    }
};

template <>
struct StringifyViewHelper<TinyInt> {
    static void stringify(std::ostream& out, const DTTViewType<TinyInt>& view) {
        out << (int)view << "_s8";
    }
};

template <>
struct StringifyViewHelper<UTinyInt> {
    static void stringify(std::ostream& out, const DTTViewType<UTinyInt>& view) {
        out << (unsigned)view << "_u8";
    }
};

template <>
struct StringifyViewHelper<SmallInt> {
    static void stringify(std::ostream& out, const DTTViewType<SmallInt>& view) {
        out << view << "_s16";
    }
};

template <>
struct StringifyViewHelper<USmallInt> {
    static void stringify(std::ostream& out, const DTTViewType<USmallInt>& view) {
        out << view << "_u16";
    }
};

template <>
struct StringifyViewHelper<Integer> {
    static void stringify(std::ostream& out, const DTTViewType<Integer>& view) {
        out << view;
    }
};

template <>
struct StringifyViewHelper<UInteger> {
    static void stringify(std::ostream& out, const DTTViewType<UInteger>& view) {
        out << view << "u";
    }
};

template <>
struct StringifyViewHelper<BigInt> {
    static void stringify(std::ostream& out, const DTTViewType<BigInt>& view) {
        out << view << "ll";
    }
};

template <>
struct StringifyViewHelper<UBigInt> {
    static void stringify(std::ostream& out, const DTTViewType<UBigInt>& view) {
        out << view << "ull";
    }
};

template <>
struct StringifyViewHelper<Boolean> {
    static void stringify(std::ostream& out, const DTTViewType<Boolean>& view) {
        out << (view ? "true" : "false");
    }
};

template <>
struct StringifyViewHelper<Double> {
    static void stringify(std::ostream& out, const DTTViewType<Double>& view)
    {
        std::ios_base::fmtflags f(out.flags());
        using DV = DTTViewType<Double>;
        using lims = std::numeric_limits<DV>;
        out.precision(lims::max_digits10);
        out << view << "d";
        out.flags(f);
    }
};

template <>
struct StringifyViewHelper<Real> {
    static void stringify(std::ostream& out, const DTTViewType<Real>& view)
    {
        std::ios_base::fmtflags f(out.flags());
        using DV = DTTViewType<Real>;
        using lims = std::numeric_limits<DV>;
        out.precision(lims::max_digits10);
        out << view;
        out.flags(f);
    }
};

}



template <typename DT>
class alignas(std::max<size_t>(2, alignof(DTTViewType<DT>))) ArenaDataTypeContainer<DT, FixedSizeDataTypeTag> {
    using ViewT = DTTViewType<DT>;
    using OViewT = DTView<DT>;

    ViewT value_;
public:
    ArenaDataTypeContainer(ViewT view):
        value_(view)
    {}

//    const ViewT& view() const noexcept {
//        return value_;
//    }

    OViewT view(LWMemHolder* ptr_holder) const noexcept {
        return OViewT(value_);
    }

    bool equals_to(const ViewT& view, LWMemHolder* mem_holder) const noexcept {
        return view(mem_holder) == view;
    }

    bool equals_to(const ArenaDataTypeContainer* other, LWMemHolder* mem_holder) const noexcept {
        return view(mem_holder) == other->view(mem_holder);
    }

    void hash_to(FNVHasher<8>& hasher) const noexcept {
        for (auto ch: view()) {
            hasher.append(ch);
        }
    }

    void stringify(std::ostream& out,
                   hermes::DumpFormatState& state, LWMemHolder* mem_holder)
    {
        stringify_view(out, state, value_);
    }

    static void stringify_view(
            std::ostream& out,
            hermes::DumpFormatState& state,
            const DTTViewType<DT>& view
    ){
        detail::StringifyViewHelper<DT>::stringify(out, view);
    }

    ArenaDataTypeContainer* deep_copy_to(
            ShortTypeCode tag,
            hermes::DeepCopyState& dedup) const
    {
        auto& dst = dedup.arena();
        ArenaDataTypeContainer* str = dedup.resolve(dst, this);
        if (MMA_LIKELY((bool)str)) {
            return str;
        }
        else {
            ArenaDataTypeContainer* new_str = dst.template allocate_tagged_object<ArenaDataTypeContainer>(tag, value_);
            dedup.map(dst, this, new_str);
            return new_str;
        }
    }

    void check(hermes::CheckStructureState& state, const char* src) const
    {

    }

    static void check(
        const arena::EmbeddingRelativePtr<void>& ptr,
        hermes::CheckStructureState& state,
        const char* src
    ) {

    }
};


template <>
class ArenaDataTypeContainer<Boolean, FixedSizeDataTypeTag> {
    using ViewT = DTTViewType<Boolean>;
    using OViewT = DTView<Boolean>;

    uint8_t value_;
public:
    ArenaDataTypeContainer(ViewT view):
        value_(view)
    {}

    OViewT view(LWMemHolder* mem_holder) const noexcept {
        return OViewT(value_);
    }

    bool equals_to(ViewT view, LWMemHolder*) const noexcept {
        return value_ == view;
    }

    bool equals_to(const ArenaDataTypeContainer* other, LWMemHolder*) const noexcept {
        return value_ == other->value_;
    }

    void hash_to(FNVHasher<8>& hasher) const noexcept {
        hasher.append(value_);
    }

    void stringify(std::ostream& out,
                   hermes::DumpFormatState& state, LWMemHolder*)
    {
        stringify_view(out, state, value_);
    }


    static void stringify_view(
            std::ostream& out,
            hermes::DumpFormatState& state,
            const bool& value
    ){
        detail::StringifyViewHelper<Boolean>::stringify(out, value);
    }

    void check(hermes::CheckStructureState& state, const char* src) const
    {
        if (value_ != 0 && value_ != 1) {
            MEMORIA_MAKE_GENERIC_ERROR("Boolean value is no 0 or 1: {} at {}", (uint32_t)value_, src).do_throw();
        }
    }

    static void check(
        const arena::EmbeddingRelativePtr<void>& ptr,
        hermes::CheckStructureState& state,
        const char* src
    ) {

    }


    ArenaDataTypeContainer* deep_copy_to(
            ShortTypeCode tag,
            hermes::DeepCopyState& dedup) const
    {
        auto& dst = dedup.arena();
        ArenaDataTypeContainer* str = dedup.resolve(dst, this);
        if (MMA_LIKELY((bool)str)) {
            return str;
        }
        else {
            ArenaDataTypeContainer* new_str = dst.template allocate_tagged_object<ArenaDataTypeContainer>(tag, value_);
            dedup.map(dst, this, new_str);
            return new_str;
        }
    }
};



}}
