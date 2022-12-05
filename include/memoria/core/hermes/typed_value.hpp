
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
            ShortTypeCode tag,
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
                dt_data = dst.get_resolver_for(get_type_reflection(tag0).deep_copy(dst, datatype_.get(), ptr_holder, dedup));
            }

            if (constructor_.is_not_null())
            {
                auto tag0 = arena::read_type_tag(constructor_.get());
                constructor = dst.get_resolver_for(get_type_reflection(tag0).deep_copy(dst, constructor_.get(), ptr_holder, dedup));
            }

            TypedValueData* tv_data = dst.allocate_tagged_object<TypedValueData>(tag, dt_data.get(dst), constructor.get(dst));
            dedup.map(dst, this, tv_data);

            return tv_data;
        }
    }
};

}

class TypedValue: public HoldingView<TypedValue> {
    using Base = HoldingView<TypedValue>;
protected:
    mutable detail::TypedValueData* tv_;
public:
    TypedValue() noexcept:
        tv_()
    {}

    TypedValue(void* tv, ViewPtrHolder* ptr_holder) noexcept:
        Base(ptr_holder),
        tv_(reinterpret_cast<detail::TypedValueData*>(tv))
    {}

    PoolSharedPtr<HermesCtr> document() {
        assert_not_null();
        return PoolSharedPtr<HermesCtr>(
                    ptr_holder_->ctr(),
                    ptr_holder_->owner(),
                    pool::DoRef{}
        );
    }

    ObjectPtr as_object() const {
        return ObjectPtr(Object(tv_, ptr_holder_));
    }

    bool is_null() const {
        return tv_ == nullptr;
    }

    bool is_null_not() const {
        return tv_ != nullptr;
    }

    ObjectPtr constructor() const
    {
        assert_not_null();
        return ObjectPtr(Object(tv_->constructor(), ptr_holder_));
    }

    DatatypePtr datatype() const
    {
        assert_not_null();
        return DatatypePtr(Datatype(tv_->datatype(), ptr_holder_));
    }

    void stringify(std::ostream& out,
                   DumpFormatState& state) const;

    bool is_simple_layout() const
    {
        assert_not_null();
        return constructor()->is_simple_layout() && datatype()->is_simple_layout();
    }

    void* deep_copy_to(arena::ArenaAllocator& arena, DeepCopyDeduplicator& dedup) const {
        assert_not_null();
        return tv_->deep_copy_to(arena, ShortTypeCode::of<TypedValue>(), ptr_holder_, dedup);
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
