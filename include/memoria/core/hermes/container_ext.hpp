
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
#include <memoria/core/hermes/map/object_map.hpp>
#include <memoria/core/hermes/map/typed_map.hpp>
#include <memoria/core/hermes/array/object_array.hpp>
#include <memoria/core/hermes/array/typed_array.hpp>

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
Object HermesCtr::new_dataobject(DTTViewType<DT> view)
{
    using DTCtr = arena::ArenaDataTypeContainer<DT>;

    auto arena_dtc = arena_->allocate_tagged_object<DTCtr>(
        ShortTypeCode::of<DT>(),
        view
    );

    return Object(mem_holder_, arena_dtc);
}


template <typename DT>
Object HermesCtr::wrap_primitive(DTTViewType<DT> view)
{
    return wrap_primitive<DT>(view, common_instance().get());
}


template <typename DT>
Object HermesCtr::wrap_primitive(DTTViewType<DT> view, HermesCtr* ctr)
{
    static_assert(
        TaggedValue::dt_fits_in<DT>(),
        ""
    );

    TaggedValue storage(ShortTypeCode::of<DT>(), view);
    return Object(ctr->mem_holder_, storage);
}


template <typename DT>
Object HermesCtr::wrap_dataobject__full(DTTViewType<DT> view)
{
    using ContainerT = arena::ArenaDataTypeContainer<DT>;

    auto& arena = arena::get_local_instance();
    arena.object_pool_init_state();

    arena::Cleaner cleaner(arena);

    auto header  = arena.allocate_object_untagged<DocumentHeader>();
    auto tag     = ShortTypeCode::of<DT>();
    header->root = arena.allocate_tagged_object<ContainerT>(tag, view);

    if (arena.chunks() == 1)
    {
        auto instance = get_ctr_instance(arena.head().size);
        if (!instance.is_null())
        {
            instance->init_from(arena);
            return instance->root();
        }
    }

    auto instance = make_pooled();
    instance->init_from(arena);
    return instance->root();
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
Object HermesCtr::new_embeddable_dataobject(DTTViewType<DT> view)
{
    return detail::DTSizeDispatcher<
            DT,
            arena::ERelativePtr::dt_fits_in<DT>()
    >::dispatch(view, this);
}


template <typename DT>
Object HermesCtr::wrap_dataobject(DTTViewType<DT> view)
{
    return detail::DTSizeDispatcher<
        DT, TaggedValue::dt_fits_in<DT>()
    >::dispatch(view);
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


namespace detail {


template <typename T>
constexpr CtrMakers HermesCtrMakerType = DataTypeTraits<T>::isDataType ?
            CtrMakers::DATAOBJECT :
            CtrMakers::OTHER;
}


template <typename DT>
struct CtrMakeHelper<DT, CtrMakers::DATAOBJECT> {
    template <typename... Args>
    static auto make(HermesCtr* ctr, Args&&... args) {
        return ctr->template make_dataobject<DT>(std::forward<Args>(args)...);
    }

    static Object import_object(HermesCtr* ctr, const Object& source) {
        auto view = source.view_unchecked<DT>();
        using CtrT = arena::ArenaDataTypeContainer<DT>;

        auto dtc = ctr->arena()->allocate_tagged_object<CtrT>(
            ShortTypeCode::of<DT>(), view
        );

        return Object(ctr->get_mem_holder(), dtc);
    }
};

template <typename T>
struct CtrMakeHelper<Array<T>, CtrMakers::OTHER> {
    template <typename... CtrArgs>
    static Array<T> make(HermesCtr* ctr, CtrArgs&&... args) {
        return ctr->template make_array<T>(std::forward<CtrArgs>(args)...);
    }
};

template <typename T>
struct CtrMakeHelper<ArrayView<T>, CtrMakers::OTHER> {
    template <typename... CtrArgs>
    static Array<T> make(HermesCtr* ctr, CtrArgs&&... args) {
        return ctr->template make_array<T>(std::forward<CtrArgs>(args)...);
    }
};


template <typename Key, typename Value>
struct CtrMakeHelper<MapView<Key, Value>, CtrMakers::OTHER> {
    template <typename... CtrArgs>
    static Map<Key, Value> make(HermesCtr* ctr, CtrArgs&&... args) {
        return ctr->template make_map<Key, Value>(std::forward<CtrArgs>(args)...);
    }
};

template <typename Key, typename Value>
struct CtrMakeHelper<Map<Key, Value>, CtrMakers::OTHER> {
    template <typename... CtrArgs>
    static Map<Key, Value> make(HermesCtr* ctr, CtrArgs&&... args) {
        return ctr->template make_map<Key, Value>(std::forward<CtrArgs>(args)...);
    }
};

template <>
struct CtrMakeHelper<Parameter, CtrMakers::OTHER> {
    template <typename ViewT>
    static Parameter make(HermesCtr* ctr, ViewT&& param_name) {
        return ctr->make_parameter(param_name);
    }
};

template <>
struct CtrMakeHelper<ParameterView, CtrMakers::OTHER> {
    template <typename ViewT>
    static Parameter make(HermesCtr* ctr, ViewT&& param_name) {
        return ctr->make_parameter(param_name);
    }
};

template <>
struct CtrMakeHelper<Datatype, CtrMakers::OTHER> {
    template <typename ViewT>
    static Datatype make(HermesCtr* ctr, ViewT&& datatype_name) {
        return ctr->make_datatype(datatype_name);
    }
};

template <>
struct CtrMakeHelper<DatatypeView, CtrMakers::OTHER> {
    template <typename ViewT>
    static Datatype make(HermesCtr* ctr, ViewT&& datatype_name) {
        return ctr->make_datatype(datatype_name);
    }
};

template <>
struct CtrMakeHelper<TypedValue, CtrMakers::OTHER> {
    template <typename ViewT>
    static TypedValue make(HermesCtr* ctr, const Datatype& datatype, const Object& constructor) {
        return ctr->make_typed_value(datatype, constructor);
    }
};

template <>
struct CtrMakeHelper<TypedValueView, CtrMakers::OTHER> {
    template <typename ViewT>
    static TypedValue make(HermesCtr* ctr, const Datatype& datatype, const Object& constructor) {
        return ctr->make_typed_value(datatype, constructor);
    }
};

template <typename T, typename std::enable_if_t<!HermesObject<T>::Value, int>>
auto HermesCtr::make(T&& view) {
    using DT = typename ViewToDTMapping<std::decay_t<T>>::Type;
    return make_dataobject<DT>(view);
}

template <typename T, typename... CtrArgs>
auto HermesCtr::make_t(CtrArgs&&... args) {
    return CtrMakeHelper<T, detail::HermesCtrMakerType<T>>::
        make(this, std::forward<CtrArgs>(args)...);
}



template <typename DT>
Object HermesCtr::make_dataobject(const DTViewArg<DT>& view)
{
    assert_not_null();
    assert_mutable();

    return detail::DTSizeDispatcher<
            DT,
            arena::ERelativePtr::dt_fits_in<DT>()
    >::dispatch(view, this);

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
            array = array.push_back(item);
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
            array = array.push_back(item);
        }
        return array;
    }

    template <typename DT>
    static auto make_array_t(HermesCtr* ctr, Span<const T> span)
    {
        auto array = ctr->make_array<DT>(span.size());
        for (const T& item: span) {
            // FIXME: when append_t is avaialble, use it
            array = array.push_back((DTTViewType<DT>)item);
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




namespace detail {

template <bool SmallV>
struct WrappingImportHelper {
    template <typename ViewT>
    static Object do_import(HermesCtr* ctr, const Own<ViewT, OwningKind::WRAPPING>& view)
    {
        using DT = typename ViewToDTMapping<ViewT>::Type;
        TaggedValue storage(ShortTypeCode::of<DT>(), view.value_t());
        return Object(ctr->mem_holder_, storage);
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
        return Object(mem_holder_, addr);
    }
    else {
        return make(object);
    }
}




}}
