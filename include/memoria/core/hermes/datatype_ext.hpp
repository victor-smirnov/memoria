
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

#include <memoria/core/hermes/object.hpp>
#include <memoria/core/hermes/array/array.hpp>
#include <memoria/core/hermes/container.hpp>
#include <memoria/core/hermes/data_object.hpp>
#include <memoria/core/hermes/map/map.hpp>

namespace memoria {
namespace hermes {



inline void Datatype::assert_mutable()
{
    auto ctr = ptr_holder_->ctr();
    if (MMA_UNLIKELY(!ctr->is_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("Datatype is immutable").do_throw();
    }
}



inline StringValuePtr Datatype::type_name() const {
    assert_not_null();
    return StringValuePtr(StringValue(ptr_holder_, datatype_->name()));
}

template <typename DT>
DataObjectPtr<DT> Datatype::append_integral_parameter(DTTViewType<DT> view)
{
    static_assert (
        std::is_same_v<DT, BigInt>  ||
        std::is_same_v<DT, UBigInt> ||
        std::is_same_v<DT, Boolean>,
    "");

    assert_not_null();
    assert_mutable();

    ObjectArrayPtr params = type_parameters();

    if (MMA_UNLIKELY(params->is_null())) {
        auto ctr = ptr_holder_->ctr();
        params = ctr->new_array();
        datatype_->set_parameters(params->array_);
    }

    return params->append(view);
}


}}
