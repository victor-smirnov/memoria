
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
#include <memoria/core/hermes/array/object_array.hpp>
#include <memoria/core/hermes/container.hpp>
#include <memoria/core/hermes/map/object_map.hpp>

namespace memoria {
namespace hermes {



inline void DatatypeView::assert_mutable()
{
    if (MMA_UNLIKELY(!mem_holder_->is_mem_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("DatatypeView is immutable").do_throw();
    }
}



inline StringOView DatatypeView::type_name() const {
    assert_not_null();
    return datatype_->name()->view(mem_holder_);
}

template <typename DT>
void DatatypeView::append_integral_parameter(DTTViewType<DT> view)
{
    static_assert (
        std::is_same_v<DT, BigInt>  ||
        std::is_same_v<DT, UBigInt> ||
        std::is_same_v<DT, Integer>  ||
        std::is_same_v<DT, Boolean>,
    "");

    assert_not_null();
    assert_mutable();

    ObjectArray params = type_parameters();

    if (MMA_UNLIKELY(params.is_null())) {
        auto ctr = HermesCtr(mem_holder_);
        params = ctr.make_object_array();
        datatype_->set_parameters(params.array_);
    }

    auto new_params = params.push_back<DT>(view);
    datatype_->set_parameters(new_params.array_);
}

inline HermesCtr DatatypeView::ctr() const {
    assert_not_null();
    return HermesCtr(get_mem_holder());
}


}}
