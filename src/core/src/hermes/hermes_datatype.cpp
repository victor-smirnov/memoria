
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

#include <hash-library/sha256.h>

namespace memoria {
namespace hermes {

ObjectArrayPtr Datatype::constructor() const
{
    assert_not_null();
    if (datatype_->has_constructor()) {
        return ObjectArrayPtr(ObjectArray(datatype_->constructor(), doc_, ptr_holder_));
    }
    else {
        return ObjectArrayPtr(ObjectArray(nullptr, doc_, ptr_holder_));
    }
}


ObjectArrayPtr Datatype::type_parameters() const
{
    assert_not_null();
    if (datatype_->is_parametric()) {
        return ObjectArrayPtr(ObjectArray(datatype_->parameters(), doc_, ptr_holder_));
    }
    else {
        return ObjectArrayPtr(ObjectArray(nullptr, doc_, ptr_holder_));
    }
}


void Datatype::stringify(std::ostream& out, DumpFormatState& state) const
{
    if (datatype_)
    {
        auto& spec = state.cfg().spec();
        out << *type_name()->view();

        auto params = type_parameters();
        if (params->is_not_null())
        {
            out << "<" << spec.nl_start();
            bool first = true;

            state.push();
            for (size_t c = 0; c < params->size(); c++)
            {
                if (MMA_LIKELY(!first)) {
                    out << "," << spec.nl_middle();
                }
                else {
                    first = false;
                }

                state.make_indent(out);

                auto type_param = params->get(c);
                type_param->stringify(out, state);
            }
            state.pop();

            out << spec.nl_end();

            state.make_indent(out);
            out << ">";
        }

        auto ctr_args = constructor();
        if (ctr_args->is_not_null())
        {
            out << "(" << spec.nl_start();
            bool first = true;

            state.push();
            for (size_t c = 0; c < ctr_args->size(); c++)
            {
                if (MMA_LIKELY(!first)) {
                    out << "," << spec.nl_middle();
                }
                else {
                    first = false;
                }

                state.make_indent(out);

                auto value = ctr_args->get(c);
                value->stringify(out, state);
            }
            state.pop();

            out << spec.nl_end();

            state.make_indent(out);
            out << ")";
        }

        for (uint64_t c = 0; c < datatype_->extras().pointers_size(); c++)
        {
            PtrQualifier qual = datatype_->extras().pointer(c);

            if (qual.is_const()) {
                out <<" const";
            }

            if (qual.is_volatile()) {
                out <<" volatile";
            }

            out << "*";
        }

        if (datatype_->extras().is_const()) {
            out <<" const";
        }

        if (datatype_->extras().is_volatile()) {
            out <<" volatile";
        }

        for (uint64_t c = 0; c < datatype_->extras().refs_size(); c++)
        {
            out << "&";
        }
    }
    else {
        out << "null";
    }
}


void Datatype::stringify_cxx(std::ostream& out,
               DumpFormatState& state) const
{
    if (datatype_)
    {
        auto& spec = state.cfg().spec();
        out << *type_name()->view();

        auto params = type_parameters();
        if (params->is_not_null())
        {
            out << "<" << spec.nl_start();
            bool first = true;

            state.push();
            for (size_t c = 0; c < params->size(); c++)
            {
                if (MMA_LIKELY(!first)) {
                    out << "," << spec.nl_middle();
                }
                else {
                    first = false;
                }

                state.make_indent(out);

                ObjectPtr type_param = params->get(c);
                if (type_param->is_a(TypeTag<Datatype>{})) {
                    cast_to<Datatype>(type_param)->stringify_cxx(out, state);
                }
                else {
                    type_param->stringify(out, state);
                }
            }
            state.pop();

            out << spec.nl_end();

            state.make_indent(out);
            out << ">";
        }


        for (uint64_t c = 0; c < datatype_->extras().pointers_size(); c++)
        {
            PtrQualifier qual = datatype_->extras().pointer(c);

            if (qual.is_const()) {
                out <<" const";
            }

            if (qual.is_volatile()) {
                out <<" volatile";
            }

            out << "*";
        }

        if (datatype_->extras().is_const()) {
            out <<" const";
        }

        if (datatype_->extras().is_volatile()) {
            out <<" volatile";
        }

        for (uint64_t c = 0; c < datatype_->extras().refs_size(); c++)
        {
            out << "&";
        }
    }
    else {
        out << "null";
    }
}

bool Datatype::is_simple_layout() const {
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

    ObjectArrayPtr params = type_parameters();
    if (MMA_UNLIKELY(params->is_null())) {
        params = doc_->new_array();
        datatype_->set_parameters(params->array_);
    }

    auto datatype = doc_->new_datatype(name);
    params->append(datatype->as_object());
    return datatype;
}

DatatypePtr Datatype::append_type_parameter(StringValuePtr name)
{
    assert_not_null();
    assert_mutable();

    ObjectArrayPtr params = type_parameters();
    if (MMA_UNLIKELY(params->is_null())) {
        params = doc_->new_array();
        datatype_->set_parameters(params->array_);
    }

    auto datatype = doc_->new_datatype(name);
    params->append(datatype->as_object());
    return datatype;
}

void Datatype::append_type_parameter(ObjectPtr value)
{
    assert_not_null();
    assert_mutable();

    ObjectArrayPtr params = type_parameters();
    if (MMA_UNLIKELY(params->is_null())) {
        params = doc_->new_array();
        datatype_->set_parameters(params->array_);
    }

    params->append(value);
}

void Datatype::append_constructor_argument(ObjectPtr value)
{
    assert_not_null();
    assert_mutable();

    ObjectArrayPtr ctr = constructor();

    if (MMA_UNLIKELY(ctr->is_null())) {
        ctr = doc_->new_array();
        datatype_->set_constructor(ctr->array_);
    }

    ctr->append(value);
}


ObjectArrayPtr Datatype::set_constructor()
{
    assert_not_null();
    assert_mutable();

    auto ptr = doc_->new_array();
    datatype_->set_constructor(ptr->array_);

    return ptr;
}

UID256 Datatype::cxx_type_hash() const
{
    SHA256 hash;
    U8String cxx_type_decl = to_cxx_string();
    hash.add(cxx_type_decl.data(), cxx_type_decl.size());

    uint8_t hash_buf[SHA256::HashBytes];
    hash.getHash(hash_buf);

    UID256 uid;

    auto atoms = uid.atoms();
    for (int atom = 0; atom < 4; atom++)
    {
        atoms[atom] = 0;
        for (size_t c = 0; c < 8; c++) {
            atoms[atom] |= ((uint64_t)hash_buf[c + atom * 8]) << (c * 8);
        }
    }

    uid.set_type(UID256::Type::TYPE1);

    return uid;
}

}}
