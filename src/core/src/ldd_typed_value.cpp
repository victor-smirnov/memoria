
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

#include <memoria/v1/core/linked/document/linked_document.hpp>

namespace memoria {
namespace v1 {

std::ostream& LDDTypedValue::dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const
{
    LDDValue ctr = constructor();
    LDTypeDeclaration type = this->type();

    auto ref = dump_state.resolve_type_id(type.state_.get());

    if (MMA1_LIKELY(!ctr.is_string()))
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

SDN2Ptr<LDDTypedValue::State> LDDTypedValue::deep_copy_to(LDDocument* tgt, SDN2ArenaAddressMapping& mapping) const
{
    const State* src_state = state();

    auto mapped_tgt_type = sdn2_::resolve(mapping, src_state->type_decl.get());

    SDN2Ptr<sdn2_::TypeDeclState> tgt_type{};

    if (MMA1_LIKELY((bool)mapped_tgt_type))
    {
        tgt_type = mapped_tgt_type.get();
    }
    else {
        LDTypeDeclaration td(doc_, src_state->type_decl);
        tgt_type = td.deep_copy_to(tgt, mapping);
        mapping[src_state->type_decl.get()] = tgt_type.get();
    }

    LDDValue src_value(doc_, src_state->value_ptr);

    SDN2Ptr<State> tgt_state = allocate_tagged<State>(
                sizeof(LDDValueTag),
                tgt->ld_arena_.view(),
                State{tgt_type.get(), src_value.deep_copy_to(tgt, mapping)}
    );

    sdn2_::ld_set_tag(tgt->ld_arena_.view(), tgt_state.get(), LDDValueTraits<LDDTypedValue>::ValueTag);

    return tgt_state.get();
}


}}
