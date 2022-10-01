
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

#include <memoria/core/hermes/array.hpp>
#include <memoria/core/hermes/document.hpp>
#include <memoria/core/hermes/data_object.hpp>
#include <memoria/core/hermes/map.hpp>

namespace memoria {
namespace hermes {

inline void Array<Value>::assert_mutable()
{
    if (MMA_UNLIKELY(doc_->is_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("Array<Value> is immutable");
    }
}


template <typename DT>
DataObjectPtr<DT> Array<Value>::append(DTTViewType<DT> view)
{
    assert_not_null();
    assert_mutable();

    auto ptr = doc_->new_tv<DT>(view);

    array_->push_back(*doc_->arena(), ptr->dt_ctr_);

    return ptr;
}

inline GenericMapPtr Array<Value>::append_generic_map()
{
    assert_not_null();
    assert_mutable();

    auto ptr = doc_->new_map();
    array_->push_back(*doc_->arena(), ptr->map_);

    return ptr;
}


inline GenericArrayPtr Array<Value>::append_generic_array()
{
    assert_not_null();
    assert_mutable();

    auto ptr = doc_->new_array();
    array_->push_back(*doc_->arena(), ptr->array_);

    return ptr;
}

inline DatatypePtr Array<Value>::append_datatype(U8StringView name) {
    assert_not_null();
    assert_mutable();

    auto ptr = doc_->new_datatype(name);
    array_->push_back(*doc_->arena(), ptr->datatype_);

    return ptr;
}

inline DatatypePtr Array<Value>::append_datatype(StringValuePtr name) {
    assert_not_null();
    assert_mutable();

    auto ptr = doc_->new_datatype(name);
    array_->push_back(*doc_->arena(), ptr->datatype_);

    return ptr;
}

template <typename DT>
DataObjectPtr<DT> Array<Value>::set(uint64_t idx, DTTViewType<DT> view)
{    
    assert_not_null();
    assert_mutable();

    if (MMA_LIKELY(idx < array_->size()))
    {
        auto ptr = doc_->new_tv<DT>(view);
        array_->set(idx, ptr->dt_ctr_);
        return ptr;
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Range check in Array<Value>::set(): {}::{}", idx, array_->size()).do_throw();
    }
}



inline GenericMapPtr Array<Value>::set_generic_map(uint64_t idx)
{
    assert_not_null();
    assert_mutable();

    if (MMA_LIKELY(idx < array_->size()))
    {
        auto ptr = doc_->new_map();
        array_->set(idx, ptr->map_);
        return ptr;
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Range check in Array<Value>::set(): {}::{}", idx, array_->size()).do_throw();
    }
}

inline GenericArrayPtr Array<Value>::set_generic_array(uint64_t idx)
{
    assert_not_null();
    assert_mutable();

    if (MMA_LIKELY(idx < array_->size()))
    {
        auto ptr = doc_->new_array();
        array_->set(idx, ptr->array_);
        return ptr;
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Range check in Array<Value>::set(): {}::{}", idx, array_->size()).do_throw();
    }
}


inline void Array<Value>::append(ValuePtr value)
{
    assert_not_null();
    assert_mutable();

    array_->push_back(*doc_->arena(), value->addr_);
}

}}
