
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

#include <memoria/core/hermes/array/object_array.hpp>
#include <memoria/core/hermes/array/typed_array.hpp>

#include <memoria/core/hermes/container.hpp>
#include <memoria/core/hermes/data_object.hpp>
#include <memoria/core/hermes/map/object_map.hpp>

namespace memoria {
namespace hermes {


inline void ArrayView<Object>::assert_mutable()
{
    if (MMA_UNLIKELY(!get_mem_holder()->ctr()->is_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("ArrayView<Object> is immutable").do_throw();
    }
}

template <typename DT>
void ArrayView<DT>::assert_mutable()
{
    if (MMA_UNLIKELY(!this->get_mem_holder()->ctr()->is_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("ArrayView<DT> is immutable").do_throw();
    }
}


inline Object ArrayView<Object>::get(uint64_t idx) const
{
    assert_not_null();

    if (idx < array_->size())
    {
        const auto& ptr = array_->get(idx);
        if (MMA_LIKELY(ptr.is_pointer()))
        {
            if (MMA_LIKELY(ptr.is_not_null())) {
                return Object(ObjectView(mem_holder_, ptr.get()));
            }
            else {
                return Object{};
            }
        }
        else {
            TaggedValue tv(ptr);
            return Object(ObjectView(mem_holder_, tv));
        }
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Range check in ArrayView<Object>: {} {}", idx, array_->size()).do_throw();
    }
}


inline void ArrayView<Object>::for_each(std::function<void(const Object&)> fn) const {
    assert_not_null();

    for (auto& vv: array_->span()) {
        if (vv.is_pointer())
        {
            if (vv.is_not_null()) {
                fn(Object(ObjectView(mem_holder_, vv.get())));
            }
            else {
                fn(Object(ObjectView()));
            }
        }
        else {
            TaggedValue tv(vv);
            fn(Object(ObjectView(mem_holder_, tv)));
        }
    }
}

template <typename DT>
ObjectArray ArrayView<Object>::append(DTTViewType<DT> view)
{
    assert_not_null();
    assert_mutable();

    auto ctr = mem_holder_->ctr();
    auto vv  = ctr->new_embeddable_dataobject<DT>(view);
    return append(vv->as_object());
}


template <typename DT>
Object ArrayView<Object>::set(uint64_t idx, DTTViewType<DT> view)
{    
    assert_not_null();
    assert_mutable();
    auto ctr = mem_holder_->ctr();
    auto ptr = ctr->new_embeddable_dataobject<DT>(view);
    this->set(idx, ptr->as_object());

    return ptr;
}


inline ObjectArray ArrayView<Object>::append(const Object& value)
{
    assert_not_null();
    assert_mutable();

    auto ctr = mem_holder_->ctr();

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

    return ObjectArray{ObjectArrayView{mem_holder_, new_array}};
}

template <typename DT>
Array<DT> ArrayView<DT>::append(const DataObject<DT>& value)
{
    assert_not_null();
    assert_mutable();

    auto ctr = mem_holder_->ctr();
    auto mytag = ShortTypeCode::of<ArrayView<DT>>();
    auto* new_array = array_->push_back(*ctr->arena(), mytag, *value->view());
    return Array<DT>{ArrayView<DT>{mem_holder_, new_array}};
}

template <typename DT>
Array<DT> ArrayView<DT>::append(DTTViewType<DT> value)
{
    assert_not_null();
    assert_mutable();

    auto ctr = mem_holder_->ctr();
    auto mytag = ShortTypeCode::of<ArrayView<DT>>();
    auto* new_array = array_->push_back(*ctr->arena(), mytag, value);
    return Array<DT>{ArrayView<DT>{mem_holder_, new_array}};
}


template <typename DT>
void ArrayView<DT>::set(uint64_t idx, const DataObject<DT>& value)
{
    assert_not_null();
    assert_mutable();
    array_->set(idx, *value->view());
}

template <typename DT>
void ArrayView<DT>::set(uint64_t idx, DTTViewType<DT> value)
{
    assert_not_null();
    assert_mutable();
    array_->set(idx, value);
}


inline void ArrayView<Object>::set(uint64_t idx, const Object& value)
{
    assert_not_null();
    assert_mutable();
    auto ctr = mem_holder_->ctr();

    auto vv = ctr->do_import_value(value);

    if (MMA_LIKELY(!vv->is_null()))
    {
        array_->set(idx, vv->storage_.addr);
    }
    else {
        array_->set(idx, nullptr);
    }
}


inline Object ArrayView<Object>::set_hermes(uint64_t idx, U8StringView str) {
  assert_not_null();
  assert_mutable();

  if (MMA_LIKELY(idx < array_->size()))
  {
      auto ctr = mem_holder_->ctr();
      Object vv = ctr->parse_raw_value(str.begin(), str.end());
      auto vv1 = ctr->do_import_value(vv);
      array_->set(idx, vv1->storage_.addr);
      return vv1;
  }
  else {
      MEMORIA_MAKE_GENERIC_ERROR("Range check in ArrayView<Object>::set_hermes(): {}::{}", idx, array_->size()).do_throw();
  }
}


inline ObjectArray ArrayView<Object>::remove(uint64_t idx)
{
    assert_not_null();
    assert_mutable();

    auto ctr = mem_holder_->ctr();
    ShortTypeCode mytag = arena::read_type_tag(array_);
    ArrayStorageT* new_array = array_->remove(*ctr->arena_, mytag, idx);
    return ObjectArray{ObjectArrayView{mem_holder_, new_array}};
}

template <typename DT>
Array<DT> ArrayView<DT>::remove(uint64_t idx)
{
    assert_not_null();
    assert_mutable();

    auto ctr = mem_holder_->ctr();
    ShortTypeCode mytag = arena::read_type_tag(array_);
    ArrayStorageT* new_array = array_->remove(*ctr->arena_, mytag, idx);
    return ObjectArray{ObjectArrayView{mem_holder_, new_array}};
}


inline PoolSharedPtr<GenericArray> ArrayView<Object>::as_generic_array() const {
    return TypedGenericArray<Object>::make_wrapper(mem_holder_, array_);
}


template <typename DT>
PoolSharedPtr<GenericArray> TypedGenericArray<DT>::make_wrapper(LWMemHolder* ctr_holder, void* array) {
    using GAPoolT = pool::SimpleObjectPool<TypedGenericArray<DT>>;
    using GAPoolPtrT = boost::local_shared_ptr<GAPoolT>;

    static thread_local GAPoolPtrT wrapper_pool = MakeLocalShared<GAPoolT>();
    return wrapper_pool->allocate_shared(ctr_holder, array);
}


}}
