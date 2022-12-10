
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

#include <memoria/core/memory/shared_ptr.hpp>
#include <memoria/core/arena/string.hpp>
#include <memoria/core/arena/array.hpp>
#include <memoria/core/arena/relative_ptr.hpp>
#include <memoria/core/arena/hash_fn.hpp>


#include <memoria/core/hermes/object.hpp>
#include <memoria/core/hermes/common.hpp>

#include <memoria/core/tools/uid_256.hpp>

namespace memoria {
namespace hermes {

class HermesCtr;

class PtrQualifier {
    uint8_t value_;
public:
    PtrQualifier(): value_() {}
    PtrQualifier(uint8_t is_const, uint8_t is_volatile) {
        value_ = is_const | (is_volatile << 1);
    }

    PtrQualifier(uint64_t val):
        value_(val)
    {}

    bool is_const() const {
        return value_ & 0x1;
    }

    bool is_volatile() const {
        return value_ & 0x2;
    }

    uint64_t value() const {
        return value_;
    }
};

namespace detail {

class TypeExtras {
    static constexpr uint64_t F_CONST           = 0x1;
    static constexpr uint64_t F_VOLATILE        = 0x2;
    static constexpr uint64_t F_RESERVED        = 0x3;  // Extra bit for future use
    static constexpr uint64_t F_REFS_SIZE_START = 0x3;
    static constexpr uint64_t F_REFS_SIZE_MASK  = 0x18; // 0, 1, 2

    static constexpr uint64_t F_PTRS_SIZE_MASK  = 0x3E0;

    static constexpr uint64_t MAX_POINTERS      = 27;
    static constexpr uint64_t F_PTRS_SIZE_START = 5;

    static constexpr uint64_t F_PTRS_START      = 10;
    static constexpr uint64_t F_PTR_SIZE        = 2;
    static constexpr uint64_t F_PTR_MASK        = 0x3;

    uint64_t data_;
public:
    TypeExtras(): data_() {}

    bool is_const() const {
        return data_ & F_CONST;
    }

    void set_const() {
        data_ |= F_CONST;
    }

    void set_non_const() {
        data_ &= ~F_CONST;
    }

    bool is_volatile() const {
        return data_ & F_VOLATILE;
    }

    void set_volatile() {
        data_ |= F_VOLATILE;
    }

    void set_non_volatile() {
        data_ &= ~F_VOLATILE;
    }

    uint64_t refs_size() const {
        return (data_ & F_REFS_SIZE_MASK) >> F_REFS_SIZE_START;
    }


    uint64_t pointers_size() const {
        return (data_ & F_PTRS_SIZE_MASK) >> F_PTRS_SIZE_START;
    }

    void set_pointers_size(uint64_t val)
    {
        if (MMA_LIKELY(val < MAX_POINTERS))
        {
            data_ &= ~F_PTRS_SIZE_MASK;
            data_ |= (val << F_PTRS_SIZE_START);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Maximal muber of ptr specifiers is {}: {}", MAX_POINTERS, val).do_throw();
        }
    }

    void set_refs_size(uint64_t val)
    {
        if (MMA_LIKELY(val < 3))
        {
            data_ &= ~F_REFS_SIZE_MASK;
            data_ |= (val << F_REFS_SIZE_START);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Maximal muber of reference specifiers is {}: {}", 3, val).do_throw();
        }
    }

    void add_pointer(PtrQualifier tq)
    {
        uint64_t size = pointers_size();
        if (size < MAX_POINTERS - 1) {
            set_pointers_size(size + 1);
            set_pointer(size, tq);
        }
    }

    void remove_pointer()
    {
        uint64_t size = pointers_size();
        if (size) {
            set_pointers_size(size - 1);
            set_pointer(size, PtrQualifier());
        }
    }

    PtrQualifier pointer(uint64_t idx) const
    {
        if (MMA_LIKELY(idx < MAX_POINTERS))
        {
            return (data_ >> (F_PTRS_START + idx * F_PTR_SIZE)) & F_PTR_MASK;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Pointer index is out of range: {}", idx).do_throw();
        }
    }



private:

    void set_pointer(uint64_t idx, PtrQualifier value)
    {
        if (MMA_LIKELY(idx < MAX_POINTERS))
        {
            uint64_t bit_offs = (F_PTRS_START + idx * F_PTR_SIZE);
            data_ &= ~(F_PTR_MASK << bit_offs);
            data_ |= (value.value() << bit_offs);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Qualifier index is out of range: {}", idx).do_throw();
        }
    }
};

class DatatypeData {
    arena::RelativePtr<arena::ArenaString> name_;
    arena::RelativePtr<arena::GenericVector> parameters_;
    arena::RelativePtr<arena::GenericVector> constructor_;
    TypeExtras extras_;
public:
    DatatypeData(arena::ArenaString* name):
        name_(name)
    {}

    TypeExtras& extras() {return extras_;}
    const TypeExtras& extras() const {return extras_;}

    arena::ArenaString* name() const {
        return name_.get();
    }

    arena::GenericVector* parameters() const {
        return parameters_.get();
    }

    void set_parameters(arena::GenericVector* params) {
        parameters_ = params;
    }

    arena::GenericVector* constructor() const {
        return constructor_.get();
    }

    void set_constructor(arena::GenericVector* args) {
        constructor_ = args;
    }

    bool has_constructor() const {
        return constructor_.is_not_null();
    }

    bool is_parametric() const {
        return parameters_.is_not_null();
    }

    DatatypeData* deep_copy_to(
            arena::ArenaAllocator& dst,
            ShortTypeCode tag,
            LWMemHolder* ptr_holder,
            DeepCopyDeduplicator& dedup) const
    {
        DatatypeData* existing = dedup.resolve(dst, this);
        if (MMA_LIKELY((bool)existing)) {
            return existing;
        }
        else {
            arena::ArenaString * name_ptr =
                get_type_reflection(ShortTypeCode::of<Varchar>()).deep_copy(dst, ptr_holder, name_.get(), dedup);

            auto dtd = dst.get_resolver_for(dst.template allocate_tagged_object<DatatypeData>(
                tag, name_ptr
            ));

            dedup.map(dst, this, dtd.get(dst));

            if (parameters_.is_not_null())
            {
                auto par_tag = arena::read_type_tag(parameters_.get());
                arena::GenericVector* ptr = get_type_reflection(par_tag).deep_copy(dst, ptr_holder, parameters_.get(), dedup);
                dtd.get(dst)->parameters_ = ptr;
            }
            else {
                dtd.get(dst)->parameters_ = nullptr;
            }

            if (constructor_.is_not_null())
            {
                auto ctr_tag = arena::read_type_tag(constructor_.get());
                arena::GenericVector* ptr = get_type_reflection(ctr_tag).deep_copy(dst, ptr_holder, constructor_.get(), dedup);
                dtd.get(dst)->constructor_ = ptr;
            }
            else {
                dtd.get(dst)->constructor_ = nullptr;
            }

            return dtd.get(dst);
        }
    }
};

}


class DatatypeView: public HoldingView<DatatypeView> {
    using Base = HoldingView<DatatypeView>;
    friend class HermesCtr;
    friend class ObjectView;
    friend class HermesCtrBuilder;

    template <typename, typename>
    friend class MapView;

    template <typename>
    friend class ArrayView;

protected:
    mutable detail::DatatypeData* datatype_;
    using Base::mem_holder_;
public:
    DatatypeView():
        datatype_()
    {}

    DatatypeView(LWMemHolder* ptr_holder, void* dt) noexcept :
        Base(ptr_holder), datatype_(reinterpret_cast<detail::DatatypeData*>(dt))
    {}

    PoolSharedPtr<HermesCtr> document() const {
        assert_not_null();
        return PoolSharedPtr<HermesCtr>(
                    mem_holder_->ctr(),
                    mem_holder_->owner(),
                    pool::DoRef{}
        );
    }

    Object as_object() const {
        return Object(ObjectView(mem_holder_, datatype_));
    }

    U8String to_string(const StringifyCfg& cfg = StringifyCfg()) const
    {
        std::stringstream ss;
        DumpFormatState fmt = DumpFormatState(cfg);
        stringify(ss, fmt);
        return ss.str();
    }

    U8String to_cxx_string() const
    {
        StringifyCfg cfg;
        cfg.set_spec(StringifySpec::simple());

        DumpFormatState fmt = DumpFormatState(cfg);
        std::stringstream ss;
        stringify_cxx(ss, fmt);
        return ss.str();
    }

    U8String to_pretty_string() const
    {
        DumpFormatState fmt = DumpFormatState(StringifyCfg::pretty());
        std::stringstream ss;
        stringify(ss, fmt);
        return ss.str();
    }

    void stringify(std::ostream& out,
                   DumpFormatState& state) const;

    void stringify_cxx(std::ostream& out,
                       DumpFormatState& state) const;

    bool is_simple_layout() const;

    StringValue type_name() const;
    ObjectArray set_constructor();
    ObjectArray constructor() const;

    ObjectArray type_parameters() const;
    Datatype append_type_parameter(U8StringView name);
    Datatype append_type_parameter(StringValue name);

    void clear_parameters()
    {
        assert_not_null();
        assert_mutable();

        datatype_->set_parameters(nullptr);
    }

    void clear_constructor()
    {
        assert_not_null();
        assert_mutable();

        datatype_->set_constructor(nullptr);
    }

    template <typename DT>
    void append_integral_parameter(DTTViewType<DT> view);

    bool has_constructor() const {
        assert_not_null();
        return datatype_->has_constructor();
    }

    bool is_parametric() const {
        assert_not_null();
        return datatype_->is_parametric();
    }

    void remove_constructor()
    {
        assert_not_null();
        assert_mutable();

        datatype_->set_constructor(nullptr);
    }

    void remove_parameters()
    {
        assert_not_null();
        assert_mutable();

        datatype_->set_parameters(nullptr);
    }

    bool is_const() const {
        assert_not_null();
        return datatype_->extras().is_const();
    }

    void set_const(bool v)
    {
        assert_not_null();
        assert_mutable();

        if (v)
            datatype_->extras().set_const();
        else
            datatype_->extras().set_non_const();
    }

    bool is_volatile() const {
        assert_not_null();
        return datatype_->extras().is_volatile();
    }

    void set_volatile(bool v)
    {
        assert_not_null();
        assert_mutable();

        if (v)
            datatype_->extras().set_volatile();
        else
            datatype_->extras().set_non_volatile();
    }



    uint64_t ptr_specs() const
    {
        assert_not_null();
        return datatype_->extras().pointers_size();
    }

    void add_ptr_spec(PtrQualifier qual)
    {
        assert_not_null();
        assert_mutable();

        datatype_->extras().add_pointer(qual);
    }

    PtrQualifier ptr_spec(uint64_t idx) const
    {
        assert_not_null();
        return datatype_->extras().pointer(idx);
    }

    uint64_t refs_size() const
    {
        assert_not_null();
        return datatype_->extras().refs_size();
    }

    void set_refs(uint64_t size) {
        assert_not_null();
        assert_mutable();

        datatype_->extras().set_refs_size(size);
    }

    void* deep_copy_to(arena::ArenaAllocator& arena, DeepCopyDeduplicator& dedup) const {
        assert_not_null();
        return datatype_->deep_copy_to(arena, ShortTypeCode::of<DatatypeView>(), mem_holder_, dedup);
    }

    UID256 cxx_type_hash() const;

    void append_type_parameter(Object value);
    void append_constructor_argument(Object value);


    operator Object() const & noexcept {
        return as_object();
    }

    operator Object() && noexcept {
        return Object(this->release_mem_holder(), datatype_ , MoveOwnershipTag{});
    }


private:
    void assert_not_null() const
    {
        if (MMA_UNLIKELY(datatype_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("DatatypeView is null").do_throw();
        }
    }

    void assert_mutable();
};

UID256 get_cxx_type_hash(U8StringView sre);

std::ostream& operator<<(std::ostream& out, Datatype ptr);

}}
