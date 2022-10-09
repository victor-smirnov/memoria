
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


#include <memoria/core/hermes/document.hpp>
#include <memoria/core/hermes/map.hpp>
#include <memoria/core/hermes/array.hpp>
#include <memoria/core/hermes/data_object.hpp>

namespace memoria {
namespace hermes {

inline void HermesDocView::assert_mutable()
{
    if (MMA_UNLIKELY(this->is_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("Map<String, Value> is immutable");
    }
}

template <typename DT>
DataObjectPtr<DT> HermesDocView::new_dataobject(DTTViewType<DT> view)
{
    using DTCtr = DataObject<DT>;

    auto arena_dtc = arena_->allocate_tagged_object<typename DTCtr::ArenaDTContainer>(
        TypeHashV<DTCtr>,
        view
    );

    return DataObjectPtr<DT>(DTCtr(arena_dtc, this, ptr_holder_));
}



}}
