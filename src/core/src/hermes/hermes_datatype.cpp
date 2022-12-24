
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

ObjectArray DatatypeView::constructor() const
{
    assert_not_null();
    if (datatype_->has_constructor()) {
        return ObjectArray(mem_holder_, datatype_->constructor());
    }
    else {
        return ObjectArray();
    }
}


ObjectArray DatatypeView::type_parameters() const
{
    assert_not_null();
    if (datatype_->is_parametric()) {
        return ObjectArray(mem_holder_, datatype_->parameters());
    }
    else {
        return ObjectArray();
    }
}


void DatatypeView::stringify(std::ostream& out, DumpFormatState& state) const
{
    if (datatype_)
    {
        auto& spec = state.cfg().spec();
        out << type_name();

        auto params = type_parameters();
        if (params.is_not_null())
        {
            out << "<" << spec.nl_start();
            bool first = true;

            state.push();
            for (size_t c = 0; c < params.size(); c++)
            {
                if (MMA_LIKELY(!first)) {
                    out << "," << spec.nl_middle();
                }
                else {
                    first = false;
                }

                state.make_indent(out);

                auto type_param = params.get(c);
                type_param.stringify(out, state);
            }
            state.pop();

            out << spec.nl_end();

            state.make_indent(out);
            out << ">";
        }

        auto ctr_args = constructor();
        if (ctr_args.is_not_null())
        {
            out << "(" << spec.nl_start();
            bool first = true;

            state.push();
            for (size_t c = 0; c < ctr_args.size(); c++)
            {
                if (MMA_LIKELY(!first)) {
                    out << "," << spec.nl_middle();
                }
                else {
                    first = false;
                }

                state.make_indent(out);

                auto value = ctr_args.get(c);
                value.stringify(out, state);
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


void DatatypeView::stringify_cxx(std::ostream& out,
               DumpFormatState& state) const
{
    if (datatype_)
    {
        auto& spec = state.cfg().spec();
        out << type_name();
        //println("TN: {}", *type_name()->view());

        auto params = type_parameters();
        if (params.is_not_null())
        {
            out << "<" << spec.nl_start();
            bool first = true;

            state.push();
            for (size_t c = 0; c < params.size(); c++)
            {
                if (MMA_LIKELY(!first)) {
                    out << "," << spec.nl_middle();
                }
                else {
                    first = false;
                }

                state.make_indent(out);

                Object type_param = params.get(c);
                if (type_param.is_a(TypeTag<Datatype>{})) {
                    cast_to<Datatype>(type_param).stringify_cxx(out, state);
                }
                else {
                    type_param.stringify(out, state);
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

bool DatatypeView::is_simple_layout() const {
    assert_not_null();

    bool sl = true;

    if (is_parametric()) {
        sl = type_parameters().is_simple_layout();
    }

    if (has_constructor()) {
        sl = sl && constructor().is_simple_layout();
    }

    return sl;
}

Datatype DatatypeView::append_type_parameter(U8StringView name)
{
    assert_not_null();
    assert_mutable();

    auto ctr = mem_holder_->ctr();

    ObjectArray params = type_parameters();
    if (MMA_UNLIKELY(params.is_null())) {
        params = ctr->make_object_array();
        datatype_->set_parameters(params.array_);
    }

    auto datatype = ctr->make_datatype(name);
    auto new_params = params.push_back(datatype.as_object());
    datatype_->set_parameters(new_params.array_);

    return datatype;
}


void DatatypeView::append_type_parameter(Object value)
{
    assert_not_null();
    assert_mutable();

    auto ctr = mem_holder_->ctr();

    ObjectArray params = type_parameters();
    if (MMA_UNLIKELY(params.is_null())) {
        params = ctr->make_object_array();
        datatype_->set_parameters(params.array_);
    }

    auto new_params = params.push_back(value);
    datatype_->set_parameters(new_params.array_);
}

void DatatypeView::append_constructor_argument(Object value)
{
    assert_not_null();
    assert_mutable();

    auto ctr0 = mem_holder_->ctr();

    ObjectArray ctr = constructor();

    if (MMA_UNLIKELY(ctr.is_null())) {
        ctr = ctr0->make_object_array();
        datatype_->set_constructor(ctr.array_);
    }

    auto new_ctr = ctr.push_back(value);
    datatype_->set_constructor(new_ctr.array_);
}


ObjectArray DatatypeView::set_constructor()
{
    assert_not_null();
    assert_mutable();

    auto ctr = mem_holder_->ctr();
    auto ptr = ctr->make_object_array();
    datatype_->set_constructor(ptr.array_);

    return ptr;
}

bool DatatypeView::operator==(const DatatypeView& other) const
{
    if (is_not_null() && other.is_not_null())
    {
        if (type_name() != other.type_name()) {
            return false;
        }

        if (constructor() != other.constructor()) {
            return false;
        }

        if (type_parameters() != other.type_parameters()) {
            return false;
        }

        return datatype_->extras() == other.datatype_->extras();
    }

    return is_null() && other.is_null();
}


UID256 DatatypeView::cxx_type_hash() const
{
    U8String cxx_type_decl = to_cxx_string();
    return get_cxx_type_hash(cxx_type_decl);
}

UID256 get_cxx_type_hash(U8StringView text)
{
    SHA256 hash;
    hash.add(text.data(), text.size());

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

hermes::Datatype strip_namespaces(hermes::Datatype src)
{
    auto ctr = HermesCtr::make_pooled();

    auto name = get_datatype_name(src.type_name());
    auto tgt = ctr->new_datatype(name);
    ctr->set_root(tgt.as_object());

    auto params = src.type_parameters();
    if (params.is_not_null())
    {
        for (size_t c = 0; c < params.size(); c++) {
            auto param = params.get(c);
            if (param.is_datatype()) {
                auto pp = strip_namespaces(param.as_datatype());
                tgt.append_type_parameter(pp.as_object());
            }
            else {
                tgt.append_type_parameter(param);
            }
        }
    }

    return tgt;
}



}}
