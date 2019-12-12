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


//#include <boost/config/warning_disable.hpp>
//#include <boost/spirit/home/x3.hpp>
//#include <boost/spirit/include/phoenix_core.hpp>
//#include <boost/spirit/include/phoenix_operator.hpp>
//#include <boost/spirit/include/phoenix_fusion.hpp>
//#include <boost/spirit/include/phoenix_stl.hpp>
//#include <boost/spirit/include/phoenix_object.hpp>
//#include <boost/fusion/include/std_tuple.hpp>

//#include <boost/spirit/home/x3/char/unicode.hpp>

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>

#include <boost/variant/recursive_variant.hpp>
#include <boost/foreach.hpp>

#include <boost/optional/optional_io.hpp>


#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace memoria {
namespace v1 {

namespace qi = boost::spirit::qi;
namespace enc = qi::standard;
namespace bf = boost::fusion;
namespace bp = boost::phoenix;

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

    void append_char(char value) {
        string_buffer_.append_value(value);
    }

    void clear_string_buffer() {
        string_buffer_.clear();
    }

    bool is_string_buffer_empty() const {
        return string_buffer_.size() == 0;
    }

    LDStringView new_varchar()
    {
        auto span = string_buffer_.span();
        return LDStringView{&doc_, doc_.new_value<Varchar>(U8StringView{span.data(), span.length()})};
    }

    LDIdentifierView new_identifier()
    {
        auto span = string_buffer_.span();
        return doc_.new_identifier(U8StringView{span.data(), span.length()});
    }

    LDDValueView new_bigint(int64_t v) {
        return doc_.new_bigint(v);
    }

    LDDValueView new_double(double v) {
        return doc_.new_double(v);
    }

    LDDValueView new_boolean(bool v) {
        return doc_.new_boolean(v);
    }

    void set_doc_value(LDDValueView value)
    {
        doc_.set_doc_value(value);
    }

    LDDArrayView new_array(Span<LDDValueView> span) {
        return doc_.new_array(span);
    }

    LDDMapView new_map() {
        return doc_.new_map();
    }

    LDTypeDeclarationView new_type_declaration(LDIdentifierView id) {
        return doc_.new_type_declaration(id);
    }

    void add_type_decl_param(LDTypeDeclarationView& dst, LDTypeDeclarationView param) {
        dst.add_param(param);
    }

    void add_type_decl_ctr_arg(LDTypeDeclarationView& dst, LDDValueView ctr_arg) {
        dst.add_ctr_arg(ctr_arg);
    }

    LDDValueView new_typed_value(LDTypeDeclarationView type_decl, LDDValueView constructor)
    {
        return doc_.new_typed_value(type_decl, constructor);
    }

    LDDValueView new_typed_value(LDIdentifierView id, LDDValueView constructor)
    {
        auto type_decl = doc_.get_named_type_declaration(id.view()).get();
        return doc_.new_typed_value(type_decl, constructor);
    }

    void add_type_directory_entry(LDIdentifierView id, LDTypeDeclarationView type_decl)
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
    LDStringView finish() {
        return builder_->new_varchar();
    }

    using LDCharBufferBase::operator=;
};

struct LDIdentifierValue: LDCharBufferBase {
    LDIdentifierView finish() {
        return builder_->new_identifier();
    }

    operator LDIdentifierView() {
        return finish();
    }

    using LDCharBufferBase::operator=;
};



class LDDArrayValue {
    ArenaBuffer<LDDValueView> value_buffer_;

public:
    using value_type = LDDValueView;
    using iterator = EmptyType;

    iterator end() {return iterator{};}

    void insert(iterator, value_type value) {
        value_buffer_.append_value(value);
    }

    LDDArrayView finish() {
        return LDDocumentBuilder::current()->new_array(value_buffer_.span());
    }
};


using MapEntryTuple = bf::vector<LDStringView, LDDValueView>;

class LDDMapValue {
    LDDMapView value_;

public:
    using value_type = MapEntryTuple;
    using iterator = EmptyType;

    LDDMapValue() {
        value_ = LDDocumentBuilder::current()->new_map();
    }

    iterator end() const {return iterator{};}

    void insert(iterator, const value_type& entry) {
        value_.set_ld_value(bf::at_c<0>(entry), bf::at_c<1>(entry));
    }

    LDDMapView& finish() {
        return value_;
    }
};

using LDTypeDeclarationValueBase = bf::vector<
    LDIdentifierView,
    Optional<std::vector<LDTypeDeclarationView>>,
    Optional<std::vector<LDDValueView>>
>;

struct LDTypeDeclarationValue: LDTypeDeclarationValueBase {

    void operator=(LDIdentifierView ii)
    {
        bf::at_c<0>(*this) = ii;
    }

    LDTypeDeclarationView finish() const
    {
        LDDocumentBuilder* builder = LDDocumentBuilder::current();

        LDTypeDeclarationView decl = builder->new_type_declaration(bf::at_c<0>(*this));

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

    operator LDTypeDeclarationView() const {
        return finish();
    }
};


struct TypeReference {
    LDIdentifierView id{};

    void operator=(LDIdentifierView id) {
        this->id = id;
    }
};

using TypeDeclOrReference = boost::variant<LDIdentifierView, LDTypeDeclarationValue>;


struct LDDValueVisitor: boost::static_visitor<> {

    LDDValueView value;

    void operator()(long long v){
        value = LDDocumentBuilder::current()->new_bigint(v);
    }

    void operator()(double v){
        value = LDDocumentBuilder::current()->new_double(v);
    }

    void operator()(bool v){
        value = LDDocumentBuilder::current()->new_boolean(v);
    }

    void operator()(LDNullValue& v) {}
    void operator()(LDIdentifierView& v) {}

    void operator()(LDDMapView& v) {}

    void operator()(LDStringView& v) {
        value = v;
    }

    template <typename V>
    void operator()(V& v){
        value = v.finish();
    }
};

using LDDTypedValueValueBase = bf::vector<TypeDeclOrReference, LDDValueView>;

struct LDDTypedValueValue: LDDTypedValueValueBase {

    struct Visitor: public boost::static_visitor<> {

        LDDValueView ctr_value_;
        LDDValueView typed_value;

        Visitor(LDDValueView ctr_value): ctr_value_(ctr_value) {}

        void operator()(LDIdentifierView ref){
            typed_value = LDDocumentBuilder::current()->new_typed_value(
                        ref,
                        ctr_value_
            );
        }

        void operator()(LDTypeDeclarationView type_decl){
            typed_value = LDDocumentBuilder::current()->new_typed_value(
                        type_decl,
                        ctr_value_
            );
        }
    };

    LDDValueView finish()
    {
        Visitor vv(bf::at_c<1>(*this));
        boost::apply_visitor(vv, bf::at_c<0>(*this));
        return vv.typed_value;
    }
};

using StringOrTypedValueBase = bf::vector<LDStringView, Optional<TypeDeclOrReference>>;

struct StringOrTypedValue: StringOrTypedValueBase {
    LDDValueView finish()
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

using TypeDirectoryMapEntry = bf::vector<LDIdentifierView, LDTypeDeclarationView>;

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


class EmptyCharCollection {
public:
    using value_type = char;
    using iterator = EmptyType;

    EmptyCharCollection() {}

    EmptyCharCollection(value_type) {}

    iterator end() const {return iterator();}

    void insert(iterator, value_type vv)
    {}

    void operator=(value_type vv) {}
};



template <typename Iterator>
struct SDNParser : qi::grammar<Iterator, LDDocument(), qi::space_type>
{
    SDNParser() : SDNParser::base_type(sdn_document)
    {
        using qi::long_;
        using qi::lit;
        using qi::lexeme;
        using qi::standard::char_;
        using qi::_1;
        using qi::_val;

        using bp::construct;
        using bp::val;

        constexpr auto dummy = [](const auto& attrib, auto& ctx){};

        static constexpr auto finish_identifier = [](auto& attrib, auto& ctx) {
            bf::at_c<0>(ctx.attributes) = attrib.finish();
        };

        static constexpr auto finish_string = [](auto& attrib, auto& ctx) {
            bf::at_c<0>(ctx.attributes) = attrib.finish();
        };

        static constexpr auto set_bool_true = [](const auto& attrib, auto& ctx) {
            bf::at_c<0>(ctx.attributes) = true;
        };

        static constexpr auto set_bool_false = [](const auto& attrib, auto& ctx) {
            bf::at_c<0>(ctx.attributes) = false;
        };

        static constexpr auto clear_string_buffer = [](const auto& attrib, const auto& ctx){
            LDDocumentBuilder::current()->clear_string_buffer();
        };

        static constexpr auto finish_value = [](auto& attrib, auto& ctx){
            LDDValueVisitor visitor;
            boost::apply_visitor(visitor, attrib);
            bf::at_c<0>(ctx.attributes) = visitor.value;
        };

        static constexpr auto set_doc_value = [](auto& attrib, auto& ctx){
            LDDocumentBuilder::current()->set_doc_value(bf::at_c<1>(attrib));
        };


        quoted_string    = lexeme['\'' >> *(char_ - '\'' - "\\\'" | '\\' >> char_('\'')) >> '\'']
                           | lexeme['"' >> *(char_ - '"' - "\\\"" | '\\' >> char_('"')) >> '"'];

        sdn_string       = qi::eps[clear_string_buffer] >> quoted_string [finish_string];

        identifier       = (lexeme[(enc::alpha | char_('_')) >> *(enc::alnum | char_('_'))]
                            - "null" - "true" - "false");

        sdn_identifier   = qi::eps[clear_string_buffer] >> identifier [finish_identifier];


        type_declaration = sdn_identifier
                            >> -('<' >> -(type_declaration % ',') >> '>')
                            >> -('(' >> -(sdn_value % ',') >> ')');

        type_reference   = '#' >> sdn_identifier;

        type_decl_or_reference = type_declaration | type_reference;

        typed_value      = '@' >> type_decl_or_reference >> "=" >> sdn_value;

        sdn_string_or_typed_value = sdn_string >> -('@' >> type_decl_or_reference);


        null_value   = lexeme[lit("null")][dummy];

        array        = '[' >> (sdn_value % ',') >> ']' |
                                            lit('[') >> ']';

        map_entry    = (sdn_string  >> ':' >> sdn_value);

        map          = '{' >> (map_entry % ',') >> '}' |
                                            lit('{') >> '}';

        type_directory_entry = sdn_identifier >> ':' >> type_declaration;

        type_directory = "#{" >> (type_directory_entry % ',') >> '}' | lit("#{") >> "}";

        bool_value_true      = lit("true") [set_bool_true];
        bool_value_false     = lit("false") [set_bool_false];
        bool_value           = bool_value_true | bool_value_false;

        sdn_value    = (sdn_string_or_typed_value |
                                        strict_double |
                                        qi::long_long |
                                        map |
                                        null_value |
                                        array |
                                        type_declaration |
                                        typed_value |
                                        bool_value
                                      )[finish_value];

        sdn_document = (-type_directory >> sdn_value) [set_doc_value];

        standalone_type_decl = type_declaration;
        standalone_sdn_value = sdn_value;
    }

    qi::real_parser<double, qi::strict_real_policies<double>> strict_double;

    qi::rule<Iterator, LDDocument(), qi::space_type> sdn_document;
    qi::rule<Iterator, LDDValueView(), qi::space_type> sdn_value;
    qi::rule<Iterator, LDDValueView(), qi::space_type> standalone_sdn_value;

    qi::rule<Iterator, LDDArrayValue(), qi::space_type> array;
    qi::rule<Iterator, LDDMapValue(), qi::space_type> map;

    qi::rule<Iterator, MapEntryTuple(), qi::space_type> map_entry;

    qi::rule<Iterator, LDNullValue(), qi::space_type> null_value;

    qi::rule<Iterator, LDStringValue(), qi::space_type> quoted_string;
    qi::rule<Iterator, LDStringView(), qi::space_type> sdn_string;

    qi::rule<Iterator, StringOrTypedValue(), qi::space_type> sdn_string_or_typed_value;

    qi::rule<Iterator, LDIdentifierValue(), qi::space_type> identifier;
    qi::rule<Iterator, LDIdentifierView(), qi::space_type> sdn_identifier;

    qi::rule<Iterator, LDTypeDeclarationValue(), qi::space_type> type_declaration;
    qi::rule<Iterator, LDTypeDeclarationView(), qi::space_type> standalone_type_decl;

    qi::rule<Iterator, LDDTypedValueValue(), qi::space_type> typed_value;

    qi::rule<Iterator, TypeDirectoryValue(), qi::space_type> type_directory;

    qi::rule<Iterator, TypeDirectoryMapEntry(), qi::space_type> type_directory_entry;


    qi::rule<Iterator, LDIdentifierView(), qi::space_type> type_reference;
    qi::rule<Iterator, TypeDeclOrReference(), qi::space_type> type_decl_or_reference;

    qi::rule<Iterator, bool(), qi::space_type> bool_value_true;
    qi::rule<Iterator, bool(), qi::space_type> bool_value_false;
    qi::rule<Iterator, bool(), qi::space_type> bool_value;
};

template <typename Iterator>
struct TypeDeclarationParser : qi::grammar<Iterator, LDTypeDeclarationView(), qi::space_type>
{
    SDNParser<Iterator> sdn_parser;

    TypeDeclarationParser() : TypeDeclarationParser::base_type(sdn_parser.standalone_type_decl)
    {}
};

template <typename Iterator>
struct RawSDNValueParser : qi::grammar<Iterator, LDDValueView(), qi::space_type>
{
    SDNParser<Iterator> sdn_parser;

    RawSDNValueParser() : RawSDNValueParser::base_type(sdn_parser.standalone_sdn_value)
    {}
};

template <typename Iterator>
struct SDNIdentifierParser : qi::grammar<Iterator, EmptyCharCollection(), qi::space_type>
{
    SDNParser<Iterator> sdn_parser;

    qi::rule<Iterator, EmptyCharCollection(), qi::space_type> raw_identifier;

    SDNIdentifierParser() : SDNIdentifierParser::base_type(raw_identifier)
    {
        using qi::lexeme;
        using qi::standard::char_;

        raw_identifier = (lexeme[(enc::alpha | char_('_')) >> *(enc::alnum | char_('_'))]
                          - "null" - "true" - "false");
    }
};


namespace {
    struct LDDocumentBuilderCleanup {
        ~LDDocumentBuilderCleanup() noexcept
        {
            LDDocumentBuilder::current(nullptr, true);
        }
    };
}

template <typename Iterator>
bool parse_sdn2(Iterator& first, Iterator& last, LDDocument& doc)
{
    static thread_local SDNParser<Iterator> const grammar;

    LDDocumentBuilder builder(doc);
    LDDocumentBuilder::current(&builder);
    LDDocumentBuilderCleanup cleanup;

    bool r = qi::phrase_parse(first, last, grammar, qi::standard::space_type());

    if (first != last)
        return false;

    return r;
}



template <typename Iterator>
bool parse_sdn_type_decl(Iterator& first, Iterator& last, LDDocument& doc)
{
    static thread_local TypeDeclarationParser<Iterator> const grammar;

    LDDocumentBuilder builder(doc);
    LDDocumentBuilder::current(&builder);
    LDDocumentBuilderCleanup cleanup;

    LDTypeDeclarationView type_decl{};


    bool r = qi::phrase_parse(first, last, grammar, enc::space, type_decl);

    builder.set_doc_value(type_decl);

    if (first != last)
        return false;

    return r;
}


template <typename Iterator>
bool parse_raw_sdn_type_decl(Iterator& first, Iterator& last, LDDocumentView& doc, LDTypeDeclarationView& type_decl)
{
    static thread_local TypeDeclarationParser<Iterator> const grammar;

    LDDocumentBuilder builder(doc);
    LDDocumentBuilder::current(&builder);
    LDDocumentBuilderCleanup cleanup;

    bool r = qi::phrase_parse(first, last, grammar, enc::space, type_decl);

    if (first != last)
        return false;

    return r;
}

template <typename Iterator>
bool parse_raw_value0(Iterator& first, Iterator& last, LDDocumentView& doc, LDDValueView& value)
{
    static thread_local RawSDNValueParser<Iterator> const grammar;

    LDDocumentBuilder builder(doc);
    LDDocumentBuilder::current(&builder);
    LDDocumentBuilderCleanup cleanup;

    bool r = qi::phrase_parse(first, last, grammar, enc::space, value);

    if (first != last)
        return false;

    return r;
}



template <typename Iterator>
bool parse_identifier(Iterator& first, Iterator& last)
{
    static thread_local SDNIdentifierParser<Iterator> const grammar;
    bool r = qi::phrase_parse(first, last, grammar, enc::space);

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

LDTypeDeclarationView LDDocumentView::parse_raw_type_decl(CharIterator start, CharIterator end, const SDNParserConfiguration& cfg)
{
    LDTypeDeclarationView type_decl{};

    auto tmp = start;

    bool result = parse_raw_sdn_type_decl(start, end, *this, type_decl);

    assert_parse_ok(result, "Can't parse type declaration", tmp, start, end);

    return type_decl;
}

LDDValueView LDDocumentView::parse_raw_value(CharIterator start, CharIterator end, const SDNParserConfiguration& cfg)
{
    LDDValueView value{};

    auto tmp = start;

    bool result = parse_raw_value0(start, end, *this, value);

    assert_parse_ok(result, "Can't parse SDN value", tmp, start, end);

    return value;
}

bool LDDocumentView::is_identifier(CharIterator start, CharIterator end)
{
    return parse_identifier(start, end);
}

/*

LDDocument LDDocument::parse_type_decl_qi(
        CharIterator start,
        CharIterator end,
        const SDNParserConfiguration& cfg
) {

    static thread_local SDNParser<U8StringView::const_iterator> const grammar;

    //SDNDocument decl;

//    CharCollection cc;

    LDDocument doc;

    auto ii = start;
    bool result = qi::phrase_parse(ii, end, grammar, qi::standard::space_type(), doc);

    if ((!result) || ii != end)
    {
        //MMA1_THROW(RuntimeException()) << fmt::format_ex(u"Can't parse data type signature: \"{}\", unparsed: \"{}\"", (std::string)str, (std::string)U8StringView(ii));
    }
    else {
        //decl.resolve_maps();
        //return std::move(decl);
    }

    return doc;
}

*/

}}
