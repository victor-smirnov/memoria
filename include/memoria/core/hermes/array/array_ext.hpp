
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

#include <memoria/core/hermes/array/array.hpp>
#include <memoria/core/hermes/array/typed_array.hpp>

#include <memoria/core/hermes/container.hpp>
#include <memoria/core/hermes/data_object.hpp>
#include <memoria/core/hermes/map/map.hpp>

namespace memoria {
namespace hermes {


inline void Array<Object>::assert_mutable()
{
    if (MMA_UNLIKELY(!doc_->is_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("Array<Object> is immutable").do_throw();
    }
}

template <typename DT>
void Array<DT>::assert_mutable()
{
    if (MMA_UNLIKELY(!doc_->is_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("Array<DT> is immutable").do_throw();
    }
}


template <typename DT>
DataObjectPtr<DT> Array<Object>::append(DTTViewType<DT> view)
{
    assert_not_null();
    assert_mutable();

    auto ptr = doc_->new_dataobject<DT>(view);
    array_->push_back(*doc_->arena(), ptr->dt_ctr());
    return ptr;
}


template <typename DT>
DataObjectPtr<DT> Array<Object>::set(uint64_t idx, DTTViewType<DT> view)
{    
    assert_not_null();
    assert_mutable();

    if (MMA_LIKELY(idx < array_->size()))
    {
        auto ptr = doc_->new_dataobject<DT>(view);
        array_->set(idx, ptr->dt_ctr());
        return ptr;
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Range check in Array<Object>::set(): {}::{}", idx, array_->size()).do_throw();
    }
}


inline void Array<Object>::append(const ObjectPtr& value)
{
    assert_not_null();
    assert_mutable();

    auto vv = doc_->do_import_value(value);

    if (MMA_LIKELY(!vv->is_null()))
    {
        array_->push_back(*doc_->arena(), vv->storage_.addr);
    }
    else {
        array_->push_back(*doc_->arena(), nullptr);
    }
}

template <typename DT>
void Array<DT>::append(const DataObjectPtr<DT>& value)
{
    assert_not_null();
    assert_mutable();

    array_->push_back(*doc_->arena(), *value->view());
}

template <typename DT>
void Array<DT>::append(DTTViewType<DT> value)
{
    assert_not_null();
    assert_mutable();

    array_->push_back(*doc_->arena(), value);
}


template <typename DT>
void Array<DT>::set(uint64_t idx, const DataObjectPtr<DT>& value)
{
    assert_not_null();
    assert_mutable();
    array_->set(idx, *value->view());
}

template <typename DT>
void Array<DT>::set(uint64_t idx, DTTViewType<DT> value)
{
    assert_not_null();
    assert_mutable();
    array_->set(idx, value);
}


inline void Array<Object>::set(uint64_t idx, const ObjectPtr& value)
{
    assert_not_null();
    assert_mutable();

    auto vv = doc_->do_import_value(value);

    if (MMA_LIKELY(!vv->is_null()))
    {
        array_->set(idx, vv->storage_.addr);
    }
    else {
        array_->set(idx, nullptr);
    }
}

inline ObjectPtr Array<Object>::append_hermes(U8StringView str) {
  assert_not_null();
  assert_mutable();

  ObjectPtr vv = doc_->parse_raw_value(str.begin(), str.end());

  array_->push_back(*doc_->arena(), vv->storage_.addr);

  return vv;
}

inline ObjectPtr Array<Object>::set_hermes(uint64_t idx, U8StringView str) {
  assert_not_null();
  assert_mutable();

  if (MMA_LIKELY(idx < array_->size()))
  {
      ObjectPtr vv = doc_->parse_raw_value(str.begin(), str.end());
      auto vv1 = doc_->do_import_value(vv);
      array_->set(idx, vv1->storage_.addr);
      return vv1;
  }
  else {
      MEMORIA_MAKE_GENERIC_ERROR("Range check in Array<Object>::set_hermes(): {}::{}", idx, array_->size()).do_throw();
  }
}


inline void Array<Object>::remove(uint64_t idx)
{
    assert_not_null();
    assert_mutable();

    if (idx < array_->size()) {
        array_->remove(*doc_->arena_, idx);
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Range check in Array<Object>::remove(): {}::{}", idx, array_->size()).do_throw();
    }
}

inline ObjectPtr Array<Object>::append_null() {
    assert_not_null();
    assert_mutable();

    array_->push_back(*doc_->arena(), nullptr);
    return ObjectPtr{};
}

inline PoolSharedPtr<GenericArray> Array<Object>::as_generic_array() const {
    return TypedGenericArray<Object>::make_wrapper(array_, doc_, ptr_holder_);
}


template <typename DT>
PoolSharedPtr<GenericArray> TypedGenericArray<DT>::make_wrapper(void* array, HermesCtr* ctr, ViewPtrHolder* ctr_holder) {
    using GAPoolT = pool::SimpleObjectPool<TypedGenericArray<DT>>;
    using GAPoolPtrT = boost::local_shared_ptr<GAPoolT>;

    static thread_local GAPoolPtrT wrapper_pool = MakeLocalShared<GAPoolT>();
    return wrapper_pool->allocate_shared(array, ctr, ctr_holder);
}


}}
