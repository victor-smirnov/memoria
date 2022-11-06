
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

#include <memoria/core/hermes/value.hpp>
#include <memoria/core/hermes/common.hpp>
#include <memoria/core/hermes/traits.hpp>


namespace memoria {

template <typename DT>
class HoldingView<hermes::DataObject<DT>>: public hermes::TaggedHoldingView {

public:
    HoldingView(){}

    HoldingView(ViewPtrHolder* holder) noexcept:
        hermes::TaggedHoldingView(holder)
    {}
};


namespace hermes {

class HermesCtr;
class HermesCtrImpl;

template <typename DT>
class DataObject: public HoldingView<DataObject<DT>> {
    using Base = HoldingView<DataObject<DT>>;
public:
    using ArenaDTContainer = arena::ArenaDataTypeContainer<DT>;

    friend class HermesCtr;
    friend class Value;

    template <typename, typename>
    friend class Map;

    template <typename>
    friend class Array;

protected:

    static constexpr size_t TAG_ADDRESS     = 0;
    static constexpr size_t TAG_SMALL_VLAUE = 1;
    static constexpr size_t TAG_VIEW_PTR    = 2;

    //mutable ArenaDTContainer* dt_ctr_;
    mutable HermesCtr* doc_;
    mutable ValueStorage value_storage_;

    using Base::set_tag;
    using Base::get_tag;

public:
    DataObject() noexcept:
        doc_(), value_storage_()
    {
        value_storage_.addr = nullptr;
        set_tag(TAG_ADDRESS);
    }

    DataObject(void* dt_ctr, HermesCtr* doc, ViewPtrHolder* ptr_holder) noexcept :
        Base(ptr_holder),
        doc_(doc)
    {
        value_storage_.addr = dt_ctr;
        set_tag(TAG_ADDRESS);
    }

    PoolSharedPtr<HermesCtr> document() const {
        assert_not_null();
        return PoolSharedPtr<HermesCtr>(doc_, this->get_ptr_holder()->owner(), pool::DoRef{});
    }

    bool is_convertible_to_plain_string() const
    {
        if (!is_null()) {
            auto tag = arena::read_type_tag(value_storage_.addr);
            return get_type_reflection(tag).is_convertible_to_plain_string();
        }
        return false;
    }

    U8String to_plain_string() const
    {
        assert_not_null();
        auto tag = arena::read_type_tag(value_storage_.addr);
        return get_type_reflection(tag).convert_to_plain_string(value_storage_.addr, doc_, this->get_ptr_holder());
    }

    template <typename ToDT>
    bool is_convertible_to() const {
        if (!is_null()) {
            auto src_tag = arena::read_type_tag(value_storage_.addr);
            auto to_tag = TypeHashV<ToDT>;
            return get_type_reflection(src_tag).is_convertible_to(to_tag);
        }
        return false;
    }

    template <typename ToDT>
    DataObjectPtr<ToDT> convert_to() const;

    uint64_t hash_code() const {
        assert_not_null();
        arena::DefaultHashFn<ArenaDTContainer> hash;
        return hash(dt_ctr());
    }

    bool is_null() const noexcept {
        return value_storage_.addr == nullptr;
    }

    bool is_not_null() const noexcept {
        return value_storage_.addr != nullptr;
    }

    ValuePtr as_value() const {
        return ValuePtr(Value(value_storage_.addr, doc_, this->get_ptr_holder()));
    }

    DTTViewType<DT> view() const
    {
        assert_not_null();
        return dt_ctr()->view();
    }

    U8String to_string(const StringifyCfg& cfg) const
    {
        DumpFormatState fmt = DumpFormatState(cfg);
        std::stringstream ss;
        stringify(ss, fmt);
        return ss.str();
    }

    U8String to_pretty_string() const
    {
        to_string(StringifyCfg::pretty());
    }


    void stringify(std::ostream& out,
                   DumpFormatState& state) const
    {
        dt_ctr()->stringify(out, state);
    }

    bool is_simple_layout() const {
        return true;
    }

    void* deep_copy_to(arena::ArenaAllocator& arena, DeepCopyDeduplicator& dedup) const {
        assert_not_null();
        return dt_ctr()->deep_copy_to(arena, TypeHashV<DataObject>, doc_, this->get_ptr_holder(), dedup);
    }

    template <typename RightDT>
    bool is_compareble_with(const DataObjectPtr<RightDT>& other) const
    {
        if (is_not_null() && other->is_not_null())
        {
            auto tag1 = arena::read_type_tag(value_storage_.addr);
            auto tag2 = arena::read_type_tag(other->value_storage_.addr);
            return get_type_reflection(tag1).hermes_comparable_with(tag2);
        }
        else {
            return false;
        }
    }

    template <typename RightDT>
    int32_t compare(const DataObjectPtr<RightDT>& other) const
    {
        if (is_not_null() && other->is_not_null())
        {
            auto tag1 = arena::read_type_tag(value_storage_.addr);
            return get_type_reflection(tag1).hermes_compare(
                    value_storage_.addr, doc_, this->get_ptr_holder(), other->value_storage_.addr, other->doc_, other->get_ptr_holder()
            );
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Comparing operands may not be nullptr").do_throw();
        }
    }


    template <typename RightDT>
    bool is_equals_compareble_with(const DataObjectPtr<RightDT>& other) const
    {
        if (is_not_null() && other->is_not_null())
        {
            auto tag1 = arena::read_type_tag(value_storage_.addr);
            auto tag2 = arena::read_type_tag(other->value_storage_.addr);
            return get_type_reflection(tag1).hermes_equals_comparable_with(tag2);
        }
        else {
            return false;
        }
    }

    template <typename RightDT>
    bool equals(const DataObjectPtr<RightDT>& other) const
    {
        if (is_not_null() && other->is_not_null())
        {
            auto tag1 = arena::read_type_tag(value_storage_.addr);
            return get_type_reflection(tag1).hermes_equals(
                    value_storage_.addr, doc_, this->get_ptr_holder(), other->value_storage_.addr, other->doc_, other->get_ptr_holder()
            );
        }
        else {
            return false;
        }
    }

private:

    ArenaDTContainer* dt_ctr() const {
        return reinterpret_cast<ArenaDTContainer*>(value_storage_.addr);
    }

    void assert_not_null() const
    {
        if (MMA_UNLIKELY(value_storage_.addr == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("Datatype<Varchar> is null").do_throw();
        }
    }
};


template <typename DT>
std::ostream& operator<<(std::ostream& out, DataObjectPtr<DT> ptr) {
    out << ptr->to_string();
    return out;
}

namespace detail {

template <typename DT>
struct ValueCastHelper {
    static ViewPtr<DataObject<DT>> cast_to(void* addr, HermesCtr* doc, ViewPtrHolder* ref_holder) noexcept {
        return ViewPtr<DataObject<DT>>(DataObject<DT>(
            addr,
            doc,
            ref_holder
        ));
    }
};

}
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


}



template <typename DT>
class ArenaDataTypeContainer<DT, FixedSizeDataTypeTag> {
    using ViewT = DTTViewType<DT>;

    ViewT value_;
public:
    ArenaDataTypeContainer(ViewT view):
        value_(view)
    {}

    const ViewT& view() const noexcept {
        return value_;
    }

    bool equals_to(const ViewT& view) const noexcept {
        return view() == view;
    }

    bool equals_to(const ArenaDataTypeContainer* other) const noexcept {
        return view() == other->view();
    }

    void hash_to(FNVHasher<8>& hasher) const noexcept {
        for (auto ch: view()) {
            hasher.append(ch);
        }
    }

    void stringify(std::ostream& out,
                   hermes::DumpFormatState& state)
    {
        out << value_;

        if (std::is_same_v<DT, UBigInt>) {
            out << "ull";
        }
    }

    ArenaDataTypeContainer* deep_copy_to(
            ArenaAllocator& dst,
            ObjectTag tag,
            hermes::HermesCtr*,
            ViewPtrHolder*,
            DeepCopyDeduplicator& dedup) const
    {
        ArenaDataTypeContainer* str = dedup.resolve(dst, this);
        if (MMA_LIKELY((bool)str)) {
            return str;
        }
        else {
            ArenaDataTypeContainer* new_str = dst.template allocate_tagged_object<ArenaDataTypeContainer>(tag, view());
            dedup.map(dst, this, new_str);
            return new_str;
        }
    }
};


template <>
class ArenaDataTypeContainer<Boolean, FixedSizeDataTypeTag> {
    using ViewT = DTTViewType<Boolean>;

    uint8_t value_;
public:
    ArenaDataTypeContainer(ViewT view):
        value_(view)
    {}

    ViewT view() const noexcept {
        return value_;
    }

    bool equals_to(ViewT view) const noexcept {
        return this->view() == view;
    }

    bool equals_to(const ArenaDataTypeContainer* other) const noexcept {
        return this->view() == other->view();
    }

    void hash_to(FNVHasher<8>& hasher) const noexcept {
        hasher.append(value_);
    }

    void stringify(std::ostream& out,
                   hermes::DumpFormatState& state)
    {
        if (value_) {
            out << "true";
        }
        else {
            out << "false";
        }
    }

    ArenaDataTypeContainer* deep_copy_to(
            ArenaAllocator& dst,
            ObjectTag tag,
            hermes::HermesCtr*,
            ViewPtrHolder*,
            DeepCopyDeduplicator& dedup) const
    {
        ArenaDataTypeContainer* str = dedup.resolve(dst, this);
        if (MMA_LIKELY((bool)str)) {
            return str;
        }
        else {
            ArenaDataTypeContainer* new_str = dst.template allocate_tagged_object<ArenaDataTypeContainer>(tag, view());
            dedup.map(dst, this, new_str);
            return new_str;
        }
    }
};


}}
