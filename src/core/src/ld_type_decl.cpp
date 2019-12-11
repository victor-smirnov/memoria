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

#ifndef MMA1_NO_REACTOR
#   include <memoria/v1/reactor/reactor.hpp>
#endif

#include <memoria/v1/core/linked/document/linked_document.hpp>

namespace memoria {
namespace v1 {


void LDTypeDeclarationView::do_dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const
{
    out << name();

    size_t params = this->params();
    if (params > 0)
    {
        out << "<" << state.nl_start();
        bool first = true;

        state.push();
        for (size_t c = 0; c < params; c++)
        {
            if (MMA1_LIKELY(!first)) {
                out << "," << state.nl_middle();
            }
            else {
                first = false;
            }

            state.make_indent(out);

            LDTypeDeclarationView td = get_type_declration(c);
            td.dump(out, state, dump_state);
        }
        state.pop();

        out << state.nl_end();

        state.make_indent(out);
        out << ">";
    }

    size_t args = this->constructor_args();
    if (args > 0)
    {
        out << "(" << state.nl_start();
        bool first = true;

        state.push();
        for (size_t c = 0; c < args; c++)
        {
            if (MMA1_LIKELY(!first)) {
                out << "," << state.nl_middle();
            }
            else {
                first = false;
            }

            state.make_indent(out);


            LDDValueView value = get_constructor_arg(c);
            value.dump(out, state, dump_state);
        }
        state.pop();

        out << state.nl_end();

        state.make_indent(out);
        out << ")";
    }
}

void LDTypeDeclarationView::do_dump_cxx_type_decl(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const
{
    out << name();

    size_t params = this->params();
    if (params > 0)
    {
        out << "<" << state.nl_start();
        bool first = true;

        state.push();
        for (size_t c = 0; c < params; c++)
        {
            if (MMA1_LIKELY(!first)) {
                out << "," << state.nl_middle();
            }
            else {
                first = false;
            }

            state.make_indent(out);

            LDTypeDeclarationView td = get_type_declration(c);
            td.do_dump_cxx_type_decl(out, state, dump_state);
        }
        state.pop();

        out << state.nl_end();

        state.make_indent(out);
        out << ">";
    }
}


LDTypeDeclarationView::ParamsVector* LDTypeDeclarationView::ensure_params_capacity(size_t capacity)
{
    TypeDeclState* state = this->state_mutable();
    if (!state->type_params)
    {
        state->type_params = allocate<ParamsVector>(doc_->arena_.make_mutable(), capacity, 0);
        return state->type_params.get_mutable(&doc_->arena_);
    }

    ParamsVector* params = state->type_params.get_mutable(&doc_->arena_);

    if (params->free_slots() >= capacity)
    {
        return params;
    }
    else {
        auto old_vector = state->type_params;
        size_t size = old_vector.get(&doc_->arena_)->size();

        size_t new_size = next_size(size, 1);

        auto new_ptr = allocate<ParamsVector>(doc_->arena_.make_mutable(), new_size, 0);
        state = this->state_mutable();
        state->type_params = new_ptr;

        ParamsVector* new_params = state->type_params.get_mutable(&doc_->arena_);

        old_vector.get(&doc_->arena_)->copy_to(*new_params);

        return new_params;
    }
}

LDTypeDeclarationView::ArgsVector* LDTypeDeclarationView::ensure_args_capacity(size_t capacity)
{
    TypeDeclState* state = this->state_mutable();
    if (!state->ctr_args)
    {
        state->ctr_args = allocate<ArgsVector>(doc_->arena_.make_mutable(), capacity, 0);
        return state->ctr_args.get_mutable(&doc_->arena_);
    }

    ArgsVector* args = state->ctr_args.get_mutable(&doc_->arena_);

    if (args->free_slots() >= capacity)
    {
        return args;
    }
    else {
        auto old_vector = state->ctr_args;
        size_t size = old_vector.get(&doc_->arena_)->size();

        size_t new_size = next_size(size, 1);

        auto new_ptr = allocate<ArgsVector>(doc_->arena_.make_mutable(), new_size, 0);
        state = this->state_mutable();
        state->ctr_args = new_ptr;

        ArgsVector* new_args = state->ctr_args.get_mutable(&doc_->arena_);

        old_vector.get(&doc_->arena_)->copy_to(*new_args);

        return new_args;
    }
}


ld_::LDPtr<LDTypeDeclarationView::TypeDeclState> LDTypeDeclarationView::deep_copy_to(LDDocumentView* tgt, ld_::LDArenaAddressMapping& mapping) const
{
    const TypeDeclState* src_state = state();

    LDIdentifierView src_name(doc_, src_state->name);

    ld_::LDPtr<U8LinkedString> tgt_name = tgt->new_varchar(src_name.view()).ptr();

    ld_::LDArenaView* tgt_arena_view = &tgt->arena_;

    ld_::LDPtr<ParamsVector> tgt_params{};

    if (src_state->type_params)
    {
        const ParamsVector* src_params = src_state->type_params.get(&doc_->arena_);
        tgt_params = allocate<ParamsVector>(tgt_arena_view, src_params->size(), src_params->size());

        for (size_t c = 0; c < src_params->size(); c++)
        {
            LDTypeDeclarationView src_td(doc_, src_params->access(c));
            ld_::LDPtr<TypeDeclState> tgt_td = src_td.deep_copy_to(tgt, mapping);
            tgt_params.get(tgt_arena_view)->access(c) = tgt_td;
        }
    }

    ld_::LDPtr<ArgsVector> tgt_args{};

    if (src_state->ctr_args)
    {
        const ArgsVector* src_args = src_state->ctr_args.get(&doc_->arena_);
        tgt_args = allocate<ArgsVector>(tgt_arena_view, src_args->size(), src_args->size());

        for (size_t c = 0; c < src_args->size(); c++)
        {
            LDDValueView src_val(doc_, src_args->access(c));
            ld_::LDDPtrHolder tgt_val = src_val.deep_copy_to(tgt, mapping);
            tgt_args.get(tgt_arena_view)->access(c) = tgt_val;
        }
    }

    ld_::LDPtr<TypeDeclState> tgt_state = allocate_tagged<TypeDeclState>(
                sizeof(LDDValueTag),
                tgt_arena_view,
                TypeDeclState{tgt_name, tgt_params, tgt_args, 0}
    );

    return tgt_state;
}

}}
