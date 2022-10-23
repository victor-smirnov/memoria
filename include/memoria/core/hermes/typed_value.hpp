
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

#include <memoria/core/arena/arena.hpp>
#include <memoria/core/arena/relative_ptr.hpp>
#include <memoria/core/hermes/datatype.hpp>

namespace memoria {
namespace hermes {

namespace detail {

class TypedValueData {
    arena::RelativePtr<DatatypeData> datatype_;
    arena::RelativePtr<void> constructor_;
public:
    TypedValueData(
        DatatypeData* datatype,
        void* constructor
    ): datatype_(datatype), constructor_(constructor)
    {}

    DatatypeData* datatype() const {
        return datatype_.get();
    }

    void* constructor() const {
        return constructor_.get();
    }

    TypedValueData* deep_copy_to(
            arena::ArenaAllocator& dst,
            arena::ObjectTag tag,
            void* owner_view,
            ViewPtrHolder* ptr_holder,
            DeepCopyDeduplicator& dedup) const
    {
        TypedValueData* existing = dedup.resolve(dst, this);
        if (MMA_LIKELY((bool)existing)) {
            return existing;
        }
        else {
            arena::AddrResolver<DatatypeData> dt_data;
            arena::AddrResolver<void> constructor;

            if (datatype_.is_not_null())
            {
                auto tag0 = arena::read_type_tag(datatype_.get());
                dt_data = dst.get_resolver_for(get_type_reflection(tag0).deep_copy(dst, datatype_.get(), owner_view, ptr_holder, dedup));
            }

            if (constructor_.is_not_null())
            {
                auto tag0 = arena::read_type_tag(constructor_.get());
                constructor = dst.get_resolver_for(get_type_reflection(tag0).deep_copy(dst, constructor_.get(), owner_view, ptr_holder, dedup));
            }

            TypedValueData* tv_data = dst.allocate_tagged_object<TypedValueData>(tag, dt_data.get(dst), constructor.get(dst));
            dedup.map(dst, this, tv_data);

            return tv_data;
        }
    }
};

}

class TypedValue: public HoldingView {
    mutable detail::TypedValueData* tv_;
    mutable HermesCtr* doc_;
public:
    TypedValue() noexcept:
        tv_(), doc_()
    {}

    TypedValue(void* tv, HermesCtr* doc, ViewPtrHolder* ptr_holder) noexcept:
        HoldingView(ptr_holder),
        tv_(reinterpret_cast<detail::TypedValueData*>(tv)), doc_(doc)
    {}

    PoolSharedPtr<HermesCtr> document() const {
        assert_not_null();
        return PoolSharedPtr<HermesCtr>(doc_, ptr_holder_->owner(), pool::DoRef{});
    }

    ValuePtr as_value() const {
        return ValuePtr(Value(tv_, doc_, ptr_holder_));
    }

    bool is_null() const {
        return tv_ == nullptr;
    }

    bool is_null_not() const {
        return tv_ != nullptr;
    }

    ValuePtr constructor() const
    {
        assert_not_null();
        return ValuePtr(Value(tv_->constructor(), doc_, ptr_holder_));
    }

    DatatypePtr datatype() const
    {
        assert_not_null();
        return DatatypePtr(Datatype(tv_->datatype(), doc_, ptr_holder_));
    }

    void stringify(std::ostream& out,
                   DumpFormatState& state,
                   DumpState& dump_state) const;

    bool is_simple_layout() const
    {
        assert_not_null();
        return constructor()->is_simple_layout() && datatype()->is_simple_layout();
    }

    void* deep_copy_to(arena::ArenaAllocator& arena, DeepCopyDeduplicator& dedup) const {
        assert_not_null();
        return tv_->deep_copy_to(arena, TypeHashV<TypedValue>, doc_, ptr_holder_, dedup);
    }

private:
    void assert_not_null() const
    {
        if (MMA_UNLIKELY(tv_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("TypedValue is null").do_throw();
        }
    }
};

}}
