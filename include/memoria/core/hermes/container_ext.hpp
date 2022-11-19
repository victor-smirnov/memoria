
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
#include <memoria/core/hermes/array/array.hpp>
#include <memoria/core/hermes/array/typed_array.hpp>
#include <memoria/core/hermes/data_object.hpp>

#include <memoria/core/hermes/object.hpp>

namespace memoria {
namespace hermes {

class StaticHermesCtrImpl: public HermesCtr, public pool::enable_shared_from_this<StaticHermesCtrImpl> {
protected:
    ViewPtrHolder view_ptr_holder_;
    mutable uint8_t* segment_;
    size_t capacity_;

    friend class HermesCtr;

public:
    StaticHermesCtrImpl(uint8_t* segment, size_t capacity) noexcept:
        segment_(segment), capacity_(capacity)
    {
        ptr_holder_ = &view_ptr_holder_;
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
        view_ptr_holder_.set_owner(ref_holder);
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
DataObjectPtr<DT> HermesCtr::new_dataobject(DTTViewType<DT> view)
{
    using DTCtr = DataObject<DT>;

    auto arena_dtc = arena_->allocate_tagged_object<typename DTCtr::ArenaDTContainer>(
        ShortTypeCode::of<DTCtr>(),
        view
    );

    return DataObjectPtr<DT>(DTCtr(arena_dtc, this, ptr_holder_));
}



template <typename DT>
DataObjectPtr<DT> HermesCtr::wrap_primitive(DTTViewType<DT> view)
{
    static_assert(
        TaggedValue::dt_fits_in<DT>(),
        ""
    );

    auto ctr = common_instance();
    TaggedValue storage(ShortTypeCode::of<DT>(), view);
    return DataObjectPtr<DT>(DataObject<DT>(storage, ctr.get(), ctr->ptr_holder_));
}

template <typename DT>
DataObjectPtr<DT> HermesCtr::wrap_dataobject__full(DTTViewType<DT> view)
{
    using DataObjectT = DataObject<DT>;
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
};

template <typename DT>
struct DTSizeDispatcher<DT, false> {
    static auto dispatch(const DTTViewType<DT>& view) {
        return HermesCtr::wrap_dataobject__full<DT>(view);
    }
};

}

template <typename DT>
DataObjectPtr<DT> HermesCtr::wrap_dataobject(DTTViewType<DT> view)
{
    return detail::DTSizeDispatcher<
        DT, TaggedValue::dt_fits_in<DT>()
    >::dispatch(view);
}

inline GenericArrayPtr Object::as_generic_array() const
{
    if (get_vs_tag() == VS_TAG_ADDRESS)
    {
        auto tag = get_type_tag();
        auto ctr_ptr = get_type_reflection(tag).hermes_make_wrapper(storage_.addr, doc_, get_ptr_holder());
        return ctr_ptr->as_array();
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Object is not addressable").do_throw();
    }
}


inline GenericMapPtr Object::as_generic_map() const
{
    if (get_vs_tag() == VS_TAG_ADDRESS)
    {
        auto tag = get_type_tag();
        auto ctr_ptr = get_type_reflection(tag).hermes_make_wrapper(storage_.addr, doc_, get_ptr_holder());
        return ctr_ptr->as_map();
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Object is not addressable").do_throw();
    }
}

inline DatatypePtr Object::as_datatype() const {
    return cast_to<Datatype>();
}

inline DataObjectPtr<Varchar> Object::as_varchar() const {
    return cast_to<DataObject<Varchar>>();
}

inline DataObjectPtr<Double> Object::as_double() const {
    return cast_to<DataObject<Double>>();
}

inline DataObjectPtr<BigInt> Object::as_bigint() const {
    return cast_to<DataObject<BigInt>>();
}

inline DataObjectPtr<Boolean> Object::as_boolean() const {
    return cast_to<DataObject<Boolean>>();
}

inline DataObjectPtr<Real> Object::as_real() const {
    return cast_to<DataObject<Real>>();
}

inline int64_t Object::to_i64() const {
    return *convert_to<BigInt>()->as_data_object<BigInt>()->view();
}

inline U8String Object::to_str() const {
    return *convert_to<Varchar>()->as_data_object<Varchar>()->view();
}

inline bool Object::to_bool() const {
    return *convert_to<Boolean>()->as_data_object<Boolean>()->view();
}

inline double Object::to_d64() const {
    return *convert_to<Double>()->as_data_object<Double>()->view();
}

inline float Object::to_f32() const {
    return *convert_to<Real>()->as_data_object<Real>()->view();
}


inline U8String Object::type_str() const {
    assert_not_null();
    auto tag = get_type_tag();
    return get_type_reflection(tag).str();
}


template <typename DT>
ObjectPtr Object::convert_to() const
{
    assert_not_null();
    auto src_tag = get_type_tag();
    auto to_tag = ShortTypeCode::of<DT>();
    if (src_tag != to_tag) {
        return get_type_reflection(src_tag).datatype_convert_to(to_tag, get_vs_tag(), storage_, doc_, get_ptr_holder());
    }
    else {
        return this->as_object();
    }
}

template <typename DT>
template <typename ToDT>
DataObjectPtr<ToDT> DataObject<DT>::convert_to() const
{
    assert_not_null();
    auto src_tag = get_type_tag();
    auto to_tag = ShortTypeCode::of<ToDT>();
    return get_type_reflection(src_tag).datatype_convert_to(to_tag, get_vs_tag(), storage_, doc_, this->get_ptr_holder());
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
ArrayPtr<DT> HermesCtr::new_typed_array() {
    assert_not_null();
    assert_mutable();

    using CtrT = Array<DT>;

    auto arena_dtc = arena()->allocate_tagged_object<typename CtrT::ArrayStorageT>(
        ShortTypeCode::of<CtrT>()
    );

    return ArrayPtr<DT>(CtrT(arena_dtc, this, ptr_holder_));
}

template <typename KeyDT, typename ValueDT>
MapPtr<KeyDT, ValueDT> HermesCtr::new_typed_map() {
    assert_not_null();
    assert_mutable();

    using CtrT = Map<KeyDT, ValueDT>;

    auto arena_dtc = arena()->allocate_tagged_object<typename CtrT::MapStorageT>(
        ShortTypeCode::of<CtrT>()
    );

    return MapPtr<KeyDT, ValueDT>(CtrT(arena_dtc, this, ptr_holder_));
}

template <typename DT>
ObjectPtr HermesCtr::new_from_string(U8StringView str) {
    auto tag = ShortTypeCode::of<DT>();
    return get_type_reflection(tag).datatype_convert_from_plain_string(str);
}


inline ObjectMapPtr Object::as_object_map() const {
    return cast_to<Map<Varchar, Object>>();
}

inline ObjectArrayPtr Object::as_object_array() const {
    return cast_to<Array<Object>>();
}


}}
