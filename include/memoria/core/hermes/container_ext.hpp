
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


#include <memoria/core/hermes/container.hpp>
#include <memoria/core/hermes/map/map.hpp>
#include <memoria/core/hermes/map/typed_map.hpp>
#include <memoria/core/hermes/array/array.hpp>
#include <memoria/core/hermes/array/typed_array.hpp>
#include <memoria/core/hermes/data_object.hpp>

#include <memoria/core/hermes/object.hpp>

namespace memoria {
namespace hermes {

class StaticHermesCtrImpl: public HermesCtr, public pool::enable_shared_from_this<StaticHermesCtrImpl> {
protected:
    LWMemHolder view_mem_holder_;
    mutable uint8_t* segment_;
    size_t capacity_;

    friend class HermesCtr;

public:
    StaticHermesCtrImpl(uint8_t* segment, size_t capacity) noexcept:
        segment_(segment), capacity_(capacity)
    {
        mem_holder_ = &view_mem_holder_;
        segment_size_ = 0;
    }

protected:
    uint8_t* memory_buffer() const {
        return segment_;
    }

    size_t capacity() const {
        return capacity_;
    }

    void init_from(const arena::ArenaAllocator& arena)
    {
        auto arena_size = arena.head().size;

        if (arena_size <= capacity_)
        {
            std::memcpy(segment_, arena.head().memory.get(), arena_size);
            header_ = ptr_cast<DocumentHeader>(segment_);
            segment_size_ = arena_size;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                "Supplied ArenaAllocator is too big. Expected: {}, actual {} bytes",
                        capacity_, arena_size
            ).do_throw();
        }
    }

    void configure_refholder(SharedPtrHolder* ref_holder) {
        view_mem_holder_.set_owner(ref_holder);
    }
};



template <size_t Size>
class SizedHermesCtrImpl: public StaticHermesCtrImpl {

    uint8_t buffer_[Size];
    friend class HermesCtr;

public:
    SizedHermesCtrImpl() noexcept:
        StaticHermesCtrImpl(buffer_, Size)
    {
    }
};



inline void HermesCtr::assert_mutable()
{
    if (MMA_UNLIKELY(!this->is_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("HermesCtr is immutable").do_throw();
    }
}

template <typename DT>
DataObject<DT> HermesCtr::new_dataobject(DTTViewType<DT> view)
{
    using DTCtr = DataObjectView<DT>;

    auto arena_dtc = arena_->allocate_tagged_object<typename DTCtr::ArenaDTContainer>(
        ShortTypeCode::of<DTCtr>(),
        view
    );

    return DataObject<DT>(DTCtr(mem_holder_, arena_dtc));
}


template <typename DT>
DataObject<DT> HermesCtr::wrap_primitive(DTTViewType<DT> view)
{
    return wrap_primitive<DT>(view, common_instance().get());
}


template <typename DT>
DataObject<DT> HermesCtr::wrap_primitive(DTTViewType<DT> view, HermesCtr* ctr)
{
    static_assert(
        TaggedValue::dt_fits_in<DT>(),
        ""
    );

    TaggedValue storage(ShortTypeCode::of<DT>(), view);
    return DataObject<DT>(DataObjectView<DT>(ctr->mem_holder_, storage));
}


template <typename DT>
DataObject<DT> HermesCtr::wrap_dataobject__full(DTTViewType<DT> view)
{
    using DataObjectT = DataObjectView<DT>;
    using ContainerT = typename DataObjectT::ArenaDTContainer;

    auto& arena = arena::get_local_instance();
    arena.object_pool_init_state();

    arena::Cleaner cleaner(arena);

    auto header  = arena.allocate_object_untagged<DocumentHeader>();
    auto tag     = ShortTypeCode::of<DataObjectT>();
    header->root = arena.allocate_tagged_object<ContainerT>(tag, view);

    if (arena.chunks() == 1)
    {
        auto instance = get_ctr_instance(arena.head().size);
        if (!instance.is_null())
        {
            instance->init_from(arena);
            return instance->root()->as_data_object<DT>();
        }
    }

    auto instance = make_pooled();
    instance->init_from(arena);
    return instance->root()->as_data_object<DT>();
}

namespace detail {

template <typename DT>
struct DTSizeDispatcher<DT, true> {
    static auto dispatch(const DTTViewType<DT>& view) {
        return HermesCtr::wrap_primitive<DT>(view);
    }

    static auto dispatch(const DTTViewType<DT>& view, HermesCtr* owner) {
        return HermesCtr::wrap_primitive<DT>(view, owner);
    }
};

template <typename DT>
struct DTSizeDispatcher<DT, false> {
    static auto dispatch(const DTTViewType<DT>& view) {
        return HermesCtr::wrap_dataobject__full<DT>(view);
    }

    static auto dispatch(const DTTViewType<DT>& view, HermesCtr* owner) {
        return owner->new_dataobject<DT>(view);
    }
};

}


template <typename DT>
DataObject<DT> HermesCtr::new_embeddable_dataobject(DTTViewType<DT> view)
{
    return detail::DTSizeDispatcher<
            DT,
            arena::ERelativePtr::dt_fits_in<DT>()
    >::dispatch(view, this);
}


template <typename DT>
DataObject<DT> HermesCtr::wrap_dataobject(DTTViewType<DT> view)
{
    return detail::DTSizeDispatcher<
        DT, TaggedValue::dt_fits_in<DT>()
    >::dispatch(view);
}

inline GenericArrayPtr ObjectView::as_generic_array() const
{
    if (get_vs_tag() == VS_TAG_ADDRESS)
    {
        auto tag = get_type_tag();
        auto ctr_ptr = get_type_reflection(tag).hermes_make_wrapper(get_mem_holder(), storage_.addr);
        return ctr_ptr->as_array();
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Object is not addressable").do_throw();
    }
}


inline GenericMapPtr ObjectView::as_generic_map() const
{
    if (get_vs_tag() == VS_TAG_ADDRESS)
    {
        auto tag = get_type_tag();
        auto ctr_ptr = get_type_reflection(tag).hermes_make_wrapper(get_mem_holder(), storage_.addr);
        return ctr_ptr->as_map();
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Object is not addressable").do_throw();
    }
}

inline Datatype ObjectView::as_datatype() const {
    return cast_to<DatatypeView>();
}

inline DataObject<Varchar> ObjectView::as_varchar() const {
    return cast_to<DataObjectView<Varchar>>();
}

inline DataObject<Double> ObjectView::as_double() const {
    return cast_to<DataObjectView<Double>>();
}

inline DataObject<BigInt> ObjectView::as_bigint() const {
    return cast_to<DataObjectView<BigInt>>();
}

inline DataObject<Boolean> ObjectView::as_boolean() const {
    return cast_to<DataObjectView<Boolean>>();
}

inline DataObject<Real> ObjectView::as_real() const {
    return cast_to<DataObjectView<Real>>();
}

inline int64_t ObjectView::to_i64() const {
    return *convert_to<BigInt>()->as_data_object<BigInt>()->view();
}

inline int64_t ObjectView::to_i32() const {
    return *convert_to<Integer>()->as_data_object<Integer>()->view();
}

inline U8String ObjectView::to_str() const {
    return *convert_to<Varchar>()->as_data_object<Varchar>()->view();
}

inline bool ObjectView::to_bool() const {
    return *convert_to<Boolean>()->as_data_object<Boolean>()->view();
}

inline double ObjectView::to_d64() const {
    return *convert_to<Double>()->as_data_object<Double>()->view();
}

inline float ObjectView::to_f32() const {
    return *convert_to<Real>()->as_data_object<Real>()->view();
}


inline U8String ObjectView::type_str() const {
    assert_not_null();
    auto tag = get_type_tag();
    return get_type_reflection(tag).str();
}


template <typename DT>
Object ObjectView::convert_to() const
{
    assert_not_null();
    auto src_tag = get_type_tag();
    auto to_tag = ShortTypeCode::of<DT>();
    if (src_tag != to_tag) {
        return get_type_reflection(src_tag).datatype_convert_to(get_mem_holder(), to_tag, get_vs_tag(), storage_);
    }
    else {
        return this->as_object();
    }
}

template <typename DT>
template <typename ToDT>
DataObject<ToDT> DataObjectView<DT>::convert_to() const
{
    assert_not_null();
    auto src_tag = get_type_tag();
    auto to_tag = ShortTypeCode::of<ToDT>();
    return get_type_reflection(src_tag).datatype_convert_to(this->get_mem_holder(), to_tag, get_vs_tag(), storage_);
}

inline PoolSharedPtr<HermesCtr> CtrAware::ctr() const {
    if (MMA_LIKELY((bool)ctr_)) {
        return ctr_->self();
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Container is not specified for this Object").do_throw();
    }
}

template <typename DT>
Array<DT> HermesCtr::new_typed_array() {
    assert_not_null();
    assert_mutable();

    using CtrT = ArrayView<DT>;

    auto arena_dtc = arena()->allocate_tagged_object<typename CtrT::ArrayStorageT>(
        ShortTypeCode::of<CtrT>()
    );

    return Array<DT>(mem_holder_, arena_dtc);
}

template <typename KeyDT, typename ValueDT>
Map<KeyDT, ValueDT> HermesCtr::new_typed_map() {
    assert_not_null();
    assert_mutable();

    using CtrT = Map<KeyDT, ValueDT>;

    auto arena_dtc = arena()->allocate_tagged_object<typename CtrT::MapStorageT>(
        ShortTypeCode::of<CtrT>()
    );

    return Map<KeyDT, ValueDT>(mem_holder_, arena_dtc);
}

template <typename DT>
Object HermesCtr::new_from_string(U8StringView str) {
    auto tag = ShortTypeCode::of<DT>();
    return get_type_reflection(tag).datatype_convert_from_plain_string(str);
}


inline ObjectMap ObjectView::as_object_map() const {
    return cast_to<Map<Varchar, Object>>();
}

inline TinyObjectMap ObjectView::as_tiny_object_map() const {
    return cast_to<Map<UTinyInt, Object>>();
}

inline ObjectArray ObjectView::as_object_array() const {
    return cast_to<Array<Object>>();
}

namespace detail {


template <typename T>
constexpr HermesCtrMakers HermesCtrMakerType = DataTypeTraits<T>::isDataType ?
            HermesCtrMakers::DATAOBJECT :
            HermesCtrMakers::OTHER;
}


template <typename DT>
struct HermesCtrMakeHelper<DT, HermesCtrMakers::DATAOBJECT> {
    template <typename T>
    static auto make(HermesCtr* ctr, const T& value) {
        //using DT = typename ViewToDTMapping<T>::Type;
        return ctr->template make_dataobject<DT>(value);
    }
};

template <typename T>
struct HermesCtrMakeHelper<Array<T>, HermesCtrMakers::OTHER> {
    template <typename... CtrArgs>
    static Array<T> make(HermesCtr* ctr, CtrArgs&&... args) {
        return ctr->template make_array<T>(std::forward<CtrArgs>(args)...);
    }
};

template <typename T>
struct HermesCtrMakeHelper<ArrayView<T>, HermesCtrMakers::OTHER> {
    template <typename... CtrArgs>
    static Array<T> make(HermesCtr* ctr, CtrArgs&&... args) {
        return ctr->template make_array<T>(std::forward<CtrArgs>(args)...);
    }
};


template <typename Key, typename Value>
struct HermesCtrMakeHelper<MapView<Key, Value>, HermesCtrMakers::OTHER> {
    template <typename... CtrArgs>
    static Map<Key, Value> make(HermesCtr* ctr, CtrArgs&&... args) {
        return ctr->template make_map<Key, Value>(std::forward<CtrArgs>(args)...);
    }
};

template <typename Key, typename Value>
struct HermesCtrMakeHelper<Map<Key, Value>, HermesCtrMakers::OTHER> {
    template <typename... CtrArgs>
    static Map<Key, Value> make(HermesCtr* ctr, CtrArgs&&... args) {
        return ctr->template make_map<Key, Value>(std::forward<CtrArgs>(args)...);
    }
};

template <>
struct HermesCtrMakeHelper<Parameter, HermesCtrMakers::OTHER> {
    template <typename ViewT>
    static Parameter make(HermesCtr* ctr, ViewT&& param_name) {
        return ctr->make_parameter(param_name);
    }
};

template <>
struct HermesCtrMakeHelper<ParameterView, HermesCtrMakers::OTHER> {
    template <typename ViewT>
    static Parameter make(HermesCtr* ctr, ViewT&& param_name) {
        return ctr->make_parameter(param_name);
    }
};

template <>
struct HermesCtrMakeHelper<Datatype, HermesCtrMakers::OTHER> {
    template <typename ViewT>
    static Datatype make(HermesCtr* ctr, ViewT&& datatype_name) {
        return ctr->make_datatype(datatype_name);
    }
};

template <>
struct HermesCtrMakeHelper<DatatypeView, HermesCtrMakers::OTHER> {
    template <typename ViewT>
    static Datatype make(HermesCtr* ctr, ViewT&& datatype_name) {
        return ctr->make_datatype(datatype_name);
    }
};

template <>
struct HermesCtrMakeHelper<TypedValue, HermesCtrMakers::OTHER> {
    template <typename ViewT>
    static TypedValue make(HermesCtr* ctr, const Datatype& datatype) {
        return ctr->make_typed_value(datatype);
    }
};

template <>
struct HermesCtrMakeHelper<TypedValueView, HermesCtrMakers::OTHER> {
    template <typename ViewT>
    static TypedValue make(HermesCtr* ctr, const Datatype& datatype) {
        return ctr->make_typed_value(datatype);
    }
};

template <typename T>
auto HermesCtr::make(T&& view) {
    using DT = typename ViewToDTMapping<T>::Type;
    return make_dataobject<DT>(view);
}

template <typename T, typename... CtrArgs>
auto HermesCtr::make_t(CtrArgs&&... args) {
    return HermesCtrMakeHelper<T, detail::HermesCtrMakerType<T>>::
        make(this, std::forward<CtrArgs>(args)...);
}



template <typename DT>
Object HermesCtr::make_dataobject(const DTViewArg<DT>& view)
{
    assert_not_null();
    assert_mutable();

    using DTCtr = DataObjectView<DT>;

    auto arena_dtc = arena_->allocate_tagged_object<typename DTCtr::ArenaDTContainer>(
        ShortTypeCode::of<DTCtr>(),
        view
    );

    return DataObject<DT>(mem_holder_, arena_dtc).as_object();
}

template <typename T, std::enable_if_t<std::is_same_v<T, ObjectArray>, int>>
Array<T> HermesCtr::make_array(uint64_t capacity)
{
    assert_not_null();
    assert_mutable();

    using CtrT = Array<T>;

    auto arena_dtc = arena()->allocate_tagged_object<typename CtrT::ArrayStorageT>(
        ShortTypeCode::of<CtrT>(), capacity
    );

    return Array<T>(mem_holder_, arena_dtc);
}

template <typename T, std::enable_if_t<!std::is_same_v<T, ObjectArray>, int>>
Array<T> HermesCtr::make_array(uint64_t capacity)
{
    assert_not_null();
    assert_mutable();

    using CtrT = Array<T>;

    auto arena_dtc = arena()->allocate_tagged_object<typename CtrT::ArrayStorageT>(
        ShortTypeCode::of<CtrT>(), capacity
    );

    return Array<T>(mem_holder_, arena_dtc);
}


namespace detail {

template <typename T>
struct ArrayMaker;

template <>
struct ArrayMaker<Object> {
    static auto make_array(HermesCtr* ctr, Span<const Object> span)
    {
        ObjectArray array = ctr->make_array<Object>(span.size());
        for (const Object& item: span) {
            array.append(item);
        }
        return array;
    }
};

template <typename T>
struct ArrayMaker {
    static auto make_array(HermesCtr* ctr, Span<const T> span)
    {
        using DT = typename ViewToDTMapping<T>::Type;
        auto array = ctr->make_array<DT>(span.size());
        for (const T& item: span) {
            array.append(item);
        }
        return array;
    }

    template <typename DT>
    static auto make_array_t(HermesCtr* ctr, Span<const T> span)
    {
        auto array = ctr->make_array<DT>(span.size());
        for (const T& item: span) {
            // FIXME: when append_t is avaialble, use it
            array.append((DTTViewType<DT>)item);
        }
        return array;
    }
};

}


template <typename T>
auto HermesCtr::make_array(Span<const T> span) {
    return detail::ArrayMaker<T>::make_array(this, span);
}

template <typename DT, typename T>
auto HermesCtr::make_array_t(Span<const T> span) {
    return detail::ArrayMaker<T>::template make_array_t<DT>(this, span);
}


template <typename Key, typename Value>
Map<Key, Value> HermesCtr::make_map(uint64_t capacity)
{
    using CtrT = Map<Key, Value>;

    auto arena_dtc = arena()->allocate_tagged_object<typename CtrT::MapStorageT>(
        ShortTypeCode::of<CtrT>()
    );

    return Map<Key, Value>(mem_holder_, arena_dtc);
}


inline ObjectArray HermesCtr::make_object_array(uint64_t capacity) {
    return HermesCtr::make_array<Object>(capacity);
}


inline Parameter HermesCtr::make_parameter(const U8StringView& name)
{
    assert_not_null();
    assert_mutable();

    auto arena_dtc = arena_->allocate_tagged_object<typename ParameterView::ArenaDTContainer>(
        ShortTypeCode::of<ParameterView>(),
        name
    );

    return Parameter(mem_holder_, arena_dtc);
}



inline Datatype HermesCtr::make_datatype(const U8StringView& name) {
    return Datatype{};
}

inline Datatype HermesCtr::make_datatype(const StringValue& name) {
    return Datatype{};
}


inline TypedValue HermesCtr::make_typed_value(const Datatype& datatype) {
    return TypedValue{};
}


namespace detail {

template <bool SmallV>
struct WrappingImportHelper {
    template <typename ViewT>
    static Object do_import(HermesCtr* ctr, const Own<ViewT, OwningKind::WRAPPING>& view)
    {
        using DT = typename ViewToDTMapping<ViewT>::Type;
        TaggedValue storage(ShortTypeCode::of<DT>(), view.value_t());
        return DataObject<DT>(DataObjectView<DT>(ctr->mem_holder_, storage));
    }
};

template <>
struct WrappingImportHelper<false> {
    template <typename ViewT>
    static Object do_import(HermesCtr* ctr, const Own<ViewT, OwningKind::WRAPPING>& obj)
    {
        using DT = typename ViewToDTMapping<ViewT>::Type;
        return ctr->make_t<DT>(obj);
    }
};



}


template <typename ViewT>
Object HermesCtr::import_object(const Own<ViewT, OwningKind::WRAPPING>& obj)
{
    using DT = typename ViewToDTMapping<ViewT>::Type;
    constexpr bool FitsIn = TaggedValue::dt_fits_in<DT>();
    return detail::WrappingImportHelper<FitsIn>::do_import(this, obj);
}

template <typename ViewT, OwningKind OK>
Object HermesCtr::import_object(const Own<ViewT, OK>& object)
{
    using DT = typename ViewToDTMapping<ViewT>::Type;
    if (object.get_mem_holder() == mem_holder_)
    {
        auto addr = arena::ArenaDataTypeContainer<DT>::from_view(object);
        return Object{mem_holder_, addr};
    }
    else {
        return make(object);
    }
}



inline Object HermesCtr::import_small_object(const Object& object)
{
    auto vs_tag = object->get_vs_tag();
    if (MMA_LIKELY(vs_tag == VS_TAG_SMALL_VALUE))
    {
        auto type_tag = object.get_type_tag();
        auto& refl = get_type_reflection(type_tag);
        return refl.import_value(mem_holder_, vs_tag, object.storage_);
    }
    else {
        return object;
    }
}


}}
