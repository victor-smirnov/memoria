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


//#define BOOST_SPIRIT_QI_DEBUG

#ifndef MMA_NO_REACTOR
#   include <memoria/reactor/reactor.hpp>
#endif

#include <memoria/core/hermes/hermes.hpp>
#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/strings/format.hpp>

#include <boost/config/warning_disable.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>

#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_auxiliary.hpp>
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/qi_action.hpp>

#include <boost/variant/recursive_variant.hpp>

#include <boost/optional/optional_io.hpp>


#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace memoria {
namespace hermes {

namespace qi = boost::spirit::qi;
namespace enc = qi::standard;
namespace bf = boost::fusion;
namespace bp = boost::phoenix;

class DocumentBuilder {

    ArenaBuffer<char> string_buffer_;
    HermesDocView& doc_;

public:

    DocumentBuilder(HermesDocView& doc):
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

    auto new_varchar()
    {
        auto span = string_buffer_.span();
        return doc_.new_tv<Varchar>(U8StringView{span.data(), span.length()});
    }

    auto new_identifier()
    {
        auto span = string_buffer_.span();
        return doc_.new_tv<Varchar>(U8StringView{span.data(), span.length()});
    }

    auto new_bigint(int64_t v) {
        return doc_.new_tv<BigInt>(v);
    }

    auto new_double(double v) {
        return doc_.new_tv<Double>(v);
    }

    auto new_boolean(bool v) {
        return doc_.new_tv<BigInt>((int64_t)v);
    }

    void set_doc_value(ViewPtr<Value> value) {
        doc_.set_value(value);
    }

    auto new_array(Span<ValuePtr> span) {
        return doc_.new_array(span);
    }

    auto new_map() {
        return doc_.new_map();
    }

    auto new_datatype(StringValuePtr id) {
        return doc_.new_datatype(id);
    }

    void add_type_decl_param(DatatypePtr& dst, ValuePtr param) {
        dst->append_type_parameter(param);
    }

    void add_type_decl_ctr_arg(DatatypePtr& dst, ValuePtr ctr_arg) {
        dst->append_constructor_argument(ctr_arg);
    }

    TypedValuePtr new_typed_value(DatatypePtr type_decl, ValuePtr constructor)
    {
        return doc_.new_typed_value(type_decl, constructor);
    }

//    ValuePtr new_typed_value(LDIdentifierView id, LDDValueView constructor)
//    {
//        auto type_decl = doc_.get_named_type_declaration(*id.view());
//        return doc_.new_typed_value(*type_decl, constructor)->as_value();
//    }

//    void add_type_directory_entry(LDIdentifierView id, LDTypeDeclarationView type_decl)
//    {
//        doc_.set_named_type_declaration(id, type_decl);
//    }

    void append_entry(GenericMapPtr& map, const StringValuePtr& name, const ValuePtr& value) {
        map->put(name, value);
    }

    static DocumentBuilder* current(DocumentBuilder* bb = nullptr, bool force = false) {
        thread_local DocumentBuilder* builder = nullptr;

        if (MMA_UNLIKELY(force)) {
            builder = bb;
            return nullptr;
        }

        if (MMA_LIKELY(!bb)) {
            return builder;
        }

        builder = bb;
        return nullptr;
    }
};

struct NullValue {};

class CharBufferBase {
protected:
    DocumentBuilder* builder_;
public:

    using value_type = char;
    using size_type = size_t;
    using reference = value_type&;
    using iterator = char*;
    using const_iterator = const char*;

    CharBufferBase() {
        builder_ = DocumentBuilder::current();
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

    void operator=(StringValuePtr str) {
        //FIXME this method should be removed
        // but some 'identifier' rules depend on it.
        // Those rules need to be refactored.
    }
};



struct ParsedStringValue: CharBufferBase {

    mutable StringValuePtr value;

    auto finish() const {
        value = builder_->new_varchar();
        return value;
    }

    using CharBufferBase::operator=;

    operator StringValuePtr() const {
        if (value->is_null()) {
            value = builder_->new_varchar();
        }
        return value;
    }
};


class ArrayValue {
    std::vector<ViewPtr<Value>> value_buffer_;

public:
    using value_type = ViewPtr<Value>;
    using iterator = EmptyType;

    iterator end() {return iterator{};}

    void insert(iterator, value_type value) {
        value_buffer_.push_back(value);
    }

    auto finish() {
        return DocumentBuilder::current()->new_array(Span<ValuePtr>(value_buffer_.data(), value_buffer_.size()));
    }
};


using MapEntryTuple = bf::vector<StringValuePtr, ValuePtr>;

class MapValue {
    GenericMapPtr value_;
    DocumentBuilder* builder_;
public:
    using value_type = MapEntryTuple;
    using iterator = EmptyType;

    MapValue() {
        builder_ = DocumentBuilder::current();
        value_ = builder_->new_map();
    }

    iterator end() const {return iterator{};}

    void insert(iterator, const value_type& entry) {
        builder_->append_entry(value_, bf::at_c<0>(entry), bf::at_c<1>(entry));
    }

    auto& finish() {
        return value_;
    }
};

using TypeDeclarationValueBase = bf::vector<
    StringValuePtr,
    Optional<std::vector<DatatypePtr>>,
    Optional<std::vector<ValuePtr>>
>;


struct TypeDeclarationValue: TypeDeclarationValueBase {

    void operator=(StringValuePtr ii) {
        bf::at_c<0>(*this) = ii;
    }

    DatatypePtr finish() const
    {
        DocumentBuilder* builder = DocumentBuilder::current();
        DatatypePtr decl = builder->new_datatype(bf::at_c<0>(*this));

        auto& params = bf::at_c<1>(*this);
        if (params) {
            for (auto& td: params.get()) {
                builder->add_type_decl_param(decl, td->as_value());
            }
        }

        auto& args = bf::at_c<2>(*this);
        if (args) {
            for (auto& ctr_arg: args.get()) {
                builder->add_type_decl_ctr_arg(decl, ctr_arg);
            }
        }

        return decl;
    }

    operator DatatypePtr() const {
        return finish();
    }
};



struct TypeReference {
    StringValuePtr id{};

    void operator=(StringValuePtr id) {
        this->id = id;
    }
};

using TypeDeclOrReference = boost::variant<StringValuePtr, TypeDeclarationValue>;


struct ValueVisitor: boost::static_visitor<> {

    ValuePtr value;

    void operator()(long long v){
        value = DocumentBuilder::current()->new_bigint(v)->as_value();
    }

    void operator()(double v){
        value = DocumentBuilder::current()->new_double(v)->as_value();
    }

    void operator()(bool v){
        value = DocumentBuilder::current()->new_boolean(v)->as_value();
    }

    void operator()(NullValue& v) {}

    void operator()(GenericMapPtr& v) {}

    void operator()(StringValuePtr& v) {
        value = v->as_value();
    }

    template <typename V>
    void operator()(V& v){
        value = v.finish()->as_value();
    }
};

using TypedValueValueBase = bf::vector<TypeDeclOrReference, ValuePtr>;


struct TypedValueValue: TypedValueValueBase {

    struct Visitor: public boost::static_visitor<> {

        ValuePtr ctr_value_;
        ValuePtr typed_value;

        Visitor(ValuePtr ctr_value): ctr_value_(ctr_value) {}

        void operator()(StringValuePtr ref){
//            typed_value = DocumentBuilder::current()->new_typed_value(
//                        ref->as_value(),
//                        ctr_value_
//            )->as_value();

            MEMORIA_MAKE_GENERIC_ERROR("Not implemented 1").do_throw();
        }

        void operator()(DatatypePtr type_decl){
            typed_value = DocumentBuilder::current()->new_typed_value(
                        type_decl,
                        ctr_value_
            )->as_value();
        }
    };

    ValuePtr finish()
    {
        Visitor vv(bf::at_c<1>(*this));
        boost::apply_visitor(vv, bf::at_c<0>(*this));
        return vv.typed_value;
    }
};


using StringOrTypedValueBase = bf::vector<StringValuePtr, Optional<TypeDeclOrReference>>;



struct StringOrTypedValue: StringOrTypedValueBase {
    ValuePtr finish()
    {
        const auto& type = bf::at_c<1>(*this);
        if (MMA_LIKELY(!type)) {
            return bf::at_c<0>(*this)->as_value();
        }

        TypedValueValue typed_value;

        bf::at_c<0>(typed_value) = bf::at_c<1>(*this).get();
        bf::at_c<1>(typed_value) = bf::at_c<0>(*this)->as_value();

        return typed_value.finish();
    }
};

using TypeDirectoryMapEntry = bf::vector<StringValuePtr, DatatypePtr>;


struct TypeDirectoryValue {
    using value_type = TypeDirectoryMapEntry;
    using iterator = EmptyType;
private:
    DocumentBuilder* builder_;
public:
    TypeDirectoryValue() {
        builder_ = DocumentBuilder::current();
    }

    iterator end() {return iterator{};}

    void insert(iterator, const TypeDirectoryMapEntry& entry)
    {
//        builder_->add_type_directory_entry(
//                    bf::at_c<0>(entry),
//                    bf::at_c<1>(entry)
//        );

        MEMORIA_MAKE_GENERIC_ERROR("Not implemented 2").do_throw();
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
struct HermesDocParser : qi::grammar<Iterator, pool::SharedPtr<HermesDoc>(), qi::space_type>
{
    HermesDocParser() : HermesDocParser::base_type(sdn_document)
    {
        using qi::long_;
        using qi::lit;
        using qi::lexeme;
        using qi::standard::char_;
        using qi::_1;
        using qi::_val;

        using bp::construct;
        using bp::val;


        auto dummy = [](const auto& attrib, auto& ctx){};

        static auto finish_identifier = [](auto& attrib, auto& ctx) {
            bf::at_c<0>(ctx.attributes) = attrib.finish();
        };

        static auto finish_string = [](auto& attrib, auto& ctx) {
            bf::at_c<0>(ctx.attributes) = attrib.finish();
        };

        static auto set_bool_true = [](const auto& attrib, auto& ctx) {
            bf::at_c<0>(ctx.attributes) = true;
        };

        static auto set_bool_false = [](const auto& attrib, auto& ctx) {
            bf::at_c<0>(ctx.attributes) = false;
        };

        static auto clear_string_buffer = [](const auto& attrib, const auto& ctx){
            DocumentBuilder::current()->clear_string_buffer();
        };

        static auto finish_value = [](auto& attrib, auto& ctx){
            ValueVisitor visitor;
            boost::apply_visitor(visitor, attrib);
            bf::at_c<0>(ctx.attributes) = visitor.value;
        };

        static auto set_doc_value = [](auto& attrib, auto& ctx){
            DocumentBuilder::current()->set_doc_value(bf::at_c<1>(attrib));
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

        sdn_value    = (
                        sdn_string_or_typed_value |
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


        BOOST_SPIRIT_DEBUG_NODE(sdn_document);
        BOOST_SPIRIT_DEBUG_NODE(type_declaration);
    }

    qi::real_parser<double, qi::strict_real_policies<double>> strict_double;

    qi::rule<Iterator, pool::SharedPtr<HermesDoc>(), qi::space_type> sdn_document;
    qi::rule<Iterator, ValuePtr(), qi::space_type> sdn_value;
    qi::rule<Iterator, ValuePtr(), qi::space_type> standalone_sdn_value;

    qi::rule<Iterator, ArrayValue(), qi::space_type> array;
    qi::rule<Iterator, MapValue(), qi::space_type> map;

    qi::rule<Iterator, MapEntryTuple(), qi::space_type> map_entry;

    qi::rule<Iterator, NullValue(), qi::space_type> null_value;

    qi::rule<Iterator, ParsedStringValue(), qi::space_type> quoted_string;
    qi::rule<Iterator, StringValuePtr(), qi::space_type> sdn_string;

    qi::rule<Iterator, StringOrTypedValue(), qi::space_type> sdn_string_or_typed_value;

    qi::rule<Iterator, ParsedStringValue(), qi::space_type> identifier;
    qi::rule<Iterator, ParsedStringValue(), qi::space_type> sdn_identifier;

    qi::rule<Iterator, TypeDeclarationValue(), qi::space_type> type_declaration;
    qi::rule<Iterator, DatatypePtr(), qi::space_type> standalone_type_decl;

    qi::rule<Iterator, TypedValueValue(), qi::space_type> typed_value;

    qi::rule<Iterator, TypeDirectoryValue(), qi::space_type> type_directory;

    qi::rule<Iterator, TypeDirectoryMapEntry(), qi::space_type> type_directory_entry;


    qi::rule<Iterator, StringValuePtr(), qi::space_type> type_reference;
    qi::rule<Iterator, TypeDeclOrReference(), qi::space_type> type_decl_or_reference;

    qi::rule<Iterator, bool(), qi::space_type> bool_value_true;
    qi::rule<Iterator, bool(), qi::space_type> bool_value_false;
    qi::rule<Iterator, bool(), qi::space_type> bool_value;
};


template <typename Iterator>
struct TypeDeclarationParser : qi::grammar<Iterator, DatatypePtr(), qi::space_type>
{
    HermesDocParser<Iterator> sdn_parser;

    TypeDeclarationParser() : TypeDeclarationParser::base_type(sdn_parser.standalone_type_decl)
    {}
};


template <typename Iterator>
struct RawHermesDocValueParser : qi::grammar<Iterator, ValuePtr(), qi::space_type>
{
    HermesDocParser<Iterator> hermes_doc_parser;

    RawHermesDocValueParser() : RawHermesDocValueParser::base_type(hermes_doc_parser.standalone_sdn_value)
    {}
};

template <typename Iterator>
struct HermesDocIdentifierParser : qi::grammar<Iterator, EmptyCharCollection(), qi::space_type>
{
    HermesDocParser<Iterator> sdn_parser;

    qi::rule<Iterator, EmptyCharCollection(), qi::space_type> raw_identifier;

    HermesDocIdentifierParser() : HermesDocIdentifierParser::base_type(raw_identifier)
    {
        using qi::lexeme;
        using qi::standard::char_;

        raw_identifier = (lexeme[(enc::alpha | char_('_')) >> *(enc::alnum | char_('_'))]
                          - "null" - "true" - "false");
    }
};


namespace {
    struct DocumentBuilderCleanup {
        ~DocumentBuilderCleanup() noexcept
        {
            DocumentBuilder::current(nullptr, true);
        }
    };
}

template <typename Iterator>
bool parse_sdn2(Iterator& first, Iterator& last, HermesDoc& doc)
{
    static thread_local HermesDocParser<Iterator> const grammar;

    DocumentBuilder builder(doc);
    DocumentBuilder::current(&builder);
    DocumentBuilderCleanup cleanup;

    bool r = qi::phrase_parse(first, last, grammar, qi::standard::space_type());

    if (first != last)
        return false;

    return r;
}



template <typename Iterator>
bool parse_sdn_type_decl(Iterator& first, Iterator& last, HermesDoc& doc)
{
    static thread_local TypeDeclarationParser<Iterator> const grammar;

    DocumentBuilder builder(doc);
    DocumentBuilder::current(&builder);
    DocumentBuilderCleanup cleanup;

    DatatypePtr type_decl{};
    bool r = qi::phrase_parse(first, last, grammar, enc::space, type_decl);

    builder.set_doc_value(type_decl->as_value());

    if (first != last)
        return false;

    return r;
}


template <typename Iterator>
bool parse_raw_sdn_type_decl(Iterator& first, Iterator& last, HermesDocView& doc, LDTypeDeclarationView& type_decl)
{
    static thread_local TypeDeclarationParser<Iterator> const grammar;

    DocumentBuilder builder(doc);
    DocumentBuilder::current(&builder);
    DocumentBuilderCleanup cleanup;

    bool r = qi::phrase_parse(first, last, grammar, enc::space, type_decl);

    if (first != last)
        return false;

    return r;
}

template <typename Iterator>
bool parse_raw_value0(Iterator& first, Iterator& last, HermesDocView& doc, ValuePtr& value)
{
    static thread_local RawHermesDocValueParser<Iterator> const grammar;

    DocumentBuilder builder(doc);
    DocumentBuilder::current(&builder);
    DocumentBuilderCleanup cleanup;

    bool r = qi::phrase_parse(first, last, grammar, enc::space, value);

    if (first != last)
        return false;

    return r;
}



template <typename Iterator>
bool parse_identifier(Iterator& first, Iterator& last)
{
    static thread_local HermesDocIdentifierParser<Iterator> const grammar;
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

        MMA_THROW(SDNParseException()) << format_ex("{} at {}: {}", msg, pos, buf.str());
    }
}





PoolSharedPtr<HermesDoc> HermesDoc::parse(CharIterator start, CharIterator end, const ParserConfiguration& cfg)
{
    PoolSharedPtr<HermesDoc> doc = TL_allocate_shared<HermesDoc>();

    auto tmp = start;

    bool result = parse_sdn2(start, end, *doc);

    assert_parse_ok(result, "Can't parse type document", tmp, start, end);

    return doc;
}

PoolSharedPtr<HermesDoc> HermesDoc::parse_datatype(CharIterator start, CharIterator end, const ParserConfiguration& cfg)
{
    PoolSharedPtr<HermesDoc> doc = TL_allocate_shared<HermesDoc>();

    auto tmp = start;

    bool result = parse_sdn_type_decl(start, end, *doc);

    assert_parse_ok(result, "Can't parse datatype declaration", tmp, start, end);

    return doc;
}

//LDTypeDeclarationView HermesDocView::parse_raw_type_decl(CharIterator start, CharIterator end, const SDNParserConfiguration& cfg)
//{
//    LDTypeDeclarationView type_decl{};

//    auto tmp = start;

//    bool result = parse_raw_sdn_type_decl(start, end, *this, type_decl);

//    assert_parse_ok(result, "Can't parse type declaration", tmp, start, end);

//    return type_decl;
//}

ValuePtr HermesDocView::parse_raw_value(CharIterator start, CharIterator end, const ParserConfiguration& cfg)
{
    ValuePtr value{};

    auto tmp = start;

    bool result = parse_raw_value0(start, end, *this, value);

    assert_parse_ok(result, "Can't parse Hermes value", tmp, start, end);

    return value;
}

bool HermesDocView::is_identifier(CharIterator start, CharIterator end)
{
    return parse_identifier(start, end);
}


/**/
}}
