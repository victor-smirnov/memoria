
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

#include <memoria/core/linked/document/linked_document.hpp>

namespace memoria {

std::ostream& LDDTypedValueView::dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const
{
    LDDValueView ctr = constructor();
    LDTypeDeclarationView type = this->type();

    auto ref = dump_state.resolve_type_id(type.state_.get());

    if (MMA1_LIKELY(!ctr.is_varchar()))
    {
        out << '@';
        if (!ref) {
            type.dump(out, state, dump_state);
        }
        else {
            out << ref.get();
        }
        out << " = ";
        ctr.dump(out, state, dump_state);
    }
    else {
        ctr.dump(out, state, dump_state);
        out << '@';
        if (!ref) {
            type.dump(out, state, dump_state);
        }
        else {
            out << '#' << ref.get();
        }
    }

    return out;
}

ld_::LDPtr<LDDTypedValueView::State> LDDTypedValueView::deep_copy_to(LDDocumentView* tgt, ld_::LDArenaAddressMapping& mapping) const
{
    const State* src_state = state();

    ld_::LDDPtrHolder src_ptr = src_state->type_decl.get();

    auto mapped_tgt_type = mapping.resolve(src_ptr);

    ld_::LDPtr<ld_::TypeDeclState> tgt_type{};

    if (MMA1_LIKELY((bool)mapped_tgt_type))
    {
        tgt_type = mapped_tgt_type.get();
    }
    else {
        LDTypeDeclarationView src_td(doc_, src_state->type_decl);

        if (MMA1_LIKELY(mapping.is_compaction()))
        {
            tgt_type = src_td.deep_copy_to(tgt, mapping);
        }
        else if (mapping.is_export())
        {
            auto named_type = mapping.is_src_named_type(src_ptr);
            if (named_type && !named_type.get().imported)
            {
                tgt_type = src_td.deep_copy_to(tgt, mapping);

                LDTypeDeclarationView tgt_td(tgt, tgt_type);
                mapping.finish_src_named_type(src_ptr);
                tgt->set_named_type_declaration(named_type.get().name, tgt_td);
            }
        }
        else {
            U8String type_data = src_td.to_standard_string();
            auto dst_named_type = mapping.is_dst_named_type(type_data);

            if (dst_named_type)
            {
                tgt_type = dst_named_type.get();
            }
            else {
                tgt_type = src_td.deep_copy_to(tgt, mapping);

                auto named_type = mapping.is_src_named_type(src_ptr);
                if (named_type)
                {
                    LDTypeDeclarationView tgt_td(tgt, tgt_type);
                    tgt->set_named_type_declaration(named_type.get().name, tgt_td);
                    mapping.finish_dst_named_type(type_data, tgt_type.get());
                }
            }
        }

        mapping.map_ptrs(src_ptr, tgt_type.get());
    }

    LDDValueView src_value(doc_, src_state->value_ptr);

    ld_::LDPtr<State> tgt_state = allocate_tagged<State>(
                ld_tag_size<LDTypedValue>(),
                &tgt->arena_,
                State{tgt_type.get(), src_value.deep_copy_to(tgt, mapping)}
    );

    ld_::ldd_set_tag(&tgt->arena_, tgt_state.get(), ld_tag_value<LDTypedValue>());

    return tgt_state.get();
}


}
