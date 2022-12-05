
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
    if (MMA_UNLIKELY(!get_ptr_holder()->ctr()->is_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("Array<Object> is immutable").do_throw();
    }
}

template <typename DT>
void Array<DT>::assert_mutable()
{
    if (MMA_UNLIKELY(!this->get_ptr_holder()->ctr()->is_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("Array<DT> is immutable").do_throw();
    }
}


inline ObjectPtr Array<Object>::get(uint64_t idx) const
{
    assert_not_null();

    if (idx < array_->size())
    {
        const auto& ptr = array_->get(idx);
        if (MMA_LIKELY(ptr.is_pointer()))
        {
            if (MMA_LIKELY(ptr.is_not_null())) {
                return ObjectPtr(Object(ptr_holder_, ptr.get()));
            }
            else {
                return ObjectPtr{};
            }
        }
        else {
            TaggedValue tv(ptr);
            return ObjectPtr(Object(ptr_holder_, tv));
        }
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Range check in Array<Object>: {} {}", idx, array_->size()).do_throw();
    }
}


inline void Array<Object>::for_each(std::function<void(const ObjectPtr&)> fn) const {
    assert_not_null();

    for (auto& vv: array_->span()) {
        if (vv.is_pointer())
        {
            if (vv.is_not_null()) {
                fn(ObjectPtr(Object(ptr_holder_, vv.get())));
            }
            else {
                fn(ObjectPtr(Object()));
            }
        }
        else {
            TaggedValue tv(vv);
            fn(ObjectPtr(Object(ptr_holder_, tv)));
        }
    }
}

template <typename DT>
ObjectArrayPtr Array<Object>::append(DTTViewType<DT> view)
{
    assert_not_null();
    assert_mutable();

    auto ctr = ptr_holder_->ctr();
    auto vv  = ctr->new_embeddable_dataobject<DT>(view);
    return append(vv->as_object());
}


template <typename DT>
DataObjectPtr<DT> Array<Object>::set(uint64_t idx, DTTViewType<DT> view)
{    
    assert_not_null();
    assert_mutable();
    auto ctr = ptr_holder_->ctr();
    auto ptr = ctr->new_embeddable_dataobject<DT>(view);
    this->set(idx, ptr->as_object());

    return ptr;

}


inline ObjectArrayPtr Array<Object>::append(const ObjectPtr& value)
{
    assert_not_null();
    assert_mutable();

    auto ctr = ptr_holder_->ctr();

    ShortTypeCode mytag = arena::read_type_tag(array_);

    ArrayStorageT* new_array;

    if (value->is_not_null())
    {
        arena::ERelativePtr val_ptr;

        if (value->get_vs_tag() == VS_TAG_SMALL_VALUE)
        {
            ShortTypeCode tag = value->get_type_tag();
            bool do_import = !get_type_reflection(tag).hermes_embed(val_ptr, value->storage_.small_value);
            if (do_import)
            {
                auto vv = ctr->do_import_value(value);
                val_ptr = vv->storage_.addr;
            }
        }
        else {
            auto vv = ctr->do_import_value(value);
            val_ptr = vv->storage_.addr;
        }

        new_array = array_->push_back(*ctr->arena(), mytag, val_ptr);
    }
    else {
        new_array = array_->push_back(*ctr->arena(), mytag, nullptr);
    }

    return ObjectArrayPtr{ObjectArray{ptr_holder_, new_array}};
}

template <typename DT>
ArrayPtr<DT> Array<DT>::append(const DataObjectPtr<DT>& value)
{
    assert_not_null();
    assert_mutable();

    auto ctr = ptr_holder_->ctr();
    auto mytag = ShortTypeCode::of<Array<DT>>();
    auto* new_array = array_->push_back(*ctr->arena(), mytag, *value->view());
    return ArrayPtr<DT>{Array<DT>{ptr_holder_, new_array}};
}

template <typename DT>
ArrayPtr<DT> Array<DT>::append(DTTViewType<DT> value)
{
    assert_not_null();
    assert_mutable();

    auto ctr = ptr_holder_->ctr();
    auto* new_array = array_->push_back(*ctr->arena(), value);
    return ArrayPtr<DT>{Array<DT>{ptr_holder_, new_array}};
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
    auto ctr = ptr_holder_->ctr();

    auto vv = ctr->do_import_value(value);

    if (MMA_LIKELY(!vv->is_null()))
    {
        array_->set(idx, vv->storage_.addr);
    }
    else {
        array_->set(idx, nullptr);
    }
}


inline ObjectPtr Array<Object>::set_hermes(uint64_t idx, U8StringView str) {
  assert_not_null();
  assert_mutable();

  if (MMA_LIKELY(idx < array_->size()))
  {
      auto ctr = ptr_holder_->ctr();
      ObjectPtr vv = ctr->parse_raw_value(str.begin(), str.end());
      auto vv1 = ctr->do_import_value(vv);
      array_->set(idx, vv1->storage_.addr);
      return vv1;
  }
  else {
      MEMORIA_MAKE_GENERIC_ERROR("Range check in Array<Object>::set_hermes(): {}::{}", idx, array_->size()).do_throw();
  }
}


inline ObjectArrayPtr Array<Object>::remove(uint64_t idx)
{
    assert_not_null();
    assert_mutable();

    auto ctr = ptr_holder_->ctr();
    ShortTypeCode mytag = arena::read_type_tag(array_);
    ArrayStorageT* new_array = array_->remove(*ctr->arena_, mytag, idx);
    return ObjectArrayPtr{ObjectArray{ptr_holder_, new_array}};
}

template <typename DT>
ArrayPtr<DT> Array<DT>::remove(uint64_t idx)
{
    assert_not_null();
    assert_mutable();

    auto ctr = ptr_holder_->ctr();
    ShortTypeCode mytag = arena::read_type_tag(array_);
    ArrayStorageT* new_array = array_->remove(*ctr->arena_, mytag, idx);
    return ObjectArrayPtr{ObjectArray{ptr_holder_, new_array}};
}


inline PoolSharedPtr<GenericArray> Array<Object>::as_generic_array() const {
    return TypedGenericArray<Object>::make_wrapper(ptr_holder_, array_);
}


template <typename DT>
PoolSharedPtr<GenericArray> TypedGenericArray<DT>::make_wrapper(ViewPtrHolder* ctr_holder, void* array) {
    using GAPoolT = pool::SimpleObjectPool<TypedGenericArray<DT>>;
    using GAPoolPtrT = boost::local_shared_ptr<GAPoolT>;

    static thread_local GAPoolPtrT wrapper_pool = MakeLocalShared<GAPoolT>();
    return wrapper_pool->allocate_shared(ctr_holder, array);
}


}}
