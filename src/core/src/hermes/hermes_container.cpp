
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
    return pool::SharedPtr<HermesCtr>{mutable_self(), get_mem_holder()->owner(), pool::DoRef{}};
}


void HermesCtr::deep_copy_from(const DocumentHeader* src, DeepCopyDeduplicator& dedup) {
    header_ = src->deep_copy_to(*arena_, mem_holder_, dedup);
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



void HermesCtr::set_root(Object value)
{
    Object vv = do_import_value(value);
    header_->root = vv->storage_.addr;
}

Datatype HermesCtr::new_datatype(U8StringView name)
{
    auto str = new_dataobject<Varchar>(name);
    auto arena_dt = arena()->allocate_tagged_object<detail::DatatypeData>(
        ShortTypeCode::of<Datatype>(),
                reinterpret_cast<arena::ArenaDataTypeContainer<Varchar>*>(str->addr())
    );

    return Datatype(mem_holder_, arena_dt);
}

//Datatype HermesCtr::new_datatype(StringValue name)
//{
//    auto arena_dt = arena()->allocate_tagged_object<detail::DatatypeData>(
//        ShortTypeCode::of<Datatype>(), name->dt_ctr()
//    );

//    return Datatype(mem_holder_, arena_dt);
//}

TypedValue HermesCtr::new_typed_value(Datatype datatype, Object constructor)
{
    return make_typed_value(datatype, constructor);
}

Object HermesCtr::do_import_value(Object value)
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
                auto addr = get_type_reflection(tag).deep_copy_to(*arena_, mem_holder_, value->storage_.addr, dedup);

                return Object(ObjectView(mem_holder_, addr));
            }
            else {
                auto type_tag = value->get_type_tag();
                auto vs_tag = value->get_vs_tag();
                return get_type_reflection(type_tag).import_value(mem_holder_, vs_tag, value->storage_);
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


Object HermesCtr::do_import_embeddable(Object value)
{
    return import_object(value);
}

Object HermesCtr::import_object(const Object& object)
{
    assert_not_null();
    assert_mutable();

    if (!object.is_null())
    {
        if (object.get_mem_holder() != mem_holder_)
        {
            if (object.get_vs_tag() == ValueStorageTag::VS_TAG_ADDRESS)
            {
                auto tag = arena::read_type_tag(object.storage_.addr);

                DeepCopyDeduplicator dedup;
                auto addr = get_type_reflection(tag).deep_copy_to(*arena_, mem_holder_, object.storage_.addr, dedup);

                return Object(ObjectView(mem_holder_, addr));
            }
            else if (object.get_vs_tag() == ValueStorageTag::VS_TAG_SMALL_VALUE)
            {
                auto type_tag = object.get_type_tag();
                auto& refl = get_type_reflection(type_tag);

                if (refl.hermes_is_ptr_embeddable()) {
                    return object;
                }
                else {
                    auto vs_tag = object.get_vs_tag();
                    return refl.import_value(mem_holder_, vs_tag, object.storage_);
                }
            }
            else {
                auto type_tag = object.get_type_tag();
                auto vs_tag = object.get_vs_tag();
                return get_type_reflection(type_tag).import_value(mem_holder_, vs_tag, object.storage_);
            }
        }
        else {
            return object;
        }
    }
    else {
        return object;
    }
}


Parameter HermesCtr::new_parameter(U8StringView name)
{
    assert_not_null();
    assert_mutable();

    auto arena_dtc = arena_->allocate_tagged_object<typename ParameterView::ArenaDTContainer>(
        ShortTypeCode::of<Parameter>(),
        name
    );

    return Parameter(mem_holder_, arena_dtc);
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


ObjectArray HermesCtr::make_object_array(uint64_t capacity) {
    return HermesCtr::make_array<Object>(capacity);
}


Parameter HermesCtr::make_parameter(const U8StringView& name)
{
    assert_not_null();
    assert_mutable();

    auto arena_dtc = arena_->allocate_tagged_object<typename ParameterView::ArenaDTContainer>(
        ShortTypeCode::of<Parameter>(),
        name
    );

    return Parameter(mem_holder_, arena_dtc);
}



Datatype HermesCtr::make_datatype(const U8StringView& name)
{
    assert_not_null();
    assert_mutable();

    Object nameo = make(name);

    auto arena_dt = arena()->allocate_tagged_object<detail::DatatypeData>(
        ShortTypeCode::of<Datatype>(),
                reinterpret_cast<arena::ArenaDataTypeContainer<Varchar>*>(nameo->addr())
    );

    return Datatype(mem_holder_, arena_dt);
}

Datatype HermesCtr::make_datatype(const StringOView& name)
{
    assert_not_null();
    assert_mutable();

    Object nameo = import_object(name);

    auto arena_dt = arena()->allocate_tagged_object<detail::DatatypeData>(
        ShortTypeCode::of<Datatype>(),
                reinterpret_cast<arena::ArenaDataTypeContainer<Varchar>*>(nameo->addr())
    );

    return Datatype(mem_holder_, arena_dt);
}


TypedValue HermesCtr::make_typed_value(const Datatype& datatype, const Object& constructor)
{
    assert_not_null();
    assert_mutable();

    Object vv_ctr = do_import_value(constructor);

    auto arena_tv = arena()->allocate_tagged_object<detail::TypedValueData>(
        ShortTypeCode::of<TypedValue>(), datatype->datatype_, vv_ctr->storage_.addr
    );

    return TypedValue(mem_holder_, arena_tv);
}




}}
