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
#include <memoria/v1/core/linked/document/ld_datatype.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/strings/format.hpp>

#include <memoria/v1/core/tools/bitmap.hpp>

#include <memoria/v1/core/datatypes/type_registry.hpp>

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

ld_::LDPtr<DTTLDStorageType<LDStringView>> LDDocumentView::intern(DTTViewType<LDStringView> string)
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

    using StringStorageT = DTTLDStorageType<LDStringView>;

    Optional<ld_::LDPtr<StringStorageT>> str = set.get(string);
    if (str)
    {
        return str.get();
    }
    else {
        ld_::LDPtr<StringStorageT> ss = allocate_tagged<StringStorageT>(ld_tag_size<LDStringView>(), &arena_, string);
        ld_::ldd_set_tag(&arena_, ss.get(), ld_tag_value<LDStringView>());
        set.put(ss);
        return ss;
    }
}


LDStringView LDDocumentView::new_string(DTTViewType<LDStringView> view)
{
    auto opt_ptr = is_shared(view);
    if (MMA1_UNLIKELY((bool)opt_ptr))
    {
        return LDStringView{this, opt_ptr.get()};
    }
    else {
        return new_raw_value<LDStringView>(view);
    }
}


LDIdentifierView LDDocumentView::new_identifier(U8StringView view)
{
    auto opt_ptr = is_shared(view);
    if (MMA1_UNLIKELY((bool)opt_ptr))
    {
        return LDIdentifierView{this, opt_ptr.get()};
    }
    else {
        return LDIdentifierView{this, new_raw_value_raw<LDStringView>(view)};
    }
}


LDDValueView LDDocumentView::new_integer(LDInteger value)
{
    ld_::LDPtr<ld_::LDIntegerStorage> value_ptr = allocate_tagged<ld_::LDIntegerStorage>(
                ld_tag_size<LDInteger>(), arena_.make_mutable(), value
    );
    set_tag(value_ptr.get(), ld_tag_value<LDInteger>());
    return LDDValueView{this, value_ptr, ld_tag_value<LDInteger>()};
}

LDDValueView LDDocumentView::new_boolean(LDBoolean value)
{
    ld_::LDPtr<ld_::LDBooleanStorage> value_ptr = allocate_tagged<ld_::LDBooleanStorage>(
                ld_tag_size<LDBoolean>(), arena_.make_mutable(), value
    );
    set_tag(value_ptr.get(), ld_tag_value<LDBoolean>());
    return LDDValueView{this, value_ptr, ld_tag_value<LDBoolean>()};
}


LDDValueView LDDocumentView::new_double(double value)
{
    ld_::LDPtr<ld_::LDDoubleStorage> value_ptr = allocate_tagged<ld_::LDDoubleStorage>(
                ld_tag_size<LDDouble>(), arena_.make_mutable(), value
    );
    set_tag(value_ptr.get(), ld_tag_value<LDDouble>());
    return LDDValueView{this, value_ptr, ld_tag_value<LDDouble>()};
}


LDDArrayView LDDocumentView::new_array(Span<LDDValueView> span)
{
    Array array = Array::create_tagged(ld_tag_size<LDDArrayView>(), arena_.make_mutable(), span.length());
    set_tag(array.ptr(), ld_tag_value<LDDArrayView>());

    for (const LDDValueView& vv: span) {
        array.push_back(vv.value_ptr_);
    }

    return LDDArrayView(this, array.ptr());
}


LDDArrayView LDDocumentView::new_array()
{
    return new_value<LDDArrayView>();
}


LDDMapView LDDocumentView::new_map()
{
    return new_value<LDDMapView>();
}


void LDDocumentView::set_doc_value(LDDValueView value) noexcept
{
    state_mutable()->value = value.value_ptr_;
}

void LDDocumentView::set_string(U8StringView string)
{
    ld_::LDPtr<U8LinkedString> ss = allocate_tagged<U8LinkedString>(
        ld_tag_size<LDStringView>(), arena_.make_mutable(), string
    );
    set_tag(ss.get(), ld_tag_value<LDStringView>());
    state_mutable()->value = ss.get();
}


void LDDocumentView::set_integer(int64_t value)
{
    ld_::LDPtr<int64_t> ss = allocate_tagged<int64_t>(ld_tag_size<LDInteger>(), arena_.make_mutable(), value);
    set_tag(ss.get(), ld_tag_value<LDInteger>());
    state_mutable()->value = ss;
}


void LDDocumentView::set_double(double value)
{
    ld_::LDPtr<double> ss = allocate_tagged<double>(ld_tag_size<LDDouble>(), arena_.make_mutable(), value);
    set_tag(ss.get(), ld_tag_value<LDDouble>());
    state_mutable()->value = ss;
}


void LDDocumentView::set_boolean(bool value)
{
    ld_::LDPtr<LDBoolean> ss = allocate_tagged<LDBoolean>(ld_tag_size<LDBoolean>(), arena_.make_mutable(), value);
    set_tag(ss.get(), ld_tag_value<LDBoolean>());
    state_mutable()->value = ss;
}

void LDDocumentView::set_null()
{
    state_mutable()->value = 0;
}

LDDMapView LDDocumentView::set_map()
{
    ValueMap value = ValueMap::create(arena_.make_mutable(), ld_tag_size<LDDMapView>());

    set_tag(value.ptr(), ld_tag_value<LDDMapView>());
    state_mutable()->value = value.ptr();

    return LDDMapView(this, value);
}


LDDArrayView LDDocumentView::set_array()
{
    return set_value<LDDArrayView>();
}

LDDValueView LDDocumentView::set_sdn(U8StringView sdn)
{
    LDDValueView value = parse_raw_value(sdn.begin(), sdn.end());
    state_mutable()->value = value.value_ptr_;
    return value;
}

void LDDocumentView::set_document(const LDDocumentView& source)
{
    ld_::assert_different_docs(this, &source);

    LDDocumentView* dst_doc = this->make_mutable();

    ld_::LDArenaAddressMapping mapping(source, *dst_doc);
    ld_::LDDPtrHolder ptr = source.value().deep_copy_to(dst_doc, mapping);

    set_doc_value(LDDValueView{this, ptr});
}

LDTypeDeclarationView LDDocumentView::new_type_declaration(U8StringView name)
{
    auto td_ptr = allocate_tagged<ld_::TypeDeclState>(ld_tag_size<LDTypeDeclarationView>(), arena_.make_mutable(), ld_::TypeDeclState{0, 0, 0, 0});
    set_tag(td_ptr.get(), ld_tag_value<LDTypeDeclarationView>());

    auto ss_ptr = new_string(name).ptr();
    td_ptr.get_mutable(&arena_)->name = ss_ptr;

    return LDTypeDeclarationView(this, td_ptr);
}

LDTypeDeclarationView LDDocumentView::new_type_declaration(LDIdentifierView name)
{
    LDTypeDeclarationView td = new_detached_type_declaration(name);
    return td;
}

LDTypeDeclarationView LDDocumentView::new_detached_type_declaration(LDIdentifierView name)
{


    auto td_ptr = allocate_tagged<ld_::TypeDeclState>(ld_tag_size<LDTypeDeclarationView>(), arena_.make_mutable(), ld_::TypeDeclState{0, 0, 0});
    set_tag(td_ptr.get(), ld_tag_value<LDTypeDeclarationView>());

    auto ss_ptr = name.string_;
    td_ptr.get_mutable(&arena_)->name = ss_ptr;
    return LDTypeDeclarationView(this, td_ptr);
}

LDDValueView LDDocumentView::new_typed_value(LDTypeDeclarationView typedecl, LDDValueView ctr_value)
{
    U8String cxx_typedecl = typedecl.to_cxx_typedecl();

    auto ops = DataTypeRegistry::local().get_operations(cxx_typedecl);
    if (ops) {
        LDPtrHolder ptr = ops.get()->construct_from(this, ctr_value);
        return LDDValueView{this, ptr};
    }
    else {
        ld_::LDPtr<ld_::TypedValueState> ss = allocate_tagged<ld_::TypedValueState>(
            ld_tag_size<LDDTypedValueView>(), arena_.make_mutable(), ld_::TypedValueState{typedecl.state_, ctr_value.value_ptr_}
        );

        set_tag(ss.get(), ld_tag_value<LDDTypedValueView>());

        return LDDTypedValueView{this, ss};
    }
}

void LDDocumentView::for_each_named_type(std::function<void (U8StringView name, LDTypeDeclarationView)> fn) const
{
    auto* sst = state();
    if (sst->type_directory)
    {
        TypeDeclsMap decls = TypeDeclsMap::get(&arena_, sst->type_directory);
        decls.for_each([&](auto name_ptr, auto decl_ptr){
            auto name = name_ptr.get(&arena_)->view();
            LDTypeDeclarationView td{const_cast<LDDocumentView*>(this), decl_ptr};
            fn(name, td);
        });
    }
}

std::vector<std::pair<U8StringView, LDTypeDeclarationView>> LDDocumentView::named_types() const
{
    std::vector<std::pair<U8StringView, LDTypeDeclarationView>> types;
    for_each_named_type([&](auto name, auto tdecl){
        types.push_back(std::make_pair(name, tdecl));
    });
    return types;
}

void LDDocumentView::set_named_type_declaration(LDIdentifierView name, LDTypeDeclarationView type_decl)
{
    TypeDeclsMap map = ensure_type_decls_exist();
    map.put(name.string_.get(), type_decl.state_);
}

void LDDocumentView::set_named_type_declaration(U8StringView name, LDTypeDeclarationView type_decl)
{
    TypeDeclsMap map = ensure_type_decls_exist();
    auto str_ptr = this->new_string(name).ptr();
    map.put(str_ptr, type_decl.state_);
}


Optional<LDTypeDeclarationView> LDDocumentView::get_named_type_declaration(U8StringView name) const
{
    auto* state = this->state();
    if (state->type_directory)
    {
        TypeDeclsMap map = TypeDeclsMap::get(&arena_, state->type_directory);

        auto value = map.get(name);

        if (value) {
            return LDTypeDeclarationView{const_cast<LDDocumentView*>(this), value.get()};
        }
    }

    return Optional<LDTypeDeclarationView>{};
}

LDTypeDeclarationView LDDocumentView::create_named_type(U8StringView name, U8StringView type_decl)
{
    assert_identifier(name);

    TypeDeclsMap map = ensure_type_decls_exist();
    auto str_ptr = this->new_string(name).ptr();
    LDTypeDeclarationView td = parse_raw_type_decl(type_decl.begin(), type_decl.end());
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

void LDDocumentView::add_shared_string(U8StringView string)
{
    intern(string);
}

Optional<ld_::LDPtr<DTTLDStorageType<LDStringView>>> LDDocumentView::is_shared(DTTViewType<LDStringView> string) const
{
    auto* sst = state();

    if (!sst->strings) {
        return Optional<ld_::LDPtr<DTTLDStorageType<LDStringView>>>{};
    }
    else {
        StringSet set = StringSet{&arena_, sst->strings};

        using StringStorageT = DTTLDStorageType<LDStringView>;

        Optional<ld_::LDPtr<StringStorageT>> str = set.get(string);
        if (str)
        {
            return Optional<ld_::LDPtr<DTTLDStorageType<LDStringView>>>{str.get()};
        }
        else {
            return Optional<ld_::LDPtr<DTTLDStorageType<LDStringView>>>{};
        }
    }
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

        ld_::LDPtr<ElementT> dst_str = allocate_tagged<ElementT>(ld_tag_size<LDStringView>(), dst, src_str);
        ld_::ldd_set_tag(dst, dst_str.get(), ld_tag_value<LDStringView>());

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

        ld_::LDPtr<Key> dst_str = allocate_tagged<Key>(ld_tag_size<LDStringView>(), dst, src_str);
        ld_::ldd_set_tag(dst, dst_str.get(), ld_tag_value<LDStringView>());

        return dst_str;
    }

    ld_::LDPtr<Value> do_deep_copy(
            ld_::LDArenaView* dst,
            const ld_::LDArenaView* src,
            ld_::LDPtr<Value> element,
            ld_::LDArenaAddressMapping& mapping
    )
    {
        LDTypeDeclarationView src_td(src_doc_, element);
        return src_td.deep_copy_to(dst_doc_, mapping);
    }
};


LDDocument::LDDocument(U8StringView sdn): LDDocument() {
    set_sdn(sdn);
}

LDDocument::LDDocument(const char* sdn): LDDocument() {
    set_sdn(sdn);
}

LDDocument LDDocument::compactify() const
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
        LDDValueView value(this, my_state->value);
        LDDValueView new_value(&tgt, value.deep_copy_to(&tgt, address_mapping));
        tgt.set_doc_value(new_value);
    };

    return tgt;
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

void LDDocument::clear() {
    arena_.clear_arena(INITIAL_ARENA_SIZE);
    allocate_state();
}

void LDDocument::reset() {
    arena_.reset_arena(INITIAL_ARENA_SIZE);
    allocate_state();
}



std::ostream& operator<<(std::ostream& out, const LDDocumentView& doc)
{
    LDDumpFormatState fmt = LDDumpFormatState().simple();
    doc.dump(out, fmt);
    return out;
}



void LDDocumentStorage::destroy() noexcept
{
    this->~LDDocumentStorage();
    free_system(this);
}

LDDocumentStorage* LDDocumentStorage::create(ViewType view)
{
    size_t storage_class_size = sizeof(LDDocumentStorage) | 0x7; // + up to 7 bytes of alignment

    auto descr = DataTypeTraits<LDDocument>::describe_data(view);
    auto& span = std::get<0>(descr);

    uint8_t* ptr = allocate_system<uint8_t>(storage_class_size + span.size()).release();

    MemCpyBuffer(span.data(), ptr + storage_class_size, span.size());

    try {
        return new (ptr) LDDocumentStorage(ViewType{Span<uint8_t>(ptr + storage_class_size, span.size())});
    }
    catch (...) {
        free_system(ptr);
        throw;
    }
}

U8String LDDocumentStorage::to_sdn_string() const
{
    return view().to_string();
}

template <>
Datum<LDDocument, EmptyType> Datum<LDDocument, EmptyType>::from_sdn(const LDDocument& value)
{
    return Datum<LDDocument, EmptyType>(value);
}


TypeSignature::TypeSignature(U8StringView name) {
    name_ = LDDocument::parse_type_decl(name).value().to_standard_string();
}

LDDocument TypeSignature::parse() const {
    return LDDocument::parse_type_decl(name_);
}

LDDocument TypeSignature::parse(U8StringView str) {
    return LDDocument::parse_type_decl(str);
}


}}
