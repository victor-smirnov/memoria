
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

#include <memoria/core/hermes/hermes.hpp>

namespace memoria::hermes {

GenericArrayPtr ObjectView::as_generic_array() const
{
    if (get_vs_tag() == VS_TAG_ADDRESS)
    {
        auto tag = get_type_tag();
        auto ctr_ptr = get_type_reflection(tag).hermes_make_wrapper(get_mem_holder(), storage_.addr);
        return ctr_ptr->as_array();
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Object is not addressable").do_throw();
    }
}


GenericMapPtr ObjectView::as_generic_map() const
{
    if (get_vs_tag() == VS_TAG_ADDRESS)
    {
        auto tag = get_type_tag();
        auto ctr_ptr = get_type_reflection(tag).hermes_make_wrapper(get_mem_holder(), storage_.addr);
        return ctr_ptr->as_map();
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Object is not addressable").do_throw();
    }
}

TypedValue ObjectView::as_typed_value() const {
    return cast_to<TypedValue>();
}

Datatype ObjectView::as_datatype() const {
    return cast_to<Datatype>();
}

DTView<Varchar> ObjectView::as_varchar() const {
    return cast_to<Varchar>();
}

DTView<Double> ObjectView::as_double() const {
    return cast_to<Double>();
}

DTView<BigInt> ObjectView::as_bigint() const {
    return cast_to<BigInt>();
}

DTView<Boolean> ObjectView::as_boolean() const {
    return cast_to<Boolean>();
}

DTView<Real> ObjectView::as_real() const {
    return cast_to<Real>();
}

int64_t ObjectView::to_i64() const {
    return convert_to<BigInt>().as_data_object<BigInt>();
}

int64_t ObjectView::to_i32() const {
    return convert_to<Integer>().as_data_object<Integer>();
}

U8String ObjectView::to_str() const {
    return convert_to<Varchar>().as_data_object<Varchar>();
}

bool ObjectView::to_bool() const {
    return convert_to<Boolean>().as_data_object<Boolean>();
}

double ObjectView::to_d64() const {
    return convert_to<Double>().as_data_object<Double>();
}

float ObjectView::to_f32() const {
    return convert_to<Real>().as_data_object<Real>();
}


U8String ObjectView::type_str() const {
    assert_not_null();
    auto tag = get_type_tag();
    return get_type_reflection(tag).str();
}




ObjectMap ObjectView::as_object_map() const {
    return cast_to<Map<Varchar, Object>>();
}

TinyObjectMap ObjectView::as_tiny_object_map() const {
    return cast_to<Map<UTinyInt, Object>>();
}

ObjectArray ObjectView::as_object_array() const {
    return cast_to<Array<Object>>();
}

PoolSharedPtr<HermesCtr> ObjectView::clone(bool make_immutable) const
{
    auto ctr = HermesCtr::make_new();

    if (is_not_null())
    {
        Object vv = ctr->do_import_value(Object(get_mem_holder(), get_vs_tag(), storage_));
        ctr->set_root(vv);
    }

    return ctr;
}


}
