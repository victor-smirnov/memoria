
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

#include <memoria/core/arena/relative_ptr.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/core/hermes/array/object_array.hpp>
#include <memoria/core/hermes/map/object_map.hpp>
#include <memoria/core/hermes/map/typed_map.hpp>
#include <memoria/core/hermes/datatype.hpp>

#include <algorithm>
#include <type_traits>

namespace memoria::hermes {

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

template <typename T>
constexpr ObjectCasters ObjectCasterFor = DataTypeTraits<T>::isDataType ?
            ObjectCasters::DATAOBJECT :
            ObjectCasters::OTHER;

template <typename DT>
struct ObjectCaster<DT, ObjectCasters::DATAOBJECT> {

    static DTView<DT> cast_to(
            LWMemHolder* mem_holder,
            ValueStorageTag vs_tag,
            ValueStorage& storage
    ) {
        if (vs_tag == VS_TAG_ADDRESS)
        {
            auto* dt_ctr = reinterpret_cast<arena::ArenaDataTypeContainer<DT>*>(storage.addr);
            return dt_ctr->view(mem_holder);
        }
        else if (vs_tag == VS_TAG_SMALL_VALUE) {
            return DTView<DT>(mem_holder, storage.small_value.get_unchecked<DTTViewType<DT>>());
        }
        else {
            if (storage.view_ptr) {
                return DTView<DT>(mem_holder, storage.view_ptr->get<DTTViewType<DT>>());
            }
            else {
                return DTView<DT>{};
            }
        }
    }
};



template <typename DT>
struct ObjectCaster<DataObject<DT>, ObjectCasters::OTHER> {

    static DTView<DT> cast_to(
            LWMemHolder* mem_holder,
            ValueStorageTag vs_tag,
            ValueStorage& storage
    ) {
        if (vs_tag == VS_TAG_ADDRESS)
        {
            auto* dt_ctr = reinterpret_cast<arena::ArenaDataTypeContainer<DT>*>(storage.addr);
            return dt_ctr->view(mem_holder);
        }
        else if (vs_tag == VS_TAG_SMALL_VALUE) {
            return DTView<DT>(mem_holder, storage.small_value.get_unchecked<DTTViewType<DT>>());
        }
        else {
            if (storage.view_ptr) {
                return DTView<DT>(mem_holder, storage.view_ptr->get<DTTViewType<DT>>());
            }
            else {
                return DTView<DT>{};
            }
        }
    }
};



template <typename T>
struct ObjectCaster<T, ObjectCasters::OTHER> {

    static auto cast_to(
            LWMemHolder* mem_holder,
            ValueStorageTag vs_tag,
            ValueStorage& storage
    ) {
        if (MMA_LIKELY(vs_tag == VS_TAG_ADDRESS)) {
            return T(mem_holder, storage.addr);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Incorrect value storage tag {} for type {}", vs_tag, type_to_str<T>()).do_throw();
        }
    }
};

template <typename T>
auto ObjectView::cast_to(TypeTag<T>) const
{
    assert_not_null();
    auto tag = ShortTypeCode::of<T>();
    auto value_tag = get_type_tag();

    if (value_tag == tag) {
        return ObjectCaster<T, ObjectCasterFor<T>>::cast_to(
            get_mem_holder(),
            get_vs_tag(),
            storage_
        );
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Invalid value type tag: expected {}, actual {}", tag.u64(), value_tag.u64()).do_throw();
    }
}

}
