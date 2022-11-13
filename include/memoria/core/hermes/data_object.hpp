
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
    friend class Object;

    template <typename, typename>
    friend class Map;

    template <typename>
    friend class Array;

protected:

    mutable HermesCtr* doc_;
    mutable ValueStorage value_storage_;

public:
    DataObject() noexcept:
        doc_(), value_storage_()
    {
        value_storage_.addr = nullptr;
        set_vs_tag(VS_TAG_ADDRESS);
    }

    DataObject(void* dt_ctr, HermesCtr* doc, ViewPtrHolder* ptr_holder) noexcept :
        Base(ptr_holder),
        doc_(doc)
    {
        value_storage_.addr = dt_ctr;
        set_vs_tag(VS_TAG_ADDRESS);
    }

    DataObject(ValueStorageTag vs_tag, ValueStorage& storage, HermesCtr* doc, ViewPtrHolder* ptr_holder) noexcept :
        Base(ptr_holder),
        doc_(doc),
        value_storage_(storage)
    {
        set_vs_tag(vs_tag);

        if (vs_tag == VS_TAG_GENERIC_VIEW) {
           value_storage_.view_ptr->ref_copy();
        }
    }

    DataObject(const TaggedValue& storage, HermesCtr* doc, ViewPtrHolder* ptr_holder) noexcept :
        Base(ptr_holder),
        doc_(doc)
    {
        value_storage_.small_value = storage;
        this->set_vs_tag(VS_TAG_SMALL_VALUE);
    }


    DataObject(DTTViewType<DT> view, HermesCtr* doc, ViewPtrHolder* ptr_holder) noexcept :
        Base(ptr_holder),
        doc_(doc)
    {
        if (TaggedValue::dt_fits_in<DT>())
        {
            this->set_vs_tag(VS_TAG_SMALL_VALUE);
            value_storage_.small_value = TaggedValue(ShortTypeCode::of<DT>(), view);
        }
        else {
            this->set_vs_tag(VS_TAG_GENERIC_VIEW);

            auto view_ptr = TaggedGenericView::allocate<DT>(view);
            value_storage_.view_ptr = view_ptr.get();
            view_ptr.release_holder();
        }
    }

    DataObject(const DataObject& other) noexcept :
        Base(other),
        doc_(other.doc_),
        value_storage_(other.value_storage_)
    {
        if (this->get_tag() == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            value_storage_.view_ptr->ref_copy();
        }
    }

    DataObject(DataObject&& other) noexcept :
        Base(std::move(other)),
        doc_(other.doc_),
        value_storage_(std::move(other.value_storage_))
    {
        if (this->get_tag() == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            other.value_storage_.view_ptr = nullptr;
        }
    }

    virtual ~DataObject() noexcept
    {
        if (this->get_tag() == ValueStorageTag::VS_TAG_GENERIC_VIEW)
        {
            if (value_storage_.view_ptr) {
                value_storage_.view_ptr->unref();
            }
        }
    }

    DataObject& operator=(const DataObject& other)
    {
        if (this->get_tag() == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            if (value_storage_.view_ptr) {
                value_storage_.view_ptr->unref();
            }
        }

        Base::operator=(other);
        value_storage_ = other.value_storage_;

        if (this->get_tag() == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            if (value_storage_.view_ptr) {
                value_storage_.view_ptr->ref_copy();
            }
        }

        return *this;
    }

    DataObject& operator=(DataObject&& other)
    {
        if (this->get_tag() == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            if (value_storage_.view_ptr) {
                value_storage_.view_ptr->unref();
            }
        }

        Base::operator=(std::move(other));
        value_storage_ = std::move(other.value_storage_);

        if (this->get_tag() == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            other.value_storage_.view_ptr = nullptr;
        }

        return *this;
    }

    bool is_detached() const {
        return get_vs_tag() != ValueStorageTag::VS_TAG_SMALL_VALUE;
    }



    PoolSharedPtr<HermesCtr> document() const {
        assert_not_null();
        return PoolSharedPtr<HermesCtr>(doc_, this->get_ptr_holder()->owner(), pool::DoRef{});
    }

    bool is_convertible_to_plain_string() const
    {
        if (!is_null()) {
            auto tag = get_type_tag();
            return get_type_reflection(tag).is_convertible_to_plain_string();
        }
        return false;
    }

    U8String to_plain_string() const
    {
        assert_not_null();
        auto tag = arena::read_type_tag(value_storage_.addr);
        return get_type_reflection(tag).convert_to_plain_string(get_vs_tag(), value_storage_, doc_, this->get_ptr_holder());
    }

    template <typename ToDT>
    bool is_convertible_to() const
    {
        if (!is_null())
        {
            auto src_tag = get_type_tag();
            auto to_tag = TypeHashV<ToDT>;
            return src_tag == to_tag || get_type_reflection(src_tag).is_convertible_to(to_tag);
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

    ObjectPtr as_object() const {
        return ObjectPtr(Object(get_vs_tag(), value_storage_, doc_, this->get_ptr_holder()));
    }

    DTTViewType<DT> view() const
    {
        assert_not_null();
        auto vs_tag = get_vs_tag();

        if (vs_tag == ValueStorageTag::VS_TAG_ADDRESS) {
            return dt_ctr()->view();
        }
        else {
            return value_storage_.get_view<DT>(vs_tag);
        }
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
        return to_string(StringifyCfg::pretty());
    }


    void stringify(std::ostream& out,
                   DumpFormatState& state) const
    {
        if (this->get_vs_tag() == ValueStorageTag::VS_TAG_ADDRESS) {
            dt_ctr()->stringify(out, state);
        }
        else {
            const auto& view = value_storage_.get_view<DT>(get_vs_tag());
            stringify_view(out, state, view);
        }
    }

    static void stringify_view(
            std::ostream& out,
            hermes::DumpFormatState& state,
            const DTTViewType<DT>& view
    ){
        ArenaDTContainer::stringify_view(out, state, view);
    }

    bool is_simple_layout() const {
        return true;
    }

    void* deep_copy_to(arena::ArenaAllocator& arena, DeepCopyDeduplicator& dedup) const {
        assert_not_null();
        return dt_ctr()->deep_copy_to(arena, ShortTypeCode::of<DataObject>(), doc_, this->get_ptr_holder(), dedup);
    }

    template <typename RightDT>
    bool is_compareble_with(const DataObjectPtr<RightDT>& other) const
    {
        if (is_not_null() && other->is_not_null())
        {
            auto tag1 = get_type_tag();
            auto tag2 = other->get_type_tag();
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
            auto tag1 = get_type_tag();
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
            auto tag1 = get_type_tag();
            auto tag2 = other->get_type_tag();
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
            auto tag1 = get_type_tag();
            return get_type_reflection(tag1).hermes_equals(
                    value_storage_.addr, doc_, this->get_ptr_holder(), other->value_storage_.addr, other->doc_, other->get_ptr_holder()
            );
        }
        else {
            return false;
        }
    }

private:
    ShortTypeCode get_type_tag() const noexcept
    {
        if (get_vs_tag() == ValueStorageTag::VS_TAG_ADDRESS) {
            return arena::read_type_tag(value_storage_.addr);
        }
        else if (get_vs_tag() == ValueStorageTag::VS_TAG_SMALL_VALUE) {
            return value_storage_.small_value.tag();
        }
        else if (get_vs_tag() == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            if (value_storage_.view_ptr) {
                return value_storage_.view_ptr->tag();
            }
        }

        return ShortTypeCode::nullv();
    }

    ValueStorageTag get_vs_tag() const {
        return static_cast<ValueStorageTag>(this->get_tag());
    }

    void set_vs_tag(ValueStorageTag tag) {
        this->set_tag(static_cast<size_t>(tag));
    }

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


class GenericDataObjectImpl: public GenericObject {
    mutable Object object_;
public:
    GenericDataObjectImpl(void* addr, HermesCtr* ctr, ViewPtrHolder* ptr_holder):
        object_(addr, ctr, ptr_holder)
    {
        object_.ref();
    }

    virtual ~GenericDataObjectImpl() noexcept {
        object_.unref();
    }

    virtual PoolSharedPtr<HermesCtr> ctr() const {
        return object_.document();
    }

    virtual bool is_array() const {
        return false;
    }
    virtual bool is_map() const {
        return false;
    }

    virtual PoolSharedPtr<GenericArray> as_array() const {
        MEMORIA_MAKE_GENERIC_ERROR("DataObject is not an Array").do_throw();
    }

    virtual PoolSharedPtr<GenericMap> as_map() const {
        MEMORIA_MAKE_GENERIC_ERROR("DataObject is not a Map").do_throw();
    }

    virtual ObjectPtr as_object() const {
        return ObjectPtr(object_);
    }
};





template <typename DT>
std::ostream& operator<<(std::ostream& out, DataObjectPtr<DT> ptr) {
    out << ptr->to_string();
    return out;
}

namespace detail {

template <typename DT>
struct ValueCastHelper<DataObject<DT>> {
    static ViewPtr<DataObject<DT>> cast_to(
            ValueStorageTag vs_tag,
            ValueStorage& storage, HermesCtr* doc, ViewPtrHolder* ref_holder
    ) noexcept {
        return ViewPtr<DataObject<DT>>(DataObject<DT>(
            vs_tag,
            storage,
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
        stringify_view(out, state, value_);
    }

    static void stringify_view(
            std::ostream& out,
            hermes::DumpFormatState& state,
            const DTTViewType<DT>& view
    ){
        out << view;

        if (std::is_same_v<DT, UBigInt>) {
            out << "ull";
        }
    }

    ArenaDataTypeContainer* deep_copy_to(
            ArenaAllocator& dst,
            ShortTypeCode tag,
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
        stringify_view(out, state, value_);
    }


    static void stringify_view(
            std::ostream& out,
            hermes::DumpFormatState& state,
            const bool& value
    ){
        if (value) {
            out << "true";
        }
        else {
            out << "false";
        }
    }

    ArenaDataTypeContainer* deep_copy_to(
            ArenaAllocator& dst,
            ShortTypeCode tag,
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
