
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

#include <memoria/core/hermes/container_ext.hpp>
#include "hermes_internal.hpp"


namespace memoria {
namespace hermes {

pool::SharedPtr<HermesCtr> HermesCtr::self() const
{
    return pool::SharedPtr<HermesCtr>{mutable_self(), get_ptr_holder()->owner(), pool::DoRef{}};
}


void HermesCtr::deep_copy_from(const DocumentHeader* src, DeepCopyDeduplicator& dedup) {
    header_ = src->deep_copy_to(*arena_, this, ptr_holder_, dedup);
}

pool::SharedPtr<HermesCtr> HermesCtr::compactify(bool make_immutable) const
{
    assert_not_null();

    if (MMA_UNLIKELY(!arena_)) {
        return self();
    }

    arena::AllocationType alc_type = make_immutable ? arena::AllocationType::GROWABLE_SINGLE_CHUNK :
                                                      arena::AllocationType::MULTI_CHUNK;
    auto doc = TL_allocate_shared<HermesCtrImpl>(arena_->chunk_size(), alc_type);

    DeepCopyDeduplicator dedup;
    doc->deep_copy_from(header_, dedup);

    return doc;
}

pool::SharedPtr<HermesCtr> HermesCtr::clone(bool as_mutable) const
{
    assert_not_null();
    if (arena_)
    {
        if (arena_->chunks() > 1 || !as_mutable)
        {
            size_t chunk_size = arena_->chunk_size();
            auto doc = TL_allocate_shared<HermesCtrImpl>(chunk_size, arena::AllocationType::GROWABLE_SINGLE_CHUNK);

            DeepCopyDeduplicator dedup;
            doc->deep_copy_from(header_, dedup);

            if (as_mutable) {
                doc->arena_.switch_to_chunked_mode();
            }

            return doc;
        }
        else {
            auto& head = arena_->head();
            auto doc = TL_allocate_shared<HermesCtrImpl>(
                        arena::AllocationType::GROWABLE_SINGLE_CHUNK,
                        arena_->chunk_size(), head.memory.get(), head.size);

            if (as_mutable) {
                doc->arena_.switch_to_chunked_mode();
            }

            return doc;
        }
    }
    else {
        arena::AllocationType alc_type = as_mutable ? arena::AllocationType::MULTI_CHUNK :
                                                      arena::AllocationType::GROWABLE_SINGLE_CHUNK;

        return TL_allocate_shared<HermesCtrImpl>(alc_type, 4096, header_, segment_size_);
    }
}


GenericMapPtr HermesCtr::new_map()
{
    using CtrT = Map<Varchar, Value>;

    auto arena_dtc = arena()->allocate_tagged_object<typename CtrT::ArenaMap>(
        TypeHashV<CtrT>
    );

    return GenericMapPtr(CtrT(arena_dtc, this, ptr_holder_));
}

GenericArrayPtr HermesCtr::new_array()
{
    using CtrT = GenericArray;

    auto arena_dtc = arena()->allocate_tagged_object<typename CtrT::ArenaArray>(
        TypeHashV<CtrT>
    );

    return GenericArrayPtr(CtrT(arena_dtc, this, ptr_holder_));
}

GenericArrayPtr HermesCtr::new_array(Span<ValuePtr> span)
{
    using CtrT = GenericArray;

    auto arena_arr = arena()->allocate_tagged_object<typename CtrT::ArenaArray>(
        TypeHashV<CtrT>
    );

    if (span.size())
    {
        arena_arr->enlarge(*arena_, span.size());
        for (auto& value: span) {
            arena_arr->push_back(*arena_, value->value_storage_.addr);
        }
    }

    return GenericArrayPtr(CtrT(arena_arr, this, ptr_holder_));
}


void HermesCtr::set_value(ValuePtr value) {
    header_->root = value->value_storage_.addr;
}

DatatypePtr HermesCtr::new_datatype(U8StringView name)
{
    auto str = new_dataobject<Varchar>(name);
    auto arena_dt = arena()->allocate_tagged_object<detail::DatatypeData>(
        TypeHashV<Datatype>, str->dt_ctr()
    );

    return DatatypePtr(Datatype(arena_dt, this, ptr_holder_));
}

DatatypePtr HermesCtr::new_datatype(StringValuePtr name)
{
    auto arena_dt = arena()->allocate_tagged_object<detail::DatatypeData>(
        TypeHashV<Datatype>, name->dt_ctr()
    );

    return DatatypePtr(Datatype(arena_dt, this, ptr_holder_));
}

TypedValuePtr HermesCtr::new_typed_value(DatatypePtr datatype, ValuePtr constructor)
{
    auto arena_tv = arena()->allocate_tagged_object<detail::TypedValueData>(
        TypeHashV<TypedValue>, datatype->datatype_, constructor->value_storage_.addr
    );

    return TypedValuePtr(TypedValue(arena_tv, this, ptr_holder_));
}

ValuePtr HermesCtr::do_import_value(ValuePtr value)
{
    assert_not_null();
    assert_mutable();

    if (!value->is_null())
    {
        auto tag = arena::read_type_tag(value->value_storage_.addr);

        DeepCopyDeduplicator dedup;
        auto addr = get_type_reflection(tag).deep_copy_to(*arena_, value->value_storage_.addr, this, ptr_holder_, dedup);

        return ValuePtr(Value(addr, this, ptr_holder_));
    }
    else {
        return value;
    }
}

ParameterPtr HermesCtr::new_parameter(U8StringView name) {
    assert_not_null();
    assert_mutable();

    auto arena_dtc = arena_->allocate_tagged_object<typename Parameter::ArenaDTContainer>(
        TypeHashV<Parameter>,
        name
    );

    return ParameterPtr(Parameter(arena_dtc, this, ptr_holder_));
}




//PoolSharedPtr<HermesCtr> HermesCtr::make_ctr() {

//}


}}
