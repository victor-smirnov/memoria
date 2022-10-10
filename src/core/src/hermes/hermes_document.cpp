
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

#include <memoria/core/hermes/document_ext.hpp>

namespace memoria {
namespace hermes {



GenericMapPtr HermesDocView::new_map()
{
    using CtrT = Map<Varchar, Value>;

    auto arena_dtc = arena()->allocate_tagged_object<typename CtrT::ArenaMap>(
        TypeHashV<CtrT>
    );

    return GenericMapPtr(CtrT(arena_dtc, this, ptr_holder_));
}

GenericArrayPtr HermesDocView::new_array()
{
    using CtrT = GenericArray;

    auto arena_dtc = arena()->allocate_tagged_object<typename CtrT::ArenaArray>(
        TypeHashV<CtrT>
    );

    return GenericArrayPtr(CtrT(arena_dtc, this, ptr_holder_));
}

GenericArrayPtr HermesDocView::new_array(Span<ValuePtr> span)
{
    using CtrT = GenericArray;

    auto arena_arr = arena()->allocate_tagged_object<typename CtrT::ArenaArray>(
        TypeHashV<CtrT>
    );

    if (span.size())
    {
        arena_arr->enlarge(*arena_, span.size());
        for (auto& value: span) {
            arena_arr->push_back(*arena_, value->addr_);
        }
    }

    return GenericArrayPtr(CtrT(arena_arr, this, ptr_holder_));
}


void HermesDocView::set_value(ValuePtr value) {
    header_->value = value->addr_;
}

DatatypePtr HermesDocView::new_datatype(U8StringView name)
{
    auto str = new_dataobject<Varchar>(name);
    auto arena_dt = arena()->allocate_tagged_object<detail::DatatypeData>(
        TypeHashV<Datatype>, str->dt_ctr_
    );

    return DatatypePtr(Datatype(arena_dt, this, ptr_holder_));
}

DatatypePtr HermesDocView::new_datatype(StringValuePtr name)
{
    auto arena_dt = arena()->allocate_tagged_object<detail::DatatypeData>(
        TypeHashV<Datatype>, name->dt_ctr_
    );

    return DatatypePtr(Datatype(arena_dt, this, ptr_holder_));
}

TypedValuePtr HermesDocView::new_typed_value(DatatypePtr datatype, ValuePtr constructor)
{
    auto arena_tv = arena()->allocate_tagged_object<detail::TypedValueData>(
        TypeHashV<TypedValue>, datatype->datatype_, constructor->addr_
    );

    return TypedValuePtr(TypedValue(arena_tv, this, ptr_holder_));
}

}}
