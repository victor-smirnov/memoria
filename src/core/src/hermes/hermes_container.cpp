
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


ObjectMapPtr HermesCtr::new_map()
{
    using CtrT = Map<Varchar, Object>;

    auto arena_dtc = arena()->allocate_tagged_object<typename CtrT::MapStorageT>(
        ShortTypeCode::of<CtrT>()
    );

    return ObjectMapPtr(CtrT(arena_dtc, this, ptr_holder_));
}

ObjectArrayPtr HermesCtr::new_array()
{
    using CtrT = ObjectArray;

    auto arena_dtc = arena()->allocate_tagged_object<typename CtrT::ArrayStorageT>(
        ShortTypeCode::of<CtrT>()
    );

    return ObjectArrayPtr(CtrT(arena_dtc, this, ptr_holder_));
}

ObjectArrayPtr HermesCtr::new_array(Span<ObjectPtr> span)
{
    using CtrT = ObjectArray;

    auto arena_arr = arena()->allocate_tagged_object<typename CtrT::ArrayStorageT>(
        ShortTypeCode::of<CtrT>()
    );

    if (span.size())
    {
        arena_arr->enlarge(*arena_, span.size());
        for (auto& value: span) {
            arena_arr->push_back(*arena_, value->storage_.addr);
        }
    }

    return ObjectArrayPtr(CtrT(arena_arr, this, ptr_holder_));
}


void HermesCtr::set_root(ObjectPtr value)
{
    ObjectPtr vv = do_import_value(value);
    header_->root = vv->storage_.addr;
}

DatatypePtr HermesCtr::new_datatype(U8StringView name)
{
    auto str = new_dataobject<Varchar>(name);
    auto arena_dt = arena()->allocate_tagged_object<detail::DatatypeData>(
        ShortTypeCode::of<Datatype>(), str->dt_ctr()
    );

    return DatatypePtr(Datatype(arena_dt, this, ptr_holder_));
}

DatatypePtr HermesCtr::new_datatype(StringValuePtr name)
{
    auto arena_dt = arena()->allocate_tagged_object<detail::DatatypeData>(
        ShortTypeCode::of<Datatype>(), name->dt_ctr()
    );

    return DatatypePtr(Datatype(arena_dt, this, ptr_holder_));
}

TypedValuePtr HermesCtr::new_typed_value(DatatypePtr datatype, ObjectPtr constructor)
{
    auto arena_tv = arena()->allocate_tagged_object<detail::TypedValueData>(
        ShortTypeCode::of<TypedValue>(), datatype->datatype_, constructor->storage_.addr
    );

    return TypedValuePtr(TypedValue(arena_tv, this, ptr_holder_));
}

ObjectPtr HermesCtr::do_import_value(ObjectPtr value)
{
    assert_not_null();
    assert_mutable();

    if (!value->is_null())
    {
        if (value->document().get() != this)
        {
            if (value->get_vs_tag() == ValueStorageTag::VS_TAG_ADDRESS)
            {
                auto tag = arena::read_type_tag(value->storage_.addr);

                DeepCopyDeduplicator dedup;
                auto addr = get_type_reflection(tag).deep_copy_to(*arena_, value->storage_.addr, this, ptr_holder_, dedup);

                return ObjectPtr(Object(addr, this, ptr_holder_));
            }
            else {
                auto type_tag = value->get_type_tag();
                auto vs_tag = value->get_vs_tag();
                return get_type_reflection(type_tag).import_value(vs_tag, value->storage_, this, ptr_holder_);
            }
        }
        else {
            return value;
        }
    }
    else {
        return value;
    }
}

ParameterPtr HermesCtr::new_parameter(U8StringView name) {
    assert_not_null();
    assert_mutable();

    auto arena_dtc = arena_->allocate_tagged_object<typename Parameter::ArenaDTContainer>(
        ShortTypeCode::of<Parameter>(),
        name
    );

    return ParameterPtr(Parameter(arena_dtc, this, ptr_holder_));
}


struct CommonInstance {
    PoolSharedPtr<HermesCtr> ctr;
    CommonInstance() {
        ctr = HermesCtr::wrap_dataobject<Varchar>("Dedicated HermesCtr instance for 8-byte data objects")->document();
    }
};



PoolSharedPtr<HermesCtr> HermesCtr::common_instance() {
    static thread_local CommonInstance instance;
    return instance.ctr;
}


hermes::DatatypePtr strip_namespaces(hermes::DatatypePtr src)
{
    auto ctr = HermesCtr::make_pooled();
    auto name = get_datatype_name(*src->type_name()->view());

    auto tgt = ctr->new_datatype(name);
    ctr->set_root(tgt->as_object());

    auto params = src->type_parameters();
    if (params->is_not_null())
    {
        for (size_t c = 0; c < params->size(); c++) {
            auto param = params->get(c);
            if (param->is_datatype()) {
                auto pp = strip_namespaces(param->as_datatype());
                tgt->append_type_parameter(pp->as_object());
            }
            else {
                tgt->append_type_parameter(param);
            }
        }
    }

    return tgt;
}


}}
