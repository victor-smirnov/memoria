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


//#define BOOST_SPIRIT_X3_DEBUG 1

#ifndef MMA1_NO_REACTOR
#   include <memoria/v1/reactor/reactor.hpp>
#endif

#include <memoria/v1/core/linked/document/linked_document.hpp>
#include <memoria/v1/core/tools/type_name.hpp>
#include <memoria/v1/core/strings/format.hpp>


#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/std_tuple.hpp>

#include <boost/spirit/home/x3/char/unicode.hpp>

#include <boost/variant/recursive_variant.hpp>
#include <boost/foreach.hpp>

#include <boost/optional/optional_io.hpp>


#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace memoria {
namespace v1 {

namespace x3 = boost::spirit::x3;
namespace enc = x3::standard;
namespace bf = boost::fusion;

SDNStringEscaper& SDNStringEscaper::current() {
    static thread_local SDNStringEscaper escaper;
    return escaper;
}


class LDDocumentBuilder {

    ArenaBuffer<char> string_buffer_;
    LDDocumentView& doc_;

public:

    LDDocumentBuilder(LDDocumentView& doc):
        doc_(doc)
    {}

    void doc_set(U8StringView view)
    {
        doc_.set(view);
    }

    void set_int64(int64_t value)
    {
        doc_.set((int64_t)value);
    }

    void set_double(double value)
    {
        doc_.set(value);
    }

    void append_char(char value) {
        string_buffer_.append_value(value);
    }

    void clear_string_buffer() {
        string_buffer_.clear();
    }

    bool is_string_buffer_empty() const {
        return string_buffer_.size() == 0;
    }

    LDString new_string()
    {
        auto span = string_buffer_.span();
        return doc_.new_string(U8StringView{span.data(), span.length()});
    }

    LDIdentifier new_identifier()
    {
        auto span = string_buffer_.span();
        return doc_.new_identifier(U8StringView{span.data(), span.length()});
    }

    LDDValue new_integer(int64_t v) {
        return doc_.new_integer(v);
    }

    LDDValue new_double(double v) {
        return doc_.new_double(v);
    }

    LDDValue new_boolean(bool v) {
        return doc_.new_boolean(v);
    }

    void set_doc_value(LDDValue value)
    {
        doc_.set_value(value);
    }

    LDDArray new_array(Span<LDDValue> span) {
        return doc_.new_array(span);
    }

    LDDMap new_map() {
        return doc_.new_map();
    }

    LDTypeDeclaration new_type_declaration(LDIdentifier id) {
        return doc_.new_type_declaration(id);
    }

    void add_type_decl_param(LDTypeDeclaration& dst, LDTypeDeclaration param) {
        dst.add_param(param);
    }

    void add_type_decl_ctr_arg(LDTypeDeclaration& dst, LDDValue ctr_arg) {
        dst.add_ctr_arg(ctr_arg);
    }

    LDDTypedValue new_typed_value(LDTypeDeclaration type_decl, LDDValue constructor)
    {
        return doc_.new_typed_value(type_decl, constructor);
    }

    LDDTypedValue new_typed_value(LDIdentifier id, LDDValue constructor)
    {
        auto type_decl = doc_.get_named_type_declaration(id.view()).get();
        return doc_.new_typed_value(type_decl, constructor);
    }

    void add_type_directory_entry(LDIdentifier id, LDTypeDeclaration type_decl)
    {
        doc_.set_named_type_declaration(id, type_decl);
    }

    static LDDocumentBuilder* current(LDDocumentBuilder* bb = nullptr, bool force = false) {
        thread_local LDDocumentBuilder* builder = nullptr;

        if (MMA1_UNLIKELY(force)) {
            builder = bb;
            return nullptr;
        }

        if (MMA1_LIKELY(!bb)) {
            return builder;
        }

        builder = bb;
        return nullptr;
    }
};



namespace {
namespace parser {

    struct LDNullValue {};

    class LDCharBufferBase {
    protected:
        LDDocumentBuilder* builder_;
    public:

        using value_type = char;
        using size_type = size_t;
        using reference = value_type&;
        using iterator = char*;
        using const_iterator = const char*;

        LDCharBufferBase()
        {
            builder_ = LDDocumentBuilder::current();
        }

        iterator begin() {return iterator{};}
        iterator end() {return iterator{};}

        const_iterator begin() const {return const_iterator{};}
        const_iterator end() const {return const_iterator{};}

        bool empty() const {
            return builder_->is_string_buffer_empty();
        }

        void insert(iterator, value_type value) {
            builder_->append_char(value);
        }

        template <typename II>
        void insert(iterator, II begin, II end){}

        void operator=(value_type value) {
            builder_->append_char(value);
        }
    };

    struct LDStringValue: LDCharBufferBase {
        LDString finish() {
            return builder_->new_string();
        }

        using LDCharBufferBase::operator=;
    };

    struct LDIdentifierValue: LDCharBufferBase {
        LDIdentifier finish() {
            return builder_->new_identifier();
        }

        operator LDIdentifier() {
            return finish();
        }

        using LDCharBufferBase::operator=;
    };



    class LDDArrayValue {
        ArenaBuffer<LDDValue> value_buffer_;

    public:
        using value_type = LDDValue;
        using iterator = EmptyType;

        iterator end() {return iterator{};}

        void insert(iterator, value_type value) {
            value_buffer_.append_value(value);
        }

        LDDArray finish() {
            return LDDocumentBuilder::current()->new_array(value_buffer_.span());
        }
    };


    using MapEntryTuple = std::tuple<LDString, LDDValue>;

    class LDDMapValue {
        LDDMap value_;

    public:
        using value_type = MapEntryTuple;
        using iterator = EmptyType;

        LDDMapValue() {
            value_ = LDDocumentBuilder::current()->new_map();
        }

        iterator end() {return iterator{};}

        void insert(iterator, value_type&& entry) {
            value_.set(std::get<0>(entry), std::get<1>(entry));
        }

        LDDMap& finish() {
            return value_;
        }
    };

    using LDTypeDeclarationValueBase = bf::vector<
        LDIdentifier,
        Optional<std::vector<LDTypeDeclaration>>,
        Optional<std::vector<LDDValue>>
    >;

    struct LDTypeDeclarationValue: LDTypeDeclarationValueBase {

        void operator=(LDIdentifier ii)
        {
            bf::at_c<0>(*this) = ii;
        }

        LDTypeDeclaration finish() const
        {
            LDDocumentBuilder* builder = LDDocumentBuilder::current();

            LDTypeDeclaration decl = builder->new_type_declaration(bf::at_c<0>(*this));

            const auto& params = bf::at_c<1>(*this);

            if (params) {
                for (auto& td: params.get()) {
                    builder->add_type_decl_param(decl, td);
                }
            }

            const auto& args = bf::at_c<2>(*this);

            if (args) {
                for (auto& ctr_arg: args.get()) {
                    builder->add_type_decl_ctr_arg(decl, ctr_arg);
                }
            }

            return decl;
        }

        operator LDTypeDeclaration() const {
            return finish();
        }
    };

    struct TypeReference {
        LDIdentifier id{};

        void operator=(LDIdentifier id) {
            this->id = id;
        }
    };

    using TypeDeclOrReference = boost::variant<TypeReference, LDTypeDeclarationValue>;


    struct LDDValueVisitor: boost::static_visitor<> {

        LDDValue value;

        void operator()(long v){
            value = LDDocumentBuilder::current()->new_integer(v);
        }

        void operator()(double v){
            value = LDDocumentBuilder::current()->new_double(v);
        }

        void operator()(bool v){
            value = LDDocumentBuilder::current()->new_boolean(v);
        }

        void operator()(LDNullValue& v) {}
        void operator()(LDIdentifier& v) {}

        void operator()(LDDMap& v) {}

        void operator()(LDString& v) {
            value = v;
        }

        template <typename V>
        void operator()(V& v){
            value = v.finish();
        }
    };

    using LDDTypedValueValueBase = bf::vector<
        TypeDeclOrReference,
        LDDValue
    >;

    struct LDDTypedValueValue: LDDTypedValueValueBase {

        struct Visitor: public boost::static_visitor<> {

            LDDValue ctr_value_;
            LDDTypedValue typed_value;

            Visitor(LDDValue ctr_value): ctr_value_(ctr_value) {}

            void operator()(TypeReference ref){
                typed_value = LDDocumentBuilder::current()->new_typed_value(
                    ref.id,
                    ctr_value_
                );
            }

            void operator()(LDTypeDeclaration type_decl){
                typed_value = LDDocumentBuilder::current()->new_typed_value(
                    type_decl,
                    ctr_value_
                );
            }
        };

        LDDTypedValue finish()
        {
            Visitor vv(bf::at_c<1>(*this));
            boost::apply_visitor(vv, bf::at_c<0>(*this));
            return vv.typed_value;
        }
    };

    using StringOrTypedValueBase = bf::vector<LDString, Optional<TypeDeclOrReference>>;

    struct StringOrTypedValue: StringOrTypedValueBase {
        LDDValue finish()
        {
            const auto& type = bf::at_c<1>(*this);
            if (MMA1_LIKELY(!type)) {
                return bf::at_c<0>(*this);
            }

            LDDTypedValueValue typed_value;

            bf::at_c<0>(typed_value) = bf::at_c<1>(*this).get();
            bf::at_c<1>(typed_value) = bf::at_c<0>(*this);

            return typed_value.finish();
        }
    };

    using TypeDirectoryMapEntry = bf::vector<LDIdentifier, LDTypeDeclaration>;

    struct TypeDirectoryValue {
        using value_type = TypeDirectoryMapEntry;
        using iterator = EmptyType;
    private:
        LDDocumentBuilder* builder_;
    public:
        TypeDirectoryValue() {
            builder_ = LDDocumentBuilder::current();
        }

        iterator end() {return iterator{};}

        void insert(iterator, const TypeDirectoryMapEntry& entry)
        {
            builder_->add_type_directory_entry(
                bf::at_c<0>(entry),
                bf::at_c<1>(entry)
            );
        }
    };



    using x3::lexeme;
    using x3::double_;
    using x3::lit;
    using enc::char_;

    const auto dummy = [](auto){};

    const auto finish_string = [](auto& ctx) {
        x3::_val(ctx) = x3::_attr(ctx).finish();
    };

    const auto finish_identifier = [](auto& ctx) {
        x3::_val(ctx) = x3::_attr(ctx).finish();
    };


    const auto set_doc_value = [](auto& ctx){
        LDDocumentBuilder::current()->set_doc_value(bf::at_c<1>(x3::_attr(ctx)));
    };

    const auto finish_value = [](auto& ctx){
        LDDValueVisitor visitor;
        boost::apply_visitor(visitor, x3::_attr(ctx));
        x3::_val(ctx) = visitor.value;
    };

    const auto clear_string_buffer = [](auto& ctx){
        LDDocumentBuilder::current()->clear_string_buffer();
    };

    const auto set_bool_true = [](auto& ctx) {
        x3::_val(ctx) = true;
    };

    const auto set_bool_false = [](auto& ctx) {
        x3::_val(ctx) = false;
    };

    x3::real_parser<double, x3::strict_real_policies<double> > const strict_double_ = {};

    x3::rule<class doc> const sdn_document              = "sdn_document";
    x3::rule<class value, LDDValue> const sdn_value     = "sdn_value";
    x3::rule<class standalone_value, LDDValue> const standalone_sdn_value     = "standalone_sdn_value";

    x3::rule<class array, LDDArrayValue> const array    = "array";
    x3::rule<class map,   LDDMapValue> const map        = "map";

    x3::rule<class map_entry, MapEntryTuple> const map_entry = "map_entry";

    x3::rule<class null_value, LDNullValue> const null_value = "null_value";

    x3::rule<class string_value, LDStringValue> const quoted_string = "quoted_string";
    x3::rule<class sdn_string, LDString> const sdn_string           = "sdn_string";

    x3::rule<class sdn_string_or_typed_value, StringOrTypedValue>
            const sdn_string_or_typed_value = "sdn_string_or_typed_value";

    x3::rule<class identifier_value, LDIdentifierValue> const identifier    = "identifier";
    x3::rule<class sdn_identifier, LDIdentifier> const sdn_identifier       = "sdn_identifier";

    x3::rule<class type_declaration, LDTypeDeclarationValue> const type_declaration     = "type_decalration";
    x3::rule<class standalone_type_decl, LDTypeDeclaration> const standalone_type_decl  = "standalone_type_decl";

    x3::rule<class typed_value, LDDTypedValueValue> const typed_value = "typed_value";

    x3::rule<class type_directory, TypeDirectoryValue> const type_directory = "type_directory";

    x3::rule<class type_directory_entry, TypeDirectoryMapEntry>
            const type_directory_entry = "type_directory_entry";


    x3::rule<class type_reference, TypeReference> const type_reference = "type_reference";
    x3::rule<class type_decl_or_reference, TypeDeclOrReference>
            const type_decl_or_reference = "type_decl_or_reference";

    x3::rule<class bool_true, bool> bool_value_true     = "bool_value_true";
    x3::rule<class bool_false, bool> bool_value_false   = "bool_value_false";
    x3::rule<class bool_value, bool> bool_value         = "bool_value";

    const auto quoted_string_def    = lexeme['\'' >> *(char_ - '\'' - "\\\'" | '\\' >> char_('\'')) >> '\'']
                                        | lexeme['"' >> *(char_ - '"' - "\\\"" | '\\' >> char_('"')) >> '"'] ;


    const auto sdn_string_def       = x3::eps[clear_string_buffer] >> quoted_string [finish_string];

    const auto raw_identifier       = (lexeme[(enc::alpha | char_('_')) >> *(enc::alnum | char_('_'))]
                                       - "null" - "true" - "false");

    const auto identifier_def       = raw_identifier;
    const auto sdn_identifier_def   = x3::eps[clear_string_buffer] >> identifier [finish_identifier];


    const auto type_declaration_def = sdn_identifier
                                        >> -('<' >> -(type_declaration % ',') >> '>')
                                        >> -('(' >> -(sdn_value % ',') >> ')');

    const auto type_reference_def   = '#' >> sdn_identifier;

    const auto type_decl_or_reference_def = type_declaration | type_reference;

    const auto typed_value_def      = '@' >> type_decl_or_reference >> "=" >> sdn_value;

    const auto sdn_string_or_typed_value_def = sdn_string >> -('@' >> type_decl_or_reference);


    const auto null_value_def   = lexeme[lit("null")][dummy];

    const auto array_def        = '[' >> (sdn_value % ',') >> ']' |
                                        lit('[') >> ']';

    const auto map_entry_def    = (sdn_string  >> ':' >> sdn_value);

    const auto map_def          = '{' >> (map_entry % ',') >> '}' |
                                        lit('{') >> '}';

    const auto type_directory_entry_def = sdn_identifier >> ':' >> type_declaration;

    const auto type_directory_def = "#{" >> (type_directory_entry % ',') >> '}' | lit("#{") >> "}";

    const auto bool_value_true_def      = lit("true") [set_bool_true];
    const auto bool_value_false_def     = lit("false") [set_bool_false];
    const auto bool_value_def           = bool_value_true | bool_value_false;

    const auto sdn_value_def    = (sdn_string_or_typed_value |
                                    strict_double_ |
                                    x3::int64 |
                                    map |
                                    null_value |
                                    array |
                                    type_declaration |
                                    typed_value |
                                    bool_value
                                  )[finish_value];

    const auto sdn_document_def = (-type_directory >> sdn_value) [set_doc_value];

    const auto standalone_type_decl_def = type_declaration;
    const auto standalone_sdn_value_def = sdn_value;


    BOOST_SPIRIT_DEFINE(
            sdn_document, sdn_value, array, map, map_entry,
            identifier, sdn_identifier, quoted_string,
            sdn_string, null_value, type_declaration,
            typed_value, type_directory, type_directory_entry,
            type_reference, type_decl_or_reference,
            sdn_string_or_typed_value, standalone_type_decl,
            standalone_sdn_value, bool_value_true, bool_value_false,
            bool_value
    );
}

template <typename Iterator>
bool parse_sdn2(Iterator& first, Iterator& last, LDDocument& doc)
{
    LDDocumentBuilder builder(doc);
    LDDocumentBuilder::current(&builder);

    bool r = x3::phrase_parse(first, last, parser::sdn_document, enc::space);

    LDDocumentBuilder::current(nullptr, true);

    if (first != last)
        return false;

    return r;
}


template <typename Iterator>
bool parse_sdn_type_decl(Iterator& first, Iterator& last, LDDocument& doc)
{
    LDDocumentBuilder builder(doc);
    LDDocumentBuilder::current(&builder);

    LDTypeDeclaration type_decl{};

    bool r = x3::phrase_parse(first, last, parser::standalone_type_decl, enc::space, type_decl);

    builder.set_doc_value(type_decl);

    LDDocumentBuilder::current(nullptr, true);

    if (first != last)
        return false;

    return r;
}


template <typename Iterator>
bool parse_raw_sdn_type_decl(Iterator& first, Iterator& last, LDDocumentView& doc, LDTypeDeclaration& type_decl)
{
    LDDocumentBuilder builder(doc);
    LDDocumentBuilder::current(&builder);

    bool r = x3::phrase_parse(first, last, parser::standalone_type_decl, enc::space, type_decl);

    LDDocumentBuilder::current(nullptr, true);

    if (first != last)
        return false;

    return r;
}

template <typename Iterator>
bool parse_raw_value0(Iterator& first, Iterator& last, LDDocumentView& doc, LDDValue& value)
{
    LDDocumentBuilder builder(doc);
    LDDocumentBuilder::current(&builder);

    bool r = x3::phrase_parse(first, last, parser::standalone_sdn_value, enc::space, value);

    LDDocumentBuilder::current(nullptr, true);

    if (first != last)
        return false;

    return r;
}



template <typename Iterator>
bool parse_identifier(Iterator& first, Iterator& last)
{
    bool r = x3::phrase_parse(first, last, parser::raw_identifier, enc::space);

    if (first != last)
        return false;

    return r;
}



template <typename II>
void assert_parse_ok(bool res, const char* msg, II start0, II start, II end)
{
    if (!res)
    {
        std::stringstream buf;

        ptrdiff_t pos = start - start0;

        bool add_ellipsis = true;
        for (size_t c = 0; c < 125; c++) {
            buf << *start;
            if (++start == end) {
                add_ellipsis = false;
                break;
            }
        }

        if (add_ellipsis) {
            buf << "...";
        }

        MMA1_THROW(SDNParseException()) << fmt::format_ex(u"{} at {}: {}", msg, pos, buf.str());
    }
}


}


LDDocument LDDocument::parse(CharIterator start, CharIterator end, const SDNParserConfiguration& cfg)
{
    LDDocument doc;

    auto tmp = start;

    bool result = parse_sdn2(start, end, doc);

    assert_parse_ok(result, "Can't parse type document", tmp, start, end);

    return std::move(doc);
}

LDDocument LDDocument::parse_type_decl(CharIterator start, CharIterator end, const SDNParserConfiguration& cfg)
{
    LDDocument doc;

    auto tmp = start;

    bool result = parse_sdn_type_decl(start, end, doc);

    assert_parse_ok(result, "Can't parse type declaration", tmp, start, end);

    return doc;
}

LDTypeDeclaration LDDocumentView::parse_raw_type_decl(CharIterator start, CharIterator end, const SDNParserConfiguration& cfg)
{
    LDTypeDeclaration type_decl{};

    auto tmp = start;

    bool result = parse_raw_sdn_type_decl(start, end, *this, type_decl);

    assert_parse_ok(result, "Can't parse type declaration", tmp, start, end);

    return type_decl;
}

LDDValue LDDocumentView::parse_raw_value(CharIterator start, CharIterator end, const SDNParserConfiguration& cfg)
{
    LDDValue value{};

    auto tmp = start;

    bool result = parse_raw_value0(start, end, *this, value);

    assert_parse_ok(result, "Can't parse SDN value", tmp, start, end);

    return value;
}

bool LDDocumentView::is_identifier(CharIterator start, CharIterator end)
{
    return parse_identifier(start, end);
}


}}
