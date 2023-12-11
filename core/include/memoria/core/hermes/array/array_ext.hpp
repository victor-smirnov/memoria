
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
#include <memoria/core/hermes/map/object_map.hpp>

namespace memoria {
namespace hermes {


inline void ArrayView<Object>::assert_mutable()
{
    if (MMA_UNLIKELY(!get_mem_holder()->is_mem_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("ArrayView<Object> is immutable").do_throw();
    }
}


template <typename DT>
void ArrayView<DT>::assert_mutable()
{
    if (MMA_UNLIKELY(!this->get_mem_holder()->is_mem_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("ArrayView<DT> is immutable").do_throw();
    }
}


inline MaybeObject ArrayView<Object>::get(uint64_t idx) const
{
    assert_not_null();

    if (idx < array_->size())
    {
        const auto& ptr = array_->get(idx);
        if (MMA_LIKELY(ptr.is_pointer()))
        {
            if (MMA_LIKELY(ptr.is_not_null())) {
                return Object(mem_holder_, ptr.get());
            }
            else {
                return {};
            }
        }
        else {
            TaggedValue tv(ptr);
            return Object(mem_holder_, tv);
        }
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Range check in ArrayView<Object>: {} {}", idx, array_->size()).do_throw();
    }
}


inline void ArrayView<Object>::for_each(std::function<void(const MaybeObject&)> fn) const {
    assert_not_null();

    for (auto& vv: array_->span()) {
        if (vv.is_pointer())
        {
            if (vv.is_not_null()) {
                fn(Object(mem_holder_, vv.get()));
            }
            else {
                fn(MaybeObject());
            }
        }
        else {
            TaggedValue tv(vv);
            fn(Object(mem_holder_, tv));
        }
    }
}


template <typename T, std::enable_if_t<!HermesObject<std::decay_t<T>>::Value, int>>
void ArrayView<Object>::push_back(T&& view)
{
    assert_not_null();
    assert_mutable();

    auto ctr = HermesCtr(mem_holder_);
    auto vv  = ctr.make(std::forward<T>(view));
    return push_back(vv);
}


template <typename DT, typename T>
void ArrayView<Object>::push_back_t(T&& view)
{
    assert_not_null();
    assert_mutable();

    auto ctr = HermesCtr(mem_holder_);
    auto vv  = ctr.make_t<DT>(std::forward<T>(view));
    return push_back(vv);
}



template <typename T, std::enable_if_t<!HermesObject<std::decay_t<T>>::Value, int>>
Object ArrayView<Object>::set(uint64_t idx, T&& view)
{    
    assert_not_null();
    assert_mutable();
    auto ctr = HermesCtr(mem_holder_);
    auto ptr = ctr.make(std::forward<T>(view));
    set(idx, ptr);

    return ptr;
}


template <typename DT, typename T>
Object ArrayView<Object>::set_t(uint64_t idx, T&& view)
{
    assert_not_null();
    assert_mutable();
    auto ctr = HermesCtr(mem_holder_);
    auto ptr = ctr.make_t<DT>(std::forward<T>(view));

    set(idx, ptr);

    return ptr;
}


inline void ArrayView<Object>::push_back(const MaybeObject& value)
{
    assert_not_null();
    assert_mutable();

    auto ctr = HermesCtr(mem_holder_);
    if (value)
    {
        arena::ERelativePtr val_ptr;
        if (value->get_vs_tag() == VS_TAG_SMALL_VALUE)
        {
            ShortTypeCode tag = value->get_type_tag();
            bool do_import = !get_type_reflection(tag).hermes_embed(val_ptr, value->storage_.small_value);
            if (do_import)
            {
                auto vv = ctr.do_import_value(value);
                val_ptr = vv.storage_.addr;
            }
        }
        else {
            auto vv = ctr.do_import_value(value);
            val_ptr = vv.storage_.addr;
        }

        array_->push_back(*ctr.arena(), val_ptr);
    }
    else {
        array_->push_back(*ctr.arena(), nullptr);
    }
}



template <typename DT>
void ArrayView<DT>::push_back(DTTViewType<DT> value)
{
    assert_not_null();
    assert_mutable();

    auto ctr = HermesCtr(mem_holder_);
    array_->push_back(*ctr.arena(), value);
}

template <typename DT>
void ArrayView<DT>::set(uint64_t idx, DTTViewType<DT> value)
{
    assert_not_null();
    assert_mutable();
    array_->set(idx, value);
}


inline MaybeObject ArrayView<Object>::set(uint64_t idx, const MaybeObject& ivalue)
{
    assert_not_null();
    assert_mutable();
    auto ctr = HermesCtr(mem_holder_);

    if (ivalue)
    {
        auto value = ctr.do_import_value(*ivalue);

        arena::ERelativePtr val_ptr;

        if (value.get_vs_tag() == VS_TAG_SMALL_VALUE)
        {
            ShortTypeCode tag = value.get_type_tag();
            bool do_import = !get_type_reflection(tag).hermes_embed(val_ptr, value.storage_.small_value);
            if (do_import)
            {
                auto vv = ctr.do_import_value(value);
                val_ptr = vv.storage_.addr;
            }
        }
        else {
            auto vv = ctr.do_import_value(value);
            val_ptr = vv.storage_.addr;
        }

        array_->set(idx, val_ptr);
        return value;
    }
    else {
        array_->set(idx, nullptr);
        return {};
    }
}



inline void ArrayView<Object>::remove(uint64_t idx)
{
    assert_not_null();
    assert_mutable();

    auto ctr = HermesCtr(mem_holder_);
    array_->remove(*ctr.arena(), idx);
}

template <typename DT>
void ArrayView<DT>::remove(uint64_t idx)
{
    assert_not_null();
    assert_mutable();

    auto ctr = HermesCtr(mem_holder_);
    array_->remove(*ctr.arena(), idx);
}


inline PoolSharedPtr<GenericArray> ArrayView<Object>::as_generic_array() const {
    return TypedGenericArray<Object>::make_wrapper(self());
}


template <typename DT>
PoolSharedPtr<GenericArray> TypedGenericArray<DT>::make_wrapper(Array<DT>&& array) {
    using GAPoolT = pool::SimpleObjectPool<TypedGenericArray<DT>>;
    using GAPoolPtrT = boost::local_shared_ptr<GAPoolT>;

    static thread_local GAPoolPtrT wrapper_pool = MakeLocalShared<GAPoolT>();
    return wrapper_pool->allocate_shared(std::move(array));
}


template <typename DT>
void ArrayView<DT>::do_stringify(std::ostream& out, DumpFormatState& state) const
{
    auto& spec = state.cfg().spec();

    out << "<";
    out << get_datatype_name(type_to_str<DT>());
    out << ">" << spec.space();

    if (size() > 0)
    {
        out << "[" << spec.nl_start();

        bool first = true;

        state.push();
        for_each([&](auto vv){
            if (MMA_LIKELY(!first)) {
                out << "," << spec.nl_middle();
            }
            else {
                first = false;
            }

            state.make_indent(out);
            arena::ArenaDataTypeContainer<DT>::stringify_view(out, state, vv);
        });
        state.pop();

        out << spec.nl_end();

        state.make_indent(out);
        out << "]";
    }
    else {
        out << "[]";
    }
}


template <typename DT>
HermesCtr ArrayView<DT>::ctr() const {
    assert_not_null();
    return HermesCtr(mem_holder_);
}

template <typename DT>
HermesCtr TypedGenericArray<DT>::ctr() const {
    return array_.ctr();
}


}}
