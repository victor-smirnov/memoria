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
#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/strings/format.hpp>

namespace memoria {
namespace v1 {


LDDumpState::LDDumpState(const LDDocumentView& doc)
{
    doc.for_each_named_type([&, this](auto type_id, auto type_decl){
        this->type_mapping_[type_decl.state_.get()] = type_id;
    });
}

void LDDocumentView::assert_identifier(U8StringView name)
{
    if (!is_identifier(name)) {
        MMA1_THROW(SDNParseException()) << fmt::format_ex(u"Supplied value '{}' is not a valid SDN identifier", name);
    }
}

SDN2Ptr<U8LinkedString> LDDocumentView::intern(U8StringView string)
{
    StringSet set;

    auto* sst = state();

    if (!sst->strings) {
        set = StringSet::create(arena_);
        state()->strings = set.ptr();
    }
    else {
        set = StringSet{arena_, sst->strings};
    }

    Optional<SDN2Ptr<U8LinkedString>> str = set.get(string);
    if (str)
    {
        return str.get();
    }
    else {
        SDN2Ptr<U8LinkedString> ss = allocate_tagged<U8LinkedString>(sizeof(LDDValueTag), arena_, string);
        set.put(ss);
        return ss;
    }
}


LDString LDDocumentView::new_string(U8StringView view)
{
    SDN2Ptr<U8LinkedString> ptr = intern(view);
    set_tag(ptr.get(), LDDValueTraits<LDString>::ValueTag);
    return LDString{this, ptr};
}


LDIdentifier LDDocumentView::new_identifier(U8StringView view)
{
    SDN2Ptr<U8LinkedString> ptr = intern(view);
    set_tag(ptr.get(), LDDValueTraits<LDIdentifier>::ValueTag);
    return LDIdentifier{this, ptr};
}


LDDValue LDDocumentView::new_integer(int64_t value)
{
    SDN2Ptr<int64_t> value_ptr = allocate_tagged<int64_t>(sizeof(LDDValueTag), arena_, value);
    set_tag(value_ptr.get(), LDDValueTraits<int64_t>::ValueTag);
    return LDDValue{this, value_ptr};
}


LDDValue LDDocumentView::new_double(double value)
{
    SDN2Ptr<double> value_ptr = allocate_tagged<double>(sizeof(LDDValueTag), arena_, value);
    set_tag(value_ptr.get(), LDDValueTraits<double>::ValueTag);
    return LDDValue{this, value_ptr};
}


LDDArray LDDocumentView::new_array(Span<LDDValue> span)
{
    Array array = Array::create_tagged(sizeof(LDDValueTag), arena_, span.length());
    set_tag(array.ptr(), LDDValueTraits<LDDArray>::ValueTag);

    for (const LDDValue& vv: span) {
        array.push_back(vv.value_ptr_);
    }

    return LDDArray(this, array.ptr());
}


LDDArray LDDocumentView::new_array()
{
    Array array = Array::create_tagged(sizeof(LDDValueTag), arena_, 4);
    set_tag(array.ptr(), LDDValueTraits<LDDArray>::ValueTag);

    return LDDArray(this, array.ptr());
}


LDDMap LDDocumentView::new_map()
{
    ValueMap value = ValueMap::create(arena_, sizeof(LDDValueTag));

    set_tag(value.ptr(), LDDValueTraits<LDDMap>::ValueTag);

    return LDDMap(this, value);
}


void LDDocumentView::set_value(LDDValue value)
{
    state()->value = value.value_ptr_;
}

void LDDocumentView::set(U8StringView string)
{
    SDN2Ptr<U8LinkedString> ss = allocate_tagged<U8LinkedString>(sizeof(LDDValueTag), arena_, string);
    set_tag(ss.get(), LDDValueTraits<LDString>::ValueTag);
    state()->value = ss.get();
}


void LDDocumentView::set(int64_t value)
{
    SDN2Ptr<int64_t> ss = allocate_tagged<int64_t>(sizeof(LDDValueTag), arena_, value);
    set_tag(ss.get(), LDDValueTraits<int64_t>::ValueTag);
    state()->value = ss;
}


void LDDocumentView::set(double value)
{
    SDN2Ptr<double> ss = allocate_tagged<double>(sizeof(LDDValueTag), arena_, value);
    set_tag(ss.get(), LDDValueTraits<double>::ValueTag);
    state()->value = ss;
}


LDDMap LDDocumentView::set_map()
{
    ValueMap value = ValueMap::create(arena_, sizeof(LDDValueTag));

    set_tag(value.ptr(), LDDValueTraits<LDDMap>::ValueTag);
    state()->value = value.ptr();

    return LDDMap(this, value);
}


LDDArray LDDocumentView::set_array()
{
    Array value = Array::create_tagged(sizeof(LDDValueTag), arena_, 4);
    set_tag(value.ptr(), LDDValueTraits<LDDArray>::ValueTag);

    state()->value = value.ptr();

    return LDDArray(this, value);
}

LDDValue LDDocumentView::set_sdn(U8StringView sdn)
{
    LDDValue value = parse_raw_value(sdn.begin(), sdn.end());
    state()->value = value.value_ptr_;
    return value;
}

LDTypeDeclaration LDDocumentView::new_type_declaration(U8StringView name)
{
    auto td_ptr = allocate_tagged<sdn2_::TypeDeclState>(sizeof(LDDValueTag), arena_, sdn2_::TypeDeclState{0, 0, 0});
    set_tag(td_ptr.get(), LDDValueTraits<LDTypeDeclaration>::ValueTag);

    auto ss_ptr = intern(name);
    td_ptr.get(arena_)->name = ss_ptr;

    return LDTypeDeclaration(this, td_ptr);
}

LDTypeDeclaration LDDocumentView::new_type_declaration(LDIdentifier name)
{
    LDTypeDeclaration td = new_detached_type_declaration(name);
    return td;
}

LDTypeDeclaration LDDocumentView::new_detached_type_declaration(LDIdentifier name)
{
    auto td_ptr = allocate_tagged<sdn2_::TypeDeclState>(sizeof(LDDValueTag), arena_, sdn2_::TypeDeclState{0, 0, 0});
    set_tag(td_ptr.get(), LDDValueTraits<LDTypeDeclaration>::ValueTag);

    auto ss_ptr = name.string_;
    td_ptr.get(arena_)->name = ss_ptr;
    return LDTypeDeclaration(this, td_ptr);
}

LDDTypedValue LDDocumentView::new_typed_value(LDTypeDeclaration typedecl, LDDValue ctr_value)
{
    SDN2Ptr<sdn2_::TypedValueState> ss = allocate_tagged<sdn2_::TypedValueState>(
        sizeof(LDDValueTag), arena_, sdn2_::TypedValueState{typedecl.state_, ctr_value.value_ptr_}
    );

    set_tag(ss.get(), LDDValueTraits<LDDTypedValue>::ValueTag);

    return LDDTypedValue{this, ss};
}

void LDDocumentView::for_each_named_type(std::function<void (U8StringView name, LDTypeDeclaration)> fn) const
{
    auto* sst = state();
    if (sst->type_directory)
    {
        TypeDeclsMap decls = TypeDeclsMap::get(arena_, sst->type_directory);
        decls.for_each([&](auto name_ptr, auto decl_ptr){
            auto name = name_ptr.get(arena_)->view();
            LDTypeDeclaration td{const_cast<LDDocumentView*>(this), decl_ptr};
            fn(name, td);
        });
    }
}

void LDDocumentView::set_named_type_declaration(LDIdentifier name, LDTypeDeclaration type_decl)
{
    TypeDeclsMap map = ensure_type_decls_exist();
    map.put(name.string_.get(), type_decl.state_);
}


Optional<LDTypeDeclaration> LDDocumentView::get_named_type_declaration(U8StringView name) const
{
    auto* state = this->state();
    if (state->type_directory)
    {
        TypeDeclsMap map = TypeDeclsMap::get(arena_, state->type_directory);

        auto value = map.get(name);

        if (value) {
            return LDTypeDeclaration{const_cast<LDDocumentView*>(this), value.get()};
        }
    }

    return Optional<LDTypeDeclaration>{};
}

LDTypeDeclaration LDDocumentView::create_named_type(U8StringView name, U8StringView type_decl)
{
    assert_identifier(name);

    TypeDeclsMap map = ensure_type_decls_exist();
    auto str_ptr = this->intern(name);
    LDTypeDeclaration td = parse_raw_type_decl(type_decl.begin(), type_decl.end());
    map.put(str_ptr, td.state_);
    return td;
}




void LDDocumentView::remove_named_type_declaration(U8StringView name)
{
    auto* sst = state();
    if (sst->type_directory)
    {
        TypeDeclsMap decls = TypeDeclsMap::get(arena_, sst->type_directory);
        decls.remove(name);
    }
}


std::ostream& LDDocumentView::dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const {

    if (has_type_dictionary()) {
        do_dump_dictionary(out, state, dump_state);
    }

    value().dump(out, state, dump_state);
    return out;
}


void LDDocumentView::do_dump_dictionary(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const
{
    out << "#{" << state.nl_start();

    bool first = true;

    state.push();
    for_each_named_type([&](auto name, auto td){
        if (MMA1_LIKELY(!first)) {
            out << "," << state.nl_middle();
        }
        else {
            first = false;
        }

        state.make_indent(out);
        out << name << ": ";
        td.dump(out, state, dump_state);
    });
    state.pop();

    out << state.nl_end();
    state.make_indent(out);
    out << "}";
}


}}
