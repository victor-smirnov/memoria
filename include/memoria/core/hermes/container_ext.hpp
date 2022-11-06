
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
#include <memoria/core/hermes/map.hpp>
#include <memoria/core/hermes/array.hpp>
#include <memoria/core/hermes/data_object.hpp>

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
        TypeHashV<DTCtr>,
        view
    );

    return DataObjectPtr<DT>(DTCtr(arena_dtc, this, ptr_holder_));
}

template <typename DT>
DataObjectPtr<DT> HermesCtr::wrap_dataobject(DTTViewType<DT> view)
{
    using DataObjectT = DataObject<DT>;
    using ContainerT = typename DataObjectT::ArenaDTContainer;

    auto& arena = arena::get_local_instance();
    arena.object_pool_init_state();

    arena::Cleaner cleaner(arena);

    auto header  = arena.allocate_object_untagged<DocumentHeader>();
    auto tag     = TypeHashV<DataObjectT>;
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


inline GenericArrayPtr Value::as_generic_array() const {
    return cast_to<GenericArray>();
}


inline GenericMapPtr Value::as_generic_map() const {
    return cast_to<GenericMap>();
}

inline DatatypePtr Value::as_datatype() const {
    return cast_to<Datatype>();
}

inline DataObjectPtr<Varchar> Value::as_varchar() const {
    return cast_to<Varchar>();
}

inline DataObjectPtr<Double> Value::as_double() const {
    return cast_to<Double>();
}

inline DataObjectPtr<BigInt> Value::as_bigint() const {
    return cast_to<BigInt>();
}

inline DataObjectPtr<Boolean> Value::as_boolean() const {
    return cast_to<Boolean>();
}

inline DataObjectPtr<Real> Value::as_real() const {
    return cast_to<Real>();
}

inline U8String Value::type_str() const {
    assert_not_null();
    auto tag = arena::read_type_tag(value_storage_.addr);
    return get_type_reflection(tag).str();
}


template <typename DT>
ValuePtr Value::convert_to() const
{
    assert_not_null();
    auto src_tag = arena::read_type_tag(value_storage_.addr);
    auto to_tag = TypeHashV<DT>;
    return get_type_reflection(src_tag).datatype_convert_to(to_tag, value_storage_.addr, doc_, get_ptr_holder());
}

template <typename DT>
template <typename ToDT>
DataObjectPtr<ToDT> DataObject<DT>::convert_to() const
{
    assert_not_null();
    auto src_tag = arena::read_type_tag(value_storage_.addr);
    auto to_tag = TypeHashV<ToDT>;
    return get_type_reflection(src_tag).datatype_convert_to(to_tag, value_storage_.addr, doc_, this->get_ptr_holder());
}

}}
