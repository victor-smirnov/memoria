
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


void HermesCtrView::deep_copy_from(const DocumentHeader* src, DeepCopyDeduplicator& dedup) {
    header_ = src->deep_copy_to(*arena_, mem_holder_, dedup);
}

HermesCtr HermesCtrView::compactify(bool make_immutable) const
{
    assert_not_null();

    if (MMA_UNLIKELY(!arena_)) {
        return HermesCtr(mem_holder_);
    }

    arena::AllocationType alc_type = make_immutable ? arena::AllocationType::GROWABLE_SINGLE_CHUNK :
                                                      arena::AllocationType::MULTI_CHUNK;
    auto arena = TL_allocate_shared<arena::PoolableArena>(alc_type, arena_->chunk_size());

    HermesCtr ctr(&arena->mem_holder(), arena.get());

    DeepCopyDeduplicator dedup;
    ctr.deep_copy_from(header_, dedup);

    return ctr;
}

HermesCtr HermesCtrView::clone(bool as_mutable) const
{
    assert_not_null();
    if (arena_)
    {
        if (arena_->chunks() > 1 || !as_mutable)
        {
            size_t chunk_size = arena_->chunk_size();
            auto arena = TL_allocate_shared<arena::PoolableArena>(arena::AllocationType::GROWABLE_SINGLE_CHUNK, chunk_size);

            HermesCtr ctr(&arena->mem_holder(), arena.get());

            DeepCopyDeduplicator dedup;
            ctr.deep_copy_from(header_, dedup);

            if (as_mutable) {
                arena->switch_to_chunked_mode();
            }

            return ctr;
        }
        else {
            auto& head = arena_->head();
            auto arena = TL_allocate_shared<arena::PoolableArena>(
                        arena::AllocationType::GROWABLE_SINGLE_CHUNK,
                        arena_->chunk_size(), head.memory.get(), head.size);

            HermesCtr ctr(&arena->mem_holder());

            if (as_mutable) {
                arena->switch_to_chunked_mode();
            }

            return ctr;
        }
    }
    else {
        arena::AllocationType alc_type = as_mutable ? arena::AllocationType::MULTI_CHUNK :
                                                      arena::AllocationType::GROWABLE_SINGLE_CHUNK;

        auto arena = TL_allocate_shared<arena::PoolableArena>(alc_type, 4096, header_, segment_size_);

        HermesCtr ctr(&arena->mem_holder());
        return ctr;
    }
}



void HermesCtrView::set_root(const Object& value)
{
    Object vv = do_import_value(value);
    header_->root = vv.storage_.addr;
}

Datatype HermesCtrView::new_datatype(U8StringView name)
{
    auto str = new_dataobject<Varchar>(name);
    auto arena_dt = arena()->allocate_tagged_object<detail::DatatypeData>(
        ShortTypeCode::of<Datatype>(),
                reinterpret_cast<arena::ArenaDataTypeContainer<Varchar>*>(str.addr())
    );

    return Datatype(mem_holder_, arena_dt);
}


TypedValue HermesCtrView::new_typed_value(Datatype datatype, const Object& constructor)
{
    return make_typed_value(datatype, constructor);
}

Object HermesCtrView::do_import_value(const Object& value)
{
    assert_not_null();
    assert_mutable();

    if (!value.is_null())
    {
        if (value.get_mem_holder() != mem_holder_)
        {
            if (value.get_vs_tag() == ValueStorageTag::VS_TAG_ADDRESS)
            {
                auto tag = arena::read_type_tag(value.storage_.addr);

                DeepCopyDeduplicator dedup;
                auto addr = get_type_reflection(tag).deep_copy_to(*arena_, mem_holder_, value.storage_.addr, dedup);

                return Object(mem_holder_, addr);
            }
            else {
                auto type_tag = value.get_type_tag();
                auto vs_tag = value.get_vs_tag();
                return get_type_reflection(type_tag).import_value(mem_holder_, vs_tag, value.storage_);
            }
        }
        else if (value.get_vs_tag() != ValueStorageTag::VS_TAG_ADDRESS)
        {
            auto type_tag = value.get_type_tag();
            auto vs_tag = value.get_vs_tag();
            return get_type_reflection(type_tag).import_value(mem_holder_, vs_tag, value.storage_);
        }
        else {
            return value;
        }
    }
    else {
        return value;
    }
}


Object HermesCtrView::do_import_embeddable(const Object& value)
{
    return import_object(value);
}

Object HermesCtrView::import_object(const Object& object)
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

                return Object(mem_holder_, addr);
            }
            else if (object.get_vs_tag() == ValueStorageTag::VS_TAG_SMALL_VALUE)
            {
                auto type_tag = object.get_type_tag();
                auto& refl = get_type_reflection(type_tag);

                if (refl.hermes_is_ptr_embeddable()) {
                    return Object(mem_holder_, object.storage_.small_value);
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


Parameter HermesCtrView::new_parameter(U8StringView name)
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
    HermesCtr ctr;
    CommonInstance()
    {
        ctr = HermesCtrView::make_pooled();
        auto str = ctr.make_t<Varchar>("Dedicated HermesCtrView instance for 8-byte data objects");
        ctr.set_root(str.as_object());
    }
};



HermesCtr HermesCtrView::common_instance() {
    static thread_local CommonInstance instance;
    return instance.ctr;
}


ObjectArray HermesCtrView::make_object_array(uint64_t capacity) {
    return HermesCtrView::make_array<Object>(capacity);
}


Parameter HermesCtrView::make_parameter(const U8StringView& name)
{
    assert_not_null();
    assert_mutable();

    auto arena_dtc = arena_->allocate_tagged_object<typename ParameterView::ArenaDTContainer>(
        ShortTypeCode::of<Parameter>(),
        name
    );

    return Parameter(mem_holder_, arena_dtc);
}



Datatype HermesCtrView::make_datatype(const U8StringView& name)
{
    assert_not_null();
    assert_mutable();

    Object nameo = make(name);

    auto arena_dt = arena()->allocate_tagged_object<detail::DatatypeData>(
        ShortTypeCode::of<Datatype>(),
                reinterpret_cast<arena::ArenaDataTypeContainer<Varchar>*>(nameo.addr())
    );

    return Datatype(mem_holder_, arena_dt);
}

Datatype HermesCtrView::make_datatype(const StringOView& name)
{
    assert_not_null();
    assert_mutable();

    Object nameo = import_object(name);

    auto arena_dt = arena()->allocate_tagged_object<detail::DatatypeData>(
        ShortTypeCode::of<Datatype>(),
                reinterpret_cast<arena::ArenaDataTypeContainer<Varchar>*>(nameo.addr())
    );

    return Datatype(mem_holder_, arena_dt);
}


TypedValue HermesCtrView::make_typed_value(const Datatype& datatype, const Object& constructor)
{
    assert_not_null();
    assert_mutable();

    Object vv_ctr = do_import_value(constructor);

    auto arena_tv = arena()->allocate_tagged_object<detail::TypedValueData>(
        ShortTypeCode::of<TypedValue>(), datatype.datatype_, vv_ctr.storage_.addr
    );

    return TypedValue(mem_holder_, arena_tv);
}

Object HermesCtrView::import_small_object(const Object& object)
{
    auto vs_tag = object.get_vs_tag();
    if (MMA_LIKELY(vs_tag == VS_TAG_SMALL_VALUE))
    {
        auto type_tag = object.get_type_tag();
        auto& refl = get_type_reflection(type_tag);
        return refl.import_value(mem_holder_, vs_tag, object.storage_);
    }
    else {
        return object;
    }
}


}}
