
// Copyright 2019 Victor Smirnov
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

#include <memoria/v1/core/linked/document/ld_document.hpp>
#include <memoria/v1/core/linked/document/ld_string.hpp>
#include <memoria/v1/core/linked/document/ld_identifier.hpp>
#include <memoria/v1/core/linked/document/ld_map.hpp>
#include <memoria/v1/core/linked/document/ld_array.hpp>
#include <memoria/v1/core/linked/document/ld_value.hpp>

#include <memoria/v1/core/strings/format.hpp>

namespace memoria {
namespace v1 {

class LDTypeDeclarationView {
    using TypeDeclState = ld_::TypeDeclState;
    using TypeDeclPtr   = ld_::TypeDeclPtr;

    using ParamsVector = LinkedVector<TypeDeclPtr>;
    using ArgsVector   = LinkedVector<ld_::PtrHolder>;

    const LDDocumentView* doc_;
    TypeDeclPtr state_;

    friend class LDDocumentBuilder;
    friend class LDDocumentView;
    friend class LDDumpState;
    friend class LDDTypedValueView;
    friend class ld_::LDArenaAddressMapping;

    template <typename>
    friend struct DataTypeTraits;

public:
    LDTypeDeclarationView(): doc_(), state_({}) {}

    LDTypeDeclarationView(const LDDocumentView* doc, TypeDeclPtr state):
        doc_(doc), state_(state)
    {}

    bool operator==(const LDTypeDeclarationView& other) const noexcept {
        return doc_->equals(other.doc_) && state_.get() == other.state_.get();
    }

    operator LDDValueView() const {
        return LDDValueView{doc_, state_.get()};
    }

    U8StringView name() const {
        return state()->name.get(&doc_->arena_)->view();
    }

    size_t params() const
    {
        const TypeDeclState* state = this->state();
        if (state->type_params) {
            return state->type_params.get(&doc_->arena_)->size();
        }

        return 0;
    }

    bool is_parametric() const {
        return params() > 0;
    }

    bool is_stateless() const
    {
        if (MMA1_LIKELY(constructor_args() == 0))
        {
            for (size_t c = 0; c < params(); c++) {
                if (!get_type_declration(c).is_stateless()) {
                    return false;
                }
            }

            return true;
        }

        return false;
    }

    LDTypeDeclarationView get_type_declration(size_t idx) const
    {
        const TypeDeclState* state = this->state();
        if (state->type_params)
        {
            const auto* params_array = state->type_params.get(&doc_->arena_);

            if (idx < params_array->size())
            {
                return LDTypeDeclarationView(doc_, params_array->access(idx));
            }
        }

        MMA1_THROW(RuntimeException()) << WhatCInfo("Supplied index is out of range");
    }

    LDTypeDeclarationView add_type_declaration(U8StringView name)
    {
        LDTypeDeclarationView decl = doc_->make_mutable()->new_type_declaration(name);
        ensure_params_capacity(1)->push_back(decl.state_);
        return decl;
    }


    void remove_type_declaration(size_t idx)
    {
        TypeDeclState* state = this->state_mutable();
        if (state->type_params)
        {
            auto* params_array = state->type_params.get_mutable(&doc_->arena_);

            size_t size = params_array->size();

            if (idx < size)
            {
                params_array->remove(idx, 1);

                if (size == 1) {
                    this->state_mutable()->type_params = {};
                }
            }
        }

        MMA1_THROW(RuntimeException()) << WhatCInfo("Supplied index is out of range");
    }

    size_t constructor_args() const
    {
        const TypeDeclState* state = this->state();
        if (state->ctr_args) {
            return state->ctr_args.get(&doc_->arena_)->size();
        }

        return 0;
    }

    LDDValueView get_constructor_arg(size_t idx) const
    {
        const TypeDeclState* state = this->state();
        if (state->ctr_args)
        {
            const auto* ctr_args = state->ctr_args.get(&doc_->arena_);

            if (idx < ctr_args->size())
            {
                return LDDValueView{doc_, ctr_args->access(idx)};
            }
        }

        MMA1_THROW(RuntimeException()) << WhatCInfo("Supplied index is out of range");
    }

    LDStringView add_string_constructor_arg(U8StringView value)
    {
        LDStringView str = doc_->make_mutable()->new_varchar(value);
        ensure_args_capacity(1)->push_back(str.string_.get());
        return str;
    }

    void add_double_constructor_arg(double value)
    {
        LDDValueView val = doc_->make_mutable()->new_double(value);
        ensure_args_capacity(1)->push_back(val.value_ptr_);
    }

    void add_integer_constructor_arg(int64_t value)
    {
        LDDValueView val = doc_->make_mutable()->new_bigint(value);
        ensure_args_capacity(1)->push_back(val.value_ptr_);
    }

    LDDArrayView add_array_constructor_arg()
    {
        LDDArrayView array = doc_->make_mutable()->new_array();
        ensure_args_capacity(1)->push_back(array.array_.ptr());
        return array;
    }

    LDDMapView add_map_constructor_arg()
    {
        LDDMapView map = doc_->make_mutable()->new_map();
        ensure_args_capacity(1)->push_back(map.map_.ptr());
        return map;
    }

    void remove_constructor_arg(size_t idx)
    {
        TypeDeclState* state = this->state_mutable();
        if (state->ctr_args)
        {
            auto* ctr_args = state->ctr_args.get_mutable(&doc_->arena_);

            size_t size = ctr_args->size();

            if (idx < size)
            {
                ctr_args->remove(idx, 1);

                if (size == 1) {
                    this->state_mutable()->ctr_args = {};
                }
            }
        }

        MMA1_THROW(RuntimeException()) << WhatCInfo("Supplied index is out of range");
    }

    U8String to_cxx_typedecl() const
    {
        std::stringstream ss;
        LDDumpFormatState state = LDDumpFormatState::simple();
        LDDumpState dump_state(*doc_);

        do_dump_cxx_type_decl(ss,state, dump_state);
        return ss.str();
    }

    U8String to_standard_string() const
    {
        std::stringstream ss;
        LDDumpFormatState state = LDDumpFormatState::no_indent();
        LDDumpState dump_state(*doc_);

        do_dump(ss,state, dump_state);

        return ss.str();
    }

    std::ostream& dump(std::ostream& out) const
    {
        LDDumpFormatState state;
        LDDumpState dump_state(*doc_);
        dump(out, state, dump_state);
        return out;
    }

    std::ostream& dump(std::ostream& out, LDDumpFormatState& format) const
    {
        LDDumpState dump_state(*doc_);
        dump(out, format, dump_state);
        return out;
    }

    std::ostream& dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const
    {
        if (state.indent_size() == 0 || !is_simple_layout()) {
            do_dump(out, state, dump_state);
        }
        else {
            LDDumpFormatState state = LDDumpFormatState::no_indent();
            do_dump(out, state, dump_state);
        }

        return out;
    }

    bool is_simple_layout() const noexcept
    {
        size_t params = this->params();
        size_t args = this->constructor_args();

        if (params > 3 || args > 5) {
            return false;
        }

        bool simple = true;

        for (size_t c = 0; c < params; c++) {
            simple = simple && get_type_declration(c).is_simple_layout();
        }

        if (simple) {
            for (size_t c = 0; c < args; c++) {
                simple = simple && get_constructor_arg(c).is_simple_layout();
            }
        }

        return simple;
    }

    ld_::LDPtr<TypeDeclState> deep_copy_to(LDDocumentView* tgt, ld_::LDArenaAddressMapping& mapping) const;

    LDDocument clone(bool compactify = true) const {
        LDDValueView vv = *this;
        return vv.clone(compactify);
    }

private:

    void do_dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const;

    void do_dump_cxx_type_decl(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const;

    void add_param(LDTypeDeclarationView type_decl)
    {
        ensure_params_capacity(1)->push_back(type_decl.state_);
    }

    void add_ctr_arg(LDDValueView ctr_arg)
    {
        ensure_args_capacity(1)->push_back(ctr_arg.value_ptr_);
    }

    ParamsVector* ensure_params_capacity(size_t capacity);
    ArgsVector* ensure_args_capacity(size_t capacity);



    size_t next_size(size_t capacity, size_t requested) const
    {
        size_t next_capaicty = capacity * 2;

        if (next_capaicty == 0) next_capaicty = 1;

        while (capacity + requested > next_capaicty)
        {
            next_capaicty *= 2;
        }

        return next_capaicty;
    }


    TypeDeclState* state_mutable() {
        return state_.get_mutable(&doc_->arena_);
    }

    const TypeDeclState* state() const {
        return state_.get(&doc_->arena_);
    }
};

static inline std::ostream& operator<<(std::ostream& out, const LDTypeDeclarationView& value) {
    LDDumpFormatState format = LDDumpFormatState().simple();
    value.dump(out, format);
    return out;
}

template <>
struct DataTypeTraits<LDTypeDeclaration> {
    static constexpr bool isDataType = true;
    using LDStorageType = NullType;
    using LDViewType = LDTypeDeclarationView;

    static void create_signature(SBuf& buf) {
        buf << "LDTypeDeclaration";
    }
};


}}

namespace fmt {

template <>
struct formatter<memoria::v1::LDTypeDeclarationView> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::v1::LDTypeDeclarationView& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.to_standard_string());
    }
};


}
