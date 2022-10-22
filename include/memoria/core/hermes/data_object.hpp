
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
namespace hermes {

class DocView;
class HermesDocImpl;

template <typename DT>
class DataObject: public HoldingView {
public:
    using ArenaDTContainer = arena::ArenaDataTypeContainer<DT>;

    friend class DocView;
    friend class Value;

    template <typename, typename>
    friend class Map;

    template <typename>
    friend class Array;

protected:
    mutable ArenaDTContainer* dt_ctr_;
    mutable DocView* doc_;
public:
    DataObject() noexcept:
        dt_ctr_(), doc_()
    {}

    DataObject(void* dt_ctr, DocView* doc, ViewPtrHolder* ptr_holder) noexcept :
        HoldingView(ptr_holder),
        dt_ctr_(reinterpret_cast<ArenaDTContainer*>(dt_ctr)),
        doc_(doc)
    {}

    PoolSharedPtr<DocView> document() const {
        assert_not_null();
        return PoolSharedPtr<DocView>(doc_, ptr_holder_->owner(), pool::DoRef{});
    }

    bool is_convertible_to_plain_string() const
    {
        if (!is_null()) {
            auto tag = arena::read_type_tag(dt_ctr_);
            return get_type_reflection(tag).is_convertible_to_plain_string();
        }
        return false;
    }

    U8String to_plain_string() const
    {
        assert_not_null();
        auto tag = arena::read_type_tag(dt_ctr_);
        return get_type_reflection(tag).convert_to_plain_string(dt_ctr_, doc_, ptr_holder_);
    }

    template <typename ToDT>
    bool is_convertible_to() const {
        if (!is_null()) {
            auto src_tag = arena::read_type_tag(dt_ctr_);
            auto to_tag = TypeHashV<ToDT>;
            return get_type_reflection(src_tag).is_convertible_from(to_tag);
        }
        return false;
    }

    template <typename ToDT>
    PoolSharedPtr<DocView> convert_to() const;

    uint64_t hash_code() const {
        assert_not_null();
        arena::DefaultHashFn<ArenaDTContainer> hash;
        return hash(dt_ctr_);
    }

    bool is_null() const noexcept {
        return dt_ctr_ == nullptr;
    }

    bool is_not_null() const noexcept {
        return dt_ctr_ != nullptr;
    }

    ValuePtr as_value() const {
        return ValuePtr(Value(dt_ctr_, doc_, ptr_holder_));
    }

    DTTViewType<DT> view() const
    {
        assert_not_null();
        return dt_ctr_->view();
    }

    U8String to_string() const
    {
        DumpFormatState fmt = DumpFormatState().simple();
        std::stringstream ss;
        stringify(ss, fmt);
        return ss.str();
    }

    U8String to_pretty_string() const
    {
        DumpFormatState fmt = DumpFormatState();
        std::stringstream ss;
        stringify(ss, fmt);
        return ss.str();
    }

    void stringify(std::ostream& out) const
    {
        DumpFormatState state;
        DumpState dump_state(*doc_);
        stringify(out, state, dump_state);
    }

    void stringify(std::ostream& out, DumpFormatState& format) const
    {
        DumpState dump_state(*doc_);
        stringify(out, format, dump_state);
    }



    void stringify(std::ostream& out,
                   DumpFormatState& state,
                   DumpState& dump_state) const
    {
        dt_ctr_->stringify(out, state, dump_state);
    }

    bool is_simple_layout() const {
        return true;
    }

    void* deep_copy_to(arena::ArenaAllocator& arena, DeepCopyDeduplicator& dedup) const {
        assert_not_null();
        return dt_ctr_->deep_copy_to(arena, TypeHashV<DataObject>, doc_, ptr_holder_, dedup);
    }

    template <typename RightDT>
    bool is_compareble_with(const DataObjectPtr<RightDT>& other) const
    {
        if (is_not_null() && other->is_not_null())
        {
            auto tag1 = arena::read_type_tag(dt_ctr_);
            auto tag2 = arena::read_type_tag(other->dt_ctr_);
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
            auto tag1 = arena::read_type_tag(dt_ctr_);
            return get_type_reflection(tag1).hermes_compare(
                    dt_ctr_, doc_, ptr_holder_, other->dt_ctr_, other->doc_, other->ptr_holder_
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
            auto tag1 = arena::read_type_tag(dt_ctr_);
            auto tag2 = arena::read_type_tag(other->dt_ctr_);
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
            auto tag1 = arena::read_type_tag(dt_ctr_);
            return get_type_reflection(tag1).hermes_equals(
                    dt_ctr_, doc_, ptr_holder_, other->dt_ctr_, other->doc_, other->ptr_holder_
            );
        }
        else {
            return false;
        }
    }



private:

    void assert_not_null() const
    {
        if (MMA_UNLIKELY(dt_ctr_ == nullptr)) {
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
    static ViewPtr<DataObject<DT>> cast_to(void* addr, DocView* doc, ViewPtrHolder* ref_holder) noexcept {
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
                   hermes::DumpFormatState& state,
                   hermes::DumpState& dump_state)
    {
        out << value_;

        if (std::is_same_v<DT, UBigInt>) {
            out << "ull";
        }
    }

    ArenaDataTypeContainer* deep_copy_to(
            ArenaAllocator& dst,
            ObjectTag tag,
            hermes::DocView*,
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
