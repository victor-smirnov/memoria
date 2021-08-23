
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

#include <memoria/core/linked/document/ld_common.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/types/typehash.hpp>

#include <memoria/core/datatypes/traits.hpp>

#include <iostream>
#include <functional>

namespace memoria {

struct SDNParseException: RuntimeException {};

class SDNParserConfiguration {
public:
    SDNParserConfiguration() {}
};

class LDDocument;


namespace ldd_ {

    template <typename T>
    struct ObjectCreatorHelper {
        template <typename Doc, typename... Args>
        static auto process(Doc* doc, Args&&... args) {
            return doc->template new_raw_value<T>(std::forward<Args>(args)...);
        }
    };

    template <>
    struct ObjectCreatorHelper<LDStringView> {
        template <typename Doc, typename... Args>
        static auto process(Doc* doc, Args&&... args) {
            return doc->new_varchar(std::forward<Args>(args)...);
        }
    };


}

class LDDocumentView {

    using AtomType      = typename ld_::LDArenaView::AtomType;
protected:
    using ValueMap      = ld_::ValueMap;
    using StringSet     = ld_::StringSet;
    using Array         = ld_::Array;
    using TypeDeclsMap  = ld_::TypeDeclsMap;
    using DocumentState = ld_::DocumentState;
    using DocumentPtr   = ld_::LDPtr<DocumentState>;

    ld_::LDArenaView arena_;
protected:
    friend class LDDocumentBuilder;
    friend class LDDMapView;
    friend class LDDArrayView;
    friend class LDTypeName;
    friend class LDDataTypeParam;
    friend class LDDataTypeCtrArg;
    friend class LDTypeDeclarationView;
    friend class LDDValueView;
    friend class LDStringView;
    friend class LDIdentifierView;
    friend class LDDTypedValueView;

    template <typename>
    friend struct DataTypeOperationsImpl;

    template <typename T>
    friend struct NumericDataTypeOperationsImpl;

    template <typename>
    friend struct ldd_::ObjectCreatorHelper;

    template <typename, typename>
    friend struct MakeLDView;

    using CharIterator = typename U8StringView::const_iterator;

    template <typename>
    friend struct DataTypeTraits;

public:
    LDDocumentView(): arena_() {}
    LDDocumentView(ld_::LDArenaView arena) noexcept: arena_(arena) {}
    LDDocumentView(Span<const AtomType> span) noexcept:
        arena_(span)
    {}

    Span<const AtomType> span() const noexcept {
        return arena_.span();
    }

    Span<AtomType> span() noexcept {
        return arena_.span();
    }

    LDDocumentView(const LDDocumentView&) noexcept = default;
    LDDocumentView(LDDocumentView&&) noexcept = default;

    LDDocumentView& operator=(const LDDocumentView&) noexcept = default;
    LDDocumentView& operator=(LDDocumentView&&) noexcept = default;

    LDDocumentView as_immutable_view() const noexcept
    {
        LDDocumentView view = *this;
        view.arena_.clear_arena_ptr();
        view.arena_.set_size(this->arena_.data_size());

        if (*this != view) {
            terminate(format_u8("Invalid LDDocumentview!").data());
        }

        return view;
    }

    LDDocument clone();

    bool equals(const LDDocumentView* other) const noexcept {
        return arena_.data() == other->arena_.data();
    }

    LDDValueView value() const noexcept;

    void set_varchar(U8StringView string);
    void set_bigint(int64_t value);
    void set_double(double value);
    void set_boolean(bool value);
    void set_null();

    void set_document(const LDDocumentView& other);

    LDDMapView set_map();
    LDDArrayView set_array();

    template <typename T, typename... Args>
    DTTLDViewType<T> set_value(Args&&... args)
    {
        auto value_ptr = LDStorageAllocator<T>::allocate_and_construct(arena_.make_mutable(), std::forward<Args>(args)...);
        set_tag(value_ptr.get(), ld_tag_value<T>());
        state_mutable()->value = value_ptr;

        using LDViewType = DTTLDViewType<T>;
        return LDViewType{this, value_ptr, ld_tag_value<T>()};
    }

    LDDValueView set_sdn(U8StringView sdn);

    LDDocumentView* make_mutable() const
    {
        if (arena_.is_mutable()) {
            return const_cast<LDDocumentView*>(this);
        }
        MMA_THROW(RuntimeException()) << WhatCInfo("Document is not mutable");
    }



    std::ostream& dump(std::ostream& out) const
    {
        LDDumpFormatState state;
        LDDumpState dump_state(*this);
        dump(out, state, dump_state);
        return out;
    }

    std::ostream& dump(std::ostream& out, LDDumpFormatState& format) const
    {
        LDDumpState dump_state(*this);
        dump(out, format, dump_state);
        return out;
    }

    std::ostream& dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const;

    U8String to_string() const
    {
        LDDumpFormatState fmt = LDDumpFormatState().simple();
        std::stringstream ss;
        dump(ss, fmt);
        return ss.str();
    }

    U8String to_pretty_string() const
    {
        LDDumpFormatState fmt = LDDumpFormatState();
        std::stringstream ss;
        dump(ss, fmt);
        return ss.str();
    }

    LDTypeDeclarationView create_named_type(U8StringView name, U8StringView type_decl);

    Optional<LDTypeDeclarationView> get_named_type_declaration(U8StringView name) const;

    void remove_named_type_declaration(U8StringView name);

    void for_each_named_type(std::function<void (U8StringView name, LDTypeDeclarationView)> fn) const;

    std::vector<std::pair<U8StringView, LDTypeDeclarationView>> named_types() const;

    static bool is_identifier(U8StringView string) {
        return is_identifier(string.begin(), string.end());
    }

    static bool is_identifier(CharIterator start, CharIterator end);

    static void assert_identifier(U8StringView name);

    bool operator==(const LDDocumentView& other) const noexcept {
        return arena_.span() == other.arena_.span();
    }

    bool operator!=(const LDDocumentView& other) const noexcept {
        return arena_.span() != other.arena_.span();
    }

    void add_shared_string(U8StringView string);

protected:
    Optional<ld_::LDPtr<DTTLDStorageType<LDString>>> is_shared(DTTViewType<LDString> string) const;

    DocumentPtr doc_ptr() const {
        return DocumentPtr{sizeof(LDDocumentHeader)};
    }

    LDTypeDeclarationView parse_raw_type_decl(
            CharIterator start,
            CharIterator end,
            const SDNParserConfiguration& cfg = SDNParserConfiguration{}
    );

    LDDValueView parse_raw_value(
            CharIterator start,
            CharIterator end,
            const SDNParserConfiguration& cfg = SDNParserConfiguration{}
    );

    void do_dump_dictionary(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const;

    bool has_type_dictionary() const {
        return state()->type_directory.get() != 0;
    }

    void set_named_type_declaration(LDIdentifierView name, LDTypeDeclarationView type_decl);
    void set_named_type_declaration(U8StringView name, LDTypeDeclarationView type_decl);

    LDTypeDeclarationView new_type_declaration(U8StringView name);
    LDTypeDeclarationView new_type_declaration(LDIdentifierView name);
    LDDValueView new_typed_value(LDTypeDeclarationView typedecl, LDDValueView ctr_value);

    LDTypeDeclarationView new_detached_type_declaration(LDIdentifierView name);

    TypeDeclsMap ensure_type_decls_exist()
    {
        auto* state = this->state_mutable();
        if (!state->type_directory)
        {
            TypeDeclsMap vv = TypeDeclsMap::create(arena_.make_mutable());
            this->state_mutable()->type_directory = vv.ptr();
            return vv;
        }

        return TypeDeclsMap::get(&arena_, state->type_directory);
    }

    LDStringView new_varchar(DTTViewType<Varchar> view);
    LDIdentifierView new_identifier(DTTViewType<Varchar> view);

    LDDValueView new_bigint(DTTViewType<BigInt> value);
    LDDValueView new_boolean(DTTViewType<Boolean> value);
    LDDValueView new_double(DTTViewType<Double> value);
    LDDArrayView new_array(Span<LDDValueView> span);
    LDDArrayView new_array();
    LDDMapView new_map();

    void set_doc_value(LDDValueView value) noexcept;

    ld_::LDPtr<DTTLDStorageType<LDString>> intern(DTTViewType<LDString> view);

    DocumentState* state_mutable() {
        return doc_ptr().get_mutable(&arena_);
    }

    const DocumentState* state() const noexcept {
        return doc_ptr().get(&arena_);
    }

    void set_tag(ld_::LDDPtrHolder ptr, LDDValueTag tag) noexcept
    {
        ld_::ldd_set_tag(&arena_, ptr, tag);
    }

    template <typename T, typename... Args>
    LDPtrHolder new_value(Args&&... args)
    {
        return ldd_::ObjectCreatorHelper<T>::process(this, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    LDPtrHolder new_raw_value(Args&&... args)
    {
        auto value_ptr = LDStorageAllocator<T>::allocate_and_construct(arena_.make_mutable(), std::forward<Args>(args)...);

        set_tag(value_ptr.get(), ld_tag_value<T>());
        return value_ptr.get();
    }
};


class LDDocument: public LDDocumentView {
    ld_::LDArena ld_arena_;

    using LDDocumentView::arena_;

    friend class LDDArrayView;
    friend class LDDMapView;
    friend class LDTypeDeclarationView;
    friend class LDDTypedValueView;

    static constexpr size_t INITIAL_ARENA_SIZE = sizeof(LDDocumentHeader) + sizeof(DocumentState) + 16;

public:
    LDDocument():
        LDDocumentView(),
        ld_arena_(INITIAL_ARENA_SIZE, &arena_)
    {
        allocate_state();
    }

    LDDocument(const LDDocument& other):
        LDDocumentView(),
        ld_arena_(&arena_, other.ld_arena_)
    {}

    LDDocument(LDDocument&& other):
        LDDocumentView(),
        ld_arena_(&arena_, std::move(other.ld_arena_))
    {}

    LDDocument(LDDocumentView view);
    LDDocument(Span<const uint8_t> data);

    LDDocument(U8StringView sdn);
    LDDocument(const char* sdn);

    LDDocument& operator=(const LDDocument&) = delete;
    LDDocument& operator=(LDDocument&&);

    LDDocument compactify() const ;

    void clear();
    void reset();

    static LDDocument parse(U8StringView view) {
        return parse(view.begin(), view.end());
    }

    static LDDocument parse(
            CharIterator start,
            CharIterator end,
            const SDNParserConfiguration& cfg = SDNParserConfiguration{}
    );

    static LDDocument parse_type_decl(
            U8StringView view,
            const SDNParserConfiguration& cfg = SDNParserConfiguration{}
    ) {
        return parse_type_decl(view.begin(), view.end());
    }

    static LDDocument parse_type_decl(
            CharIterator start,
            CharIterator end,
            const SDNParserConfiguration& cfg = SDNParserConfiguration{}
    );




    static LDDocument parse_type_decl_qi(
            CharIterator start,
            CharIterator end,
            const SDNParserConfiguration& cfg = SDNParserConfiguration{}
    );

    static LDDocument parse_type_decl_qi(
            U8StringView view,
            const SDNParserConfiguration& cfg = SDNParserConfiguration{}
    ) {
        return parse_type_decl_qi(view.begin(), view.end());
    }

protected:
    void allocate_state() {
        allocate<DocumentState>(ld_arena_.view(), DocumentState{0, 0, 0});
    }
};



std::ostream& operator<<(std::ostream& out, const LDDocumentView& doc);

}
