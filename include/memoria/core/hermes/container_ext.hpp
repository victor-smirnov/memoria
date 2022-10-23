
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

inline void HermesCtr::assert_mutable()
{
    if (MMA_UNLIKELY(!this->is_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("Map<String, Value> is immutable").do_throw();
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
DataObjectPtr<DT> HermesCtr::wrap_dataobject(DTTViewType<DT> view) {
    auto doc = make_pooled();
    return doc->set_dataobject<DT>(view);
}


inline GenericArrayPtr Value::as_generic_array() const {
    return cast_to<GenericArray>();
}


inline GenericMapPtr Value::as_generic_map() const {
    return cast_to<GenericMap>();
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
    auto tag = arena::read_type_tag(addr_);
    return get_type_reflection(tag).str();
}


template <typename DT>
PoolSharedPtr<HermesCtr> Value::convert_to() const
{
    assert_not_null();
    auto src_tag = arena::read_type_tag(addr_);
    auto to_tag = TypeHashV<DT>;
    return get_type_reflection(src_tag).datatype_convert_to(to_tag, addr_, doc_, ptr_holder_);
}

template <typename DT>
template <typename ToDT>
PoolSharedPtr<HermesCtr> DataObject<DT>::convert_to() const
{
    assert_not_null();
    auto src_tag = arena::read_type_tag(dt_ctr_);
    auto to_tag = TypeHashV<ToDT>;
    return get_type_reflection(src_tag).datatype_convert_to(to_tag, dt_ctr_, doc_, ptr_holder_);
}

}}
