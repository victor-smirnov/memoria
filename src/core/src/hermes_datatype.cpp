
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

#include <memoria/core/hermes/hermes.hpp>

namespace memoria {
namespace hermes {

GenericArrayPtr Datatype::constructor()
{
    assert_not_null();
    if (datatype_->has_constructor()) {
        return GenericArrayPtr(GenericArray(datatype_->constructor(), doc_, ptr_holder_));
    }
    else {
        return GenericArrayPtr(GenericArray(nullptr, doc_, ptr_holder_));
    }
}


GenericArrayPtr Datatype::type_parameters()
{
    assert_not_null();
    if (datatype_->is_parametric()) {
        return GenericArrayPtr(GenericArray(datatype_->parameters(), doc_, ptr_holder_));
    }
    else {
        return GenericArrayPtr(GenericArray(nullptr, doc_, ptr_holder_));
    }
}


void Datatype::stringify(std::ostream& out,
               DumpFormatState& state,
               DumpState& dump_state)
{
    if (datatype_)
    {
        out << type_name()->view();

        auto params = type_parameters();
        if (params->is_not_null())
        {
            out << "<" << state.nl_start();
            bool first = true;

            state.push();
            for (size_t c = 0; c < params->size(); c++)
            {
                if (MMA_LIKELY(!first)) {
                    out << "," << state.nl_middle();
                }
                else {
                    first = false;
                }

                state.make_indent(out);

                auto type_param = params->get(c);
                type_param->stringify(out, state, dump_state);
            }
            state.pop();

            out << state.nl_end();

            state.make_indent(out);
            out << ">";
        }

        auto ctr_args = constructor();
        if (ctr_args->is_not_null())
        {
            out << "(" << state.nl_start();
            bool first = true;

            state.push();
            for (size_t c = 0; c < ctr_args->size(); c++)
            {
                if (MMA_LIKELY(!first)) {
                    out << "," << state.nl_middle();
                }
                else {
                    first = false;
                }

                state.make_indent(out);

                auto value = ctr_args->get(c);
                value->stringify(out, state, dump_state);
            }
            state.pop();

            out << state.nl_end();

            state.make_indent(out);
            out << ")";
        }
    }
    else {
        out << "null";
    }
}


void Datatype::stringify_cxx(std::ostream& out,
               DumpFormatState& state,
               DumpState& dump_state)
{
    if (datatype_)
    {
        out << type_name()->view();

        auto params = type_parameters();
        if (params->is_not_null())
        {
            out << "<" << state.nl_start();
            bool first = true;

            state.push();
            for (size_t c = 0; c < params->size(); c++)
            {
                if (MMA_LIKELY(!first)) {
                    out << "," << state.nl_middle();
                }
                else {
                    first = false;
                }

                state.make_indent(out);

                ValuePtr type_param = params->get(c);
                if (type_param->is_a(TypeTag<Datatype>{})) {
                    cast_to<Datatype>(type_param)->stringify_cxx(out, state, dump_state);
                }
            }
            state.pop();

            out << state.nl_end();

            state.make_indent(out);
            out << ">";
        }
    }
    else {
        out << "null";
    }
}

bool Datatype::is_simple_layout() {
    assert_not_null();

    bool sl = true;

    if (is_parametric()) {
        sl = type_parameters()->is_simple_layout();
    }

    if (has_constructor()) {
        sl = sl && constructor()->is_simple_layout();
    }

    return sl;
}

DatatypePtr Datatype::append_type_parameter(U8StringView name)
{
    assert_not_null();
    assert_mutable();

    GenericArrayPtr params = type_parameters();

    if (MMA_UNLIKELY(params->is_null())) {
        params = doc_->new_array();
        datatype_->set_parameters(params->array_);
    }

    return params->append_datatype(name);
}

DatatypePtr Datatype::append_type_parameter(StringValuePtr name)
{
    assert_not_null();
    assert_mutable();

    GenericArrayPtr params = type_parameters();

    if (MMA_UNLIKELY(params->is_null())) {
        params = doc_->new_array();
        datatype_->set_parameters(params->array_);
    }

    return params->append_datatype(name);
}

void Datatype::append_type_parameter(ValuePtr value)
{
    assert_not_null();
    assert_mutable();

    GenericArrayPtr params = type_parameters();

    if (MMA_UNLIKELY(params->is_null())) {
        params = doc_->new_array();
        datatype_->set_parameters(params->array_);
    }

    params->append(value);
}

void Datatype::append_constructor_argument(ValuePtr value)
{
    assert_not_null();
    assert_mutable();

    GenericArrayPtr ctr = constructor();

    if (MMA_UNLIKELY(ctr->is_null())) {
        ctr = doc_->new_array();
        datatype_->set_constructor(ctr->array_);
    }

    ctr->append(value);
}


GenericArrayPtr Datatype::set_constructor()
{
    assert_not_null();
    assert_mutable();

    auto ptr = doc_->new_array();
    datatype_->set_constructor(ptr->array_);

    return ptr;
}

}}
