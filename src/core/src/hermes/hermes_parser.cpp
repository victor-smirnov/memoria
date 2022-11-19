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

#define BOOST_SPIRIT_UNICODE

#ifndef MMA_NO_REACTOR
#   include <memoria/reactor/reactor.hpp>
#endif

#include <memoria/core/hermes/hermes.hpp>
#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/strings/u32_string.hpp>

#include <memoria/core/flat_map/flat_hash_map.hpp>

#include <memoria/core/hermes/path/types.h>


#include "hermes_internal.hpp"
#include "hermes_grammar_value.hpp"

//#include "path/parser/hermes_grammar_path.hpp"

#include "path/parser/hermes_path_ast_converter.h"


#include <memoria/core/hermes/path/expression.h>



#include "path/ast/allnodes.h"



#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>

namespace memoria {
namespace hermes {

using IteratorType = path::U8UnicodeIteratorAdaptor;


namespace qi  = boost::spirit::qi;
namespace enc = qi::unicode;
namespace bf  = boost::fusion;
namespace bp  = boost::phoenix;

template <typename Iterator>
using SkipperT = qi::rule<Iterator>;

template <typename Iterator>
struct HermesDocParser :
    HermesValueRulesLib<
        Iterator,
        SkipperT<Iterator>
    >,
    qi::grammar<
        Iterator,
        pool::SharedPtr<HermesCtr>(),
        SkipperT<Iterator>
    >
{
    using RuleLib = HermesValueRulesLib <
        Iterator,
        SkipperT<Iterator>
    >;

    using Skipper = SkipperT<Iterator>;

    using RuleLib::m_identifierRule;
    using RuleLib::m_rawStringRule;
    using RuleLib::m_quotedStringRule;

    using RuleLib::hermes_value;
    using RuleLib::hermes_identifier;
    using RuleLib::type_declaration;

    HermesDocParser() :
        HermesDocParser::base_type(
            hermes_document
        )
    {

        using qi::long_;
        using qi::lit;
        using qi::lexeme;
        using enc::char_;
        using qi::_val;
        using qi::eol;
        using qi::eoi;
        using qi::_pass;
        using qi::fail;
        using qi::_1;

        using bp::construct;
        using bp::val;

        static auto set_doc_value = [](auto& attrib, auto&){
            HermesCtrBuilder::current().set_ctr_root(bf::at_c<1>(attrib));
        };

        type_directory_entry = hermes_identifier >> ':' >> type_declaration;
        type_directory = ("#{" >> (type_directory_entry % ',') >> '}') | (lit("#{") > "}");
        hermes_document = (-type_directory > hermes_value) [set_doc_value];

        single_line_comment = "//" >> *(char_ - eol) >> (eol|eoi);
        skipper = qi::space | single_line_comment;

        skipper.name("skipper");
        single_line_comment.name("single_line_comment");

        type_directory.name("type_directory");
        type_directory_entry.name("type_directory_entry");
    }

    qi::rule<Iterator, PoolSharedPtr<HermesCtr>(), Skipper> hermes_document;
    qi::rule<Iterator, TypeDirectoryValue(), Skipper>   type_directory;
    qi::rule<Iterator, TypeDirectoryMapEntry(), Skipper> type_directory_entry;

    qi::rule<Iterator> skipper;
    qi::rule<Iterator> single_line_comment;
};

template <typename Iterator>
using ExpectationException = boost::wrapexcept<boost::spirit::qi::expectation_failure<Iterator> >;


class ErrorMessageResolver {
    using MessageProducerFn = std::function<U8String (size_t, size_t, size_t, U8StringView)>;

    ska::flat_hash_map<U8String, MessageProducerFn> map_;
public:
    ErrorMessageResolver() noexcept {

    }

    U8String get_message(U8StringView rule_name, size_t row, size_t column, size_t abs_pos, U8StringView context) noexcept
    {
        auto ii = map_.find(rule_name);
        if (ii != map_.end()) {
            return ii->second(row, column, abs_pos, context);
        }

        return fmt::format("Parse error in Hermes document, line {}:{}, pos: {}, at: {}", row, column, abs_pos, context);
    }

    static ErrorMessageResolver& instance() noexcept {
        static ErrorMessageResolver resolver;
        return resolver;
    }

    struct ErrDescripton {
        size_t line;
        size_t column;
        size_t abs;
        U8String context;
    };

    template <typename Iterator>
    ErrDescripton process_spirit_expect_exception(
            Iterator start,
            Iterator err_head,
            Iterator end
    )
    {
        //start, ex.first, ex.last
        ptrdiff_t pos = std::distance(start, err_head);

        size_t line = 1;
        size_t column = 1;

        while (start != err_head) {
            switch(*start) {
            case '\t': column += 4; break;
            case '\n': line++; column = 1; break;
            case '\r': break;
            default: column++;
            }

            start++;
        }

        U8String buf;
        bool add_ellipsis = true;
        for (size_t c = 0; c < 125; c++)
        {
            auto out_ii = std::back_inserter(buf.to_std_string());
            boost::utf8_output_iterator<decltype(out_ii)> utf8_out_ii(out_ii);
            *utf8_out_ii++ = *err_head;

            if (++err_head == end) {
                add_ellipsis = false;
                break;
            }
        }

        if (add_ellipsis) {
            buf += "...";
        }

        return ErrDescripton{line, column, (size_t)pos, buf};
    }

    template <typename Iterator>
    void do_throw(
            Iterator start,
            const ExpectationException<Iterator>& ex
    ) {
        ErrDescripton descr = process_spirit_expect_exception(start, ex.first, ex.last);
        U8String message = this->get_message(ex.what_.tag, descr.line, descr.column, descr.abs, descr.context);
        MEMORIA_MAKE_GENERIC_ERROR("{}", message).do_throw();
    }

    template <typename Iterator>
    void do_throw(
            Iterator start,
            Iterator err_head,
            Iterator end
    ) {
        ErrDescripton descr = process_spirit_expect_exception(start, err_head, end);
        U8String message = this->get_message("unknown-node", descr.line, descr.column, descr.abs, descr.context);
        MEMORIA_MAKE_GENERIC_ERROR("{}", message).do_throw();
    }
};


template <typename Iterator>
struct TypeDeclarationParser : qi::grammar<Iterator, DatatypePtr(), SkipperT<Iterator>>
{
    HermesDocParser<Iterator> hermes_parser;
    TypeDeclarationParser() : TypeDeclarationParser::base_type(hermes_parser.standalone_type_decl)
    {}
};


template <typename Iterator>
struct RawHermesDocValueParser : qi::grammar<Iterator, ObjectPtr(), SkipperT<Iterator>>
{
    HermesDocParser<Iterator> hermes_doc_parser;
    RawHermesDocValueParser() : RawHermesDocValueParser::base_type(hermes_doc_parser.standalone_hermes_value)
    {}
};

template <typename Iterator>
struct HermesDocIdentifierParser : qi::grammar<Iterator, EmptyCharCollection(), SkipperT<Iterator>>
{
    qi::rule<Iterator, EmptyCharCollection(), SkipperT<Iterator>> raw_identifier;

    HermesDocIdentifierParser() : HermesDocIdentifierParser::base_type(raw_identifier)
    {
        using qi::lexeme;
        using qi::standard::char_;

        raw_identifier = (lexeme[(enc::alpha | char_('_')) >> *(enc::alnum | char_('_'))]
                - "null" - "true" - "false" - "const" - "volatile" - "signed"
                - "unsigned" - "int" - "long" - "char" - "double" - "float" - "short" - "bool"
        );
    }
};





template <typename Iterator>
void parse_hermes_document(Iterator& first, Iterator& last, HermesCtr& doc)
{
    HermesCtrBuilderCleanup cleanup;
    HermesCtrBuilder::enter(doc.self());

    HermesDocParser<Iterator> const grammar;

    Iterator start = first;
    try {
        bool r = qi::phrase_parse(first, last, grammar, grammar.skipper); //qi::standard::space_type()
        if (!r) {
            MEMORIA_MAKE_GENERIC_ERROR("Hermes document parse failure").do_throw();
        }
        else if (first != last) {
            ErrorMessageResolver::instance().do_throw(start, first, last);
        }
    }
    catch (const ExpectationException<Iterator>& ex) {
        ErrorMessageResolver::instance().do_throw(start, ex);
    }
}



template <typename Iterator>
void parse_datatype_decl(Iterator& first, Iterator& last, HermesCtr& doc)
{
    HermesCtrBuilderCleanup cleanup;
    HermesCtrBuilder::enter(doc.self());

    TypeDeclarationParser<Iterator> const grammar;

    Iterator start = first;
    try {
        DatatypePtr type_decl{};
        bool r = qi::phrase_parse(first, last, grammar, grammar.hermes_parser.skipper, type_decl);

        if (!r) {
            MEMORIA_MAKE_GENERIC_ERROR("Hermes datatype parse failure").do_throw();
        }
        else if (first != last) {
            ErrorMessageResolver::instance().do_throw(start, first, last);
        }

        HermesCtrBuilder::current().set_ctr_root(type_decl->as_object());
    }
    catch (const ExpectationException<Iterator>& ex) {
        ErrorMessageResolver::instance().do_throw(first, ex);
    }
}


//template <typename Iterator>
//void parse_hermes_path_expr(Iterator& first, Iterator& last, HermesCtr& doc)
//{
//    HermesCtrBuilderCleanup cleanup;
//    HermesCtrBuilder::enter(doc.self());

//    static thread_local HermesDocParser<Iterator> const grammar;

//    Iterator start = first;
//    try {
//        bool r = qi::phrase_parse(first, last, grammar, grammar.skipper);
//        if (!r) {
//            MEMORIA_MAKE_GENERIC_ERROR("Hermes document parse failure").do_throw();
//        }
//        else if (first != last) {
//            ErrorMessageResolver::instance().do_throw(start, first, last);
//        }
//    }
//    catch (const ExpectationException<Iterator>& ex) {
//        ErrorMessageResolver::instance().do_throw(start, ex);
//    }
//}



template <typename Iterator>
void parse_raw_datatype_decl(Iterator& first, Iterator& last, HermesCtr& doc, DatatypePtr& datatype)
{
    HermesCtrBuilderCleanup cleanup;
    HermesCtrBuilder::enter(doc.self());

    TypeDeclarationParser<Iterator> const grammar;

    Iterator start = first;
    try {
        bool r = qi::phrase_parse(first, last, grammar, grammar.hermes_parser.skipper, datatype);

        if (!r) {
            MEMORIA_MAKE_GENERIC_ERROR("Hermes datatype parse failure").do_throw();
        }
        else if (first != last) {
            ErrorMessageResolver::instance().do_throw(start, first, last);
        }
    }
    catch (const ExpectationException<Iterator>& ex) {
        ErrorMessageResolver::instance().do_throw(first, ex);
    }
}

template <typename Iterator>
void parse_raw_value0(Iterator& first, Iterator& last, HermesCtr& doc, ObjectPtr& value)
{
    HermesCtrBuilderCleanup cleanup;
    HermesCtrBuilder::enter(doc.self());

    RawHermesDocValueParser<Iterator> const grammar;

    Iterator start = first;
    try {
        bool r = qi::phrase_parse(first, last, grammar, grammar.hermes_doc_parser.skipper, value);

        if (!r) {
            MEMORIA_MAKE_GENERIC_ERROR("Hermes value parse failure").do_throw();
        }
        else if (first != last) {
            ErrorMessageResolver::instance().do_throw(start, first, last);
        }
    }
    catch (const ExpectationException<Iterator>& ex) {
        ErrorMessageResolver::instance().do_throw(first, ex);
    }
}



template <typename Iterator>
bool parse_identifier(Iterator& first, Iterator& last)
{
    HermesDocIdentifierParser<Iterator> const grammar;

    SkipperT<Iterator> skipper = qi::space;
    bool r = qi::phrase_parse(first, last, grammar, skipper);

    if (first != last)
        return false;

    return r;
}

PoolSharedPtr<HermesCtr> HermesCtr::parse_document(CharIterator start, CharIterator end, const ParserConfiguration&)
{
    PoolSharedPtr<HermesCtrImpl> doc = TL_get_reusable_shared_instance<HermesCtrImpl>();

    // retain the value of the begin iterator
    IteratorType beginIt(start);
    IteratorType it = beginIt;
    IteratorType endIt(end);

    parse_hermes_document(it, endIt, *doc);
    return doc;
}

PoolSharedPtr<HermesCtr> HermesCtr::parse_datatype(CharIterator start, CharIterator end, const ParserConfiguration&)
{
    PoolSharedPtr<HermesCtrImpl> doc = TL_get_reusable_shared_instance<HermesCtrImpl>();

    IteratorType beginIt(start);
    IteratorType it = beginIt;
    IteratorType endIt(end);

    parse_datatype_decl(it, endIt, *doc);
    return doc;
}

DatatypePtr HermesCtr::parse_raw_datatype(CharIterator start, CharIterator end, const ParserConfiguration&)
{
    IteratorType beginIt(start);
    IteratorType it = beginIt;
    IteratorType endIt(end);

    DatatypePtr datatype{};
    parse_raw_datatype_decl(it, endIt, *this, datatype);
    return datatype;
}

ObjectPtr HermesCtr::parse_raw_value(CharIterator start, CharIterator end, const ParserConfiguration&)
{
    IteratorType beginIt(start);
    IteratorType it = beginIt;
    IteratorType endIt(end);

    ObjectPtr value{};
    parse_raw_value0(it, endIt, *this, value);
    return value;
}

bool HermesCtr::is_identifier(CharIterator start, CharIterator end)
{
    IteratorType beginIt(start);
    IteratorType it = beginIt;
    IteratorType endIt(end);

    return parse_identifier(it, endIt);
}

void HermesCtr::assert_identifier(U8StringView name)
{
    if (!is_identifier(name)) {
        MEMORIA_MAKE_GENERIC_ERROR("Supplied value '{}' is not a valid Hermes identifier", name).do_throw();
    }
}

PoolSharedPtr<HermesCtr> HermesCtr::parse_hermes_path(U8StringView text)
{
    PoolSharedPtr<HermesCtrImpl> doc = TL_get_reusable_shared_instance<HermesCtrImpl>();

//    IteratorType beginIt(text.begin());
//    IteratorType it = beginIt;
//    IteratorType endIt(text.end());

//    parse_hermes_path_expr(it, endIt, *doc);

    HermesCtrBuilderCleanup cleanup;
    HermesCtrBuilder::enter(doc->self());

    U8String ee = text;
    path::Expression exp(ee.to_std_string());

    const path::ast::ExpressionNode* root = exp.astRoot();

    path::parser::HermesASTConverter cvt;
    cvt.visit(root);

    if (cvt.context().is_initialized()) {
        doc->set_root(cvt.context().get());
    }

    return doc;
}

void HermesCtr::init_hermes_doc_parser() {
    // Init Resolver
    ErrorMessageResolver::instance();
}

}}
