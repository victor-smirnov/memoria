
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
            ShortTypeCode tag,
            DeepCopyState& dedup) const
    {
        auto& dst = dedup.arena();
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
                dt_data = dst.get_resolver_for(
                            get_type_reflection(tag0).deep_copy(datatype_.get(), dedup));
            }

            if (constructor_.is_not_null())
            {
                auto tag0 = arena::read_type_tag(constructor_.get());
                constructor = dst.get_resolver_for(get_type_reflection(tag0).deep_copy(constructor_.get(), dedup));
            }

            TypedValueData* tv_data = dst.allocate_tagged_object<TypedValueData>(tag, dt_data.get(dst), constructor.get(dst));
            dedup.map(dst, this, tv_data);

            return tv_data;
        }
    }

    void check(CheckStructureState& state) const
    {
        state.check_and_set_tagged(this, sizeof(DatatypeData), MA_SRC);
        state.mark_as_processed(this);

        if (MMA_UNLIKELY(datatype_.is_null())) {
            MEMORIA_MAKE_GENERIC_ERROR("TypedValue datatype is null").do_throw();
        }

        state.check_alignment<DatatypeData>(datatype_.get(), MA_SRC);
        state.check_ptr(datatype_.get(), MA_SRC);

        if (constructor_.is_not_null()) {
            state.check_ptr(constructor_.get(), MA_SRC);
        }
    }
};

}

class TypedValueView: public HoldingView<TypedValueView> {
    using Base = HoldingView<TypedValueView>;
protected:
    mutable detail::TypedValueData* tv_;
public:
    TypedValueView() noexcept:
        tv_()
    {}

    TypedValueView(LWMemHolder* ptr_holder, void* tv) noexcept:
        Base(ptr_holder),
        tv_(reinterpret_cast<detail::TypedValueData*>(tv))
    {}

    HermesCtr ctr();

    Object as_object() const {
        return Object(ObjectView(mem_holder_, tv_));
    }

    bool is_null() const {
        return tv_ == nullptr;
    }

    bool is_null_not() const {
        return tv_ != nullptr;
    }

    bool is_not_null() const {
        return tv_ != nullptr;
    }

    Object constructor() const
    {
        assert_not_null();
        return Object(mem_holder_, tv_->constructor());
    }

    Datatype datatype() const
    {
        assert_not_null();
        return Datatype(mem_holder_, tv_->datatype());
    }

    void stringify(std::ostream& out,
                   DumpFormatState& state) const;

    bool is_simple_layout() const
    {
        assert_not_null();
        return constructor().is_simple_layout() && datatype().is_simple_layout();
    }

    void* deep_copy_to(DeepCopyState& dedup) const {
        assert_not_null();
        return tv_->deep_copy_to(ShortTypeCode::of<TypedValue>(), dedup);
    }

    bool operator!=(const TypedValueView& other) const {
        return !operator==(other);
    }

    bool operator==(const TypedValueView& other) const
    {
        if (is_not_null() && other.is_not_null())
        {
            if (constructor() != other.constructor()) {
                return false;
            }

            return datatype() == other.datatype();
        }

        return is_null() && other.is_null();
    }


    operator Object() const & noexcept {
        return as_object();
    }

    operator Object() && noexcept {
        return Object(this->release_mem_holder(), tv_ , MoveOwnershipTag{});
    }

    static void check_structure(const void* addr, CheckStructureState& state)
    {
        state.check_alignment<detail::TypedValueData>(addr, MA_SRC);

        const detail::TypedValueData* tv
                = reinterpret_cast<const detail::TypedValueData*>(addr);
        tv->check(state);
    }


private:
    void assert_not_null() const
    {
        if (MMA_UNLIKELY(tv_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("TypedValueView is null").do_throw();
        }
    }
};

}}
