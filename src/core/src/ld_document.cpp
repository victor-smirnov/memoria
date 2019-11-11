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

ld_::LDPtr<U8LinkedString> LDDocumentView::intern(U8StringView string)
{
    StringSet set;

    auto* sst = state_mutable();

    if (!sst->strings) {
        set = StringSet::create(&arena_);
        state_mutable()->strings = set.ptr();
    }
    else {
        set = StringSet{&arena_, sst->strings};
    }

    Optional<ld_::LDPtr<U8LinkedString>> str = set.get(string);
    if (str)
    {
        return str.get();
    }
    else {
        ld_::LDPtr<U8LinkedString> ss = allocate_tagged<U8LinkedString>(sizeof(LDDValueTag), &arena_, string);
        ld_::ldd_set_tag(&arena_, ss.get(), LDDValueTraits<LDString>::ValueTag);
        set.put(ss);
        return ss;
    }
}


LDString LDDocumentView::new_string(U8StringView view)
{
    ld_::LDPtr<U8LinkedString> ptr = intern(view);
    return LDString{this, ptr};
}


LDIdentifier LDDocumentView::new_identifier(U8StringView view)
{
    ld_::LDPtr<U8LinkedString> ptr = intern(view);
    return LDIdentifier{this, ptr};
}


LDDValue LDDocumentView::new_integer(LDInteger value)
{
    ld_::LDPtr<ld_::LDIntegerStorage> value_ptr = allocate_tagged<ld_::LDIntegerStorage>(sizeof(LDDValueTag), arena_.make_mutable(), value);
    set_tag(value_ptr.get(), LDDValueTraits<LDInteger>::ValueTag);
    return LDDValue{this, value_ptr, LDDValueTraits<LDInteger>::ValueTag};
}

LDDValue LDDocumentView::new_boolean(LDBoolean value)
{
    ld_::LDPtr<ld_::LDBooleanStorage> value_ptr = allocate_tagged<ld_::LDBooleanStorage>(sizeof(LDDValueTag), arena_.make_mutable(), value);
    set_tag(value_ptr.get(), LDDValueTraits<LDBoolean>::ValueTag);
    return LDDValue{this, value_ptr, LDDValueTraits<LDBoolean>::ValueTag};
}


LDDValue LDDocumentView::new_double(double value)
{
    ld_::LDPtr<ld_::LDDoubleStorage> value_ptr = allocate_tagged<ld_::LDDoubleStorage>(sizeof(LDDValueTag), arena_.make_mutable(), value);
    set_tag(value_ptr.get(), LDDValueTraits<LDDouble>::ValueTag);
    return LDDValue{this, value_ptr, LDDValueTraits<LDDouble>::ValueTag};
}


LDDArray LDDocumentView::new_array(Span<LDDValue> span)
{
    Array array = Array::create_tagged(sizeof(LDDValueTag), arena_.make_mutable(), span.length());
    set_tag(array.ptr(), LDDValueTraits<LDDArray>::ValueTag);

    for (const LDDValue& vv: span) {
        array.push_back(vv.value_ptr_);
    }

    return LDDArray(this, array.ptr());
}


LDDArray LDDocumentView::new_array()
{
    Array array = Array::create_tagged(sizeof(LDDValueTag), arena_.make_mutable(), 4);
    set_tag(array.ptr(), LDDValueTraits<LDDArray>::ValueTag);

    return LDDArray(this, array.ptr());
}


LDDMap LDDocumentView::new_map()
{
    ValueMap value = ValueMap::create(arena_.make_mutable(), sizeof(LDDValueTag));

    set_tag(value.ptr(), LDDValueTraits<LDDMap>::ValueTag);

    return LDDMap(this, value);
}


void LDDocumentView::set_value(LDDValue value)
{
    state_mutable()->value = value.value_ptr_;
}

void LDDocumentView::set(U8StringView string)
{
    ld_::LDPtr<U8LinkedString> ss = allocate_tagged<U8LinkedString>(sizeof(LDDValueTag), arena_.make_mutable(), string);
    set_tag(ss.get(), LDDValueTraits<LDString>::ValueTag);
    state_mutable()->value = ss.get();
}


void LDDocumentView::set(int64_t value)
{
    ld_::LDPtr<int64_t> ss = allocate_tagged<int64_t>(sizeof(LDDValueTag), arena_.make_mutable(), value);
    set_tag(ss.get(), LDDValueTraits<int64_t>::ValueTag);
    state_mutable()->value = ss;
}


void LDDocumentView::set(double value)
{
    ld_::LDPtr<double> ss = allocate_tagged<double>(sizeof(LDDValueTag), arena_.make_mutable(), value);
    set_tag(ss.get(), LDDValueTraits<double>::ValueTag);
    state_mutable()->value = ss;
}


LDDMap LDDocumentView::set_map()
{
    ValueMap value = ValueMap::create(arena_.make_mutable(), sizeof(LDDValueTag));

    set_tag(value.ptr(), LDDValueTraits<LDDMap>::ValueTag);
    state_mutable()->value = value.ptr();

    return LDDMap(this, value);
}


LDDArray LDDocumentView::set_array()
{
    Array value = Array::create_tagged(sizeof(LDDValueTag), arena_.make_mutable(), 4);
    set_tag(value.ptr(), LDDValueTraits<LDDArray>::ValueTag);

    state_mutable()->value = value.ptr();

    return LDDArray(this, value);
}

LDDValue LDDocumentView::set_sdn(U8StringView sdn)
{
    LDDValue value = parse_raw_value(sdn.begin(), sdn.end());
    state_mutable()->value = value.value_ptr_;
    return value;
}

LDTypeDeclaration LDDocumentView::new_type_declaration(U8StringView name)
{
    auto td_ptr = allocate_tagged<ld_::TypeDeclState>(sizeof(LDDValueTag), arena_.make_mutable(), ld_::TypeDeclState{0, 0, 0});
    set_tag(td_ptr.get(), LDDValueTraits<LDTypeDeclaration>::ValueTag);

    auto ss_ptr = intern(name);
    td_ptr.get_mutable(&arena_)->name = ss_ptr;

    return LDTypeDeclaration(this, td_ptr);
}

LDTypeDeclaration LDDocumentView::new_type_declaration(LDIdentifier name)
{
    LDTypeDeclaration td = new_detached_type_declaration(name);
    return td;
}

LDTypeDeclaration LDDocumentView::new_detached_type_declaration(LDIdentifier name)
{
    auto td_ptr = allocate_tagged<ld_::TypeDeclState>(sizeof(LDDValueTag), arena_.make_mutable(), ld_::TypeDeclState{0, 0, 0});
    set_tag(td_ptr.get(), LDDValueTraits<LDTypeDeclaration>::ValueTag);

    auto ss_ptr = name.string_;
    td_ptr.get_mutable(&arena_)->name = ss_ptr;
    return LDTypeDeclaration(this, td_ptr);
}

LDDTypedValue LDDocumentView::new_typed_value(LDTypeDeclaration typedecl, LDDValue ctr_value)
{
    ld_::LDPtr<ld_::TypedValueState> ss = allocate_tagged<ld_::TypedValueState>(
        sizeof(LDDValueTag), arena_.make_mutable(), ld_::TypedValueState{typedecl.state_, ctr_value.value_ptr_}
    );

    set_tag(ss.get(), LDDValueTraits<LDDTypedValue>::ValueTag);

    return LDDTypedValue{this, ss};
}

void LDDocumentView::for_each_named_type(std::function<void (U8StringView name, LDTypeDeclaration)> fn) const
{
    auto* sst = state();
    if (sst->type_directory)
    {
        TypeDeclsMap decls = TypeDeclsMap::get(&arena_, sst->type_directory);
        decls.for_each([&](auto name_ptr, auto decl_ptr){
            auto name = name_ptr.get(&arena_)->view();
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

void LDDocumentView::set_named_type_declaration(U8StringView name, LDTypeDeclaration type_decl)
{
    TypeDeclsMap map = ensure_type_decls_exist();
    auto str_ptr = this->intern(name);
    map.put(str_ptr.get(), type_decl.state_);
}


Optional<LDTypeDeclaration> LDDocumentView::get_named_type_declaration(U8StringView name) const
{
    auto* state = this->state();
    if (state->type_directory)
    {
        TypeDeclsMap map = TypeDeclsMap::get(&arena_, state->type_directory);

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
        TypeDeclsMap decls = TypeDeclsMap::get(&arena_, sst->type_directory);
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



struct StringsDeepCopyHelperBase {

    using ElementT = U8LinkedString;

    template <typename State>
    ld_::LDPtr<State> allocate_root(ld_::LDArenaView* dst, const State& state)
    {
        return allocate<State>(dst, state);
    }

    ld_::LDPtr<ElementT> do_deep_copy(
            ld_::LDArenaView* dst,
            const ld_::LDArenaView* src,
            ld_::LDPtr<ElementT> element,
            ld_::LDArenaAddressMapping& mapping
    )
    {
        const ElementT* src_str = element.get(src);

        ld_::LDPtr<ElementT> dst_str = allocate_tagged<ElementT>(sizeof(LDDValueTag), dst, src_str);
        ld_::ldd_set_tag(dst, dst_str.get(), LDDValueTraits<LDString>::ValueTag);

        return dst_str;
    }
};




class TypeDeclsDeepCopyHelperBase {

    using Key = U8LinkedString;
    using Value = ld_::TypeDeclState;

    const LDDocumentView* src_doc_;
    LDDocumentView* dst_doc_;

public:

    TypeDeclsDeepCopyHelperBase(const LDDocumentView* src_doc, LDDocumentView* dst_doc):
        src_doc_(src_doc), dst_doc_(dst_doc)
    {}


    template <typename State>
    ld_::LDPtr<State> allocate_root(ld_::LDArenaView* dst, const State& state)
    {
        return allocate<State>(dst, state);
    }

    ld_::LDPtr<Key> do_deep_copy(
            ld_::LDArenaView* dst,
            const ld_::LDArenaView* src,
            ld_::LDPtr<Key> element,
            ld_::LDArenaAddressMapping& mapping
    )
    {
        const Key* src_str = element.get(src);

        ld_::LDPtr<Key> dst_str = allocate_tagged<Key>(sizeof(LDDValueTag), dst, src_str);
        ld_::ldd_set_tag(dst, dst_str.get(), LDDValueTraits<LDString>::ValueTag);

        return dst_str;
    }

    ld_::LDPtr<Value> do_deep_copy(
            ld_::LDArenaView* dst,
            const ld_::LDArenaView* src,
            ld_::LDPtr<Value> element,
            ld_::LDArenaAddressMapping& mapping
    )
    {
        LDTypeDeclaration src_td(src_doc_, element);
        return src_td.deep_copy_to(dst_doc_, mapping);
    }
};




void LDDocument::compactify()
{
    LDDocument tgt;

    const DocumentState* my_state = this->state();

    ld_::LDArenaAddressMapping address_mapping;

    if (my_state->strings)
    {
        StringSet strings = StringSet::get(&arena_, my_state->strings);

        ld_::DeepCopyHelper<StringsDeepCopyHelperBase> helper(address_mapping);

        auto strings_state = strings.deep_copy_to(tgt.ld_arena_.view(), helper);
        tgt.state_mutable()->strings = strings_state;
    }

    if (my_state->type_directory)
    {
        TypeDeclsMap type_decls = TypeDeclsMap::get(&arena_, my_state->type_directory);

        ld_::DeepCopyHelper<TypeDeclsDeepCopyHelperBase> helper(address_mapping, this, &tgt);

        auto type_decls_state = type_decls.deep_copy_to(tgt.ld_arena_.view(), helper);
        tgt.state_mutable()->type_directory = type_decls_state;
    }

    if (my_state->value) {
        LDDValue value(this, my_state->value);
        LDDValue new_value(&tgt, value.deep_copy_to(&tgt, address_mapping));
        tgt.set_value(new_value);
    };

    (*this) = std::move(tgt);
}

LDDocument& LDDocument::operator=(LDDocument&& src)
{
    if (this != &src)
    {
        ld_arena_.move_data_from(std::move(src.ld_arena_));
        src.ld_arena_.reset_view();
    }

    return *this;
}


}}
