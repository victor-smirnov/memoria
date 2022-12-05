
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
    header_ = src->deep_copy_to(*arena_, ptr_holder_, dedup);
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

    return ObjectMapPtr(CtrT(ptr_holder_, arena_dtc));
}

TinyObjectMapPtr HermesCtr::new_tiny_map(size_t capacity) {
    using CtrT = Map<UTinyInt, Object>;

    auto arena_dtc = arena()->allocate_tagged_object<typename CtrT::MapStorageT>(
        ShortTypeCode::of<CtrT>(), capacity > 0 ? capacity : 1
    );

    return TinyObjectMapPtr(CtrT(ptr_holder_, arena_dtc));
}

ObjectArrayPtr HermesCtr::new_array(uint64_t capacity)
{
    using CtrT = ObjectArray;

    auto arena_dtc = arena()->allocate_tagged_object<typename CtrT::ArrayStorageT>(
        ShortTypeCode::of<CtrT>(), capacity
    );

    return ObjectArrayPtr(CtrT(ptr_holder_, arena_dtc));
}

ObjectArrayPtr HermesCtr::new_array(Span<const ObjectPtr> span)
{
    auto array = new_array(span.size());

    for (auto& value: span) {
        auto vv = this->do_import_embeddable(value);
        array = array->append(vv);
    }

    return array;
}

ObjectArrayPtr HermesCtr::new_array(const std::vector<ObjectPtr>& array) {
    return new_array(Span<const ObjectPtr>(array.data(), array.size()));
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

    return DatatypePtr(Datatype(ptr_holder_, arena_dt));
}

DatatypePtr HermesCtr::new_datatype(StringValuePtr name)
{
    auto arena_dt = arena()->allocate_tagged_object<detail::DatatypeData>(
        ShortTypeCode::of<Datatype>(), name->dt_ctr()
    );

    return DatatypePtr(Datatype(ptr_holder_, arena_dt));
}

TypedValuePtr HermesCtr::new_typed_value(DatatypePtr datatype, ObjectPtr constructor)
{
    ObjectPtr vv_ctr = do_import_value(constructor);

    auto arena_tv = arena()->allocate_tagged_object<detail::TypedValueData>(
        ShortTypeCode::of<TypedValue>(), datatype->datatype_, vv_ctr->storage_.addr
    );

    return TypedValuePtr(TypedValue(ptr_holder_, arena_tv));
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
                auto addr = get_type_reflection(tag).deep_copy_to(*arena_, ptr_holder_, value->storage_.addr, dedup);

                return ObjectPtr(Object(ptr_holder_, addr));
            }
            else {
                auto type_tag = value->get_type_tag();
                auto vs_tag = value->get_vs_tag();
                return get_type_reflection(type_tag).import_value(ptr_holder_, vs_tag, value->storage_);
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


ObjectPtr HermesCtr::do_import_embeddable(ObjectPtr value)
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
                auto addr = get_type_reflection(tag).deep_copy_to(*arena_, ptr_holder_, value->storage_.addr, dedup);

                return ObjectPtr(Object(ptr_holder_, addr));
            }
            else if (value->get_vs_tag() == ValueStorageTag::VS_TAG_SMALL_VALUE)
            {
                auto type_tag = value->get_type_tag();
                auto& refl = get_type_reflection(type_tag);

                if (refl.hermes_is_ptr_embeddable()) {
                    return value;
                }
                else {
                    auto vs_tag = value->get_vs_tag();
                    return refl.import_value(ptr_holder_, vs_tag, value->storage_);
                }
            }
            else {
                auto type_tag = value->get_type_tag();
                auto vs_tag = value->get_vs_tag();
                return get_type_reflection(type_tag).import_value(ptr_holder_, vs_tag, value->storage_);
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

    return ParameterPtr(Parameter(ptr_holder_, arena_dtc));
}


struct CommonInstance {
    PoolSharedPtr<HermesCtr> ctr;
    CommonInstance()
    {
        ctr = HermesCtr::make_pooled();
        auto str = ctr->new_dataobject<Varchar>("Dedicated HermesCtr instance for 8-byte data objects");
        ctr->set_root(str->as_object());
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
