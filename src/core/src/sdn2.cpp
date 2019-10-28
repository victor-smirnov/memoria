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

//#define BOOST_SPIRIT_DEBUG

#define BOOST_SPIRIT_UNICODE
//#define BOOST_SPIRIT_X3_DEBUG 1

#ifndef MMA1_NO_REACTOR
#   include <memoria/v1/reactor/reactor.hpp>
#endif

#include <memoria/v1/core/sdn/sdn2.hpp>


#include <memoria/v1/core/tools/type_name.hpp>

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
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

namespace x3 = boost::spirit::x3;
//namespace qi    = boost::spirit::qi;
namespace ascii = x3::ascii;
namespace bp    = boost::phoenix;
//namespace bs    = boost::spirit;

}}



namespace memoria {
namespace v1 {


class SDN2DocumentBuilder {
    SDN2Document& doc_;
public:
    SDN2DocumentBuilder(SDN2Document& doc):
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
};



namespace {

namespace parser {

    thread_local SDN2DocumentBuilder* builder = nullptr;


    using x3::lexeme;
    using x3::double_;
    using x3::lit;
    using ascii::char_;

//    const auto doc_set_string = [](const auto& ctx){
//        parser::builder->set_string(x3::_attr(ctx));
//    };

    const auto print_type = [](const auto& ctx){
        std::cout << TypeNameFactory<decltype(x3::_attr(ctx))>::name() << std::endl;
    };

    x3::real_parser<double, x3::strict_real_policies<double> > const strict_double_ = {};

    x3::rule<class doc>   const sdn_document = "sdn_document";
    x3::rule<class value> const sdn_value = "sdn_value";
    x3::rule<class array, SDN2Array> const array = "array";
    x3::rule<class map, SDN2Map>   const map = "map";
    x3::rule<class map_entry> const map_entry = "map_entry";

    const auto quoted_string = lexeme['\'' >> +(char_ - '\'') >> '\''] | lexeme['"' >> +(char_ - '"') >> '"'];
    const auto identifier    = lexeme[(x3::alpha | char_('_')) >> *(x3::alnum | char_('_'))];

    const auto array_def        = '[' >> (sdn_value % ',') >> ']' | lit('[') >> ']';

    const auto map_entry_def    = (quoted_string | identifier) >> ':' >> sdn_value;
    const auto map_def          = '{' >> (map_entry % ',') >> '}' | lit('{') >> '}';

    const auto sdn_value_def    = quoted_string | strict_double_ [print_type] | x3::int64 [print_type] | identifier | array[print_type] | (map[print_type]);
    const auto sdn_document_def = sdn_value;

    BOOST_SPIRIT_DEFINE(sdn_document, sdn_value, array, map, map_entry);
}

template <typename Iterator>
bool parse_sdn2(Iterator& first, Iterator& last, SDN2Document& doc)
{
    using x3::double_;
    using x3::phrase_parse;
    using x3::_attr;
    using ascii::space;

    SDN2DocumentBuilder builder(doc);

    parser::builder = &builder;

    bool r = phrase_parse(first, last,

        //  Begin grammar
        parser::sdn_document,
        //  End grammar

        space);

    parser::builder = nullptr;

    if (first != last)
        return false;

    return r;
}

}


SDN2Document SDN2Document::parse(U8StringView::const_iterator start, U8StringView::const_iterator end)
{
    std::vector<double> nums;

    SDN2Document doc;

    bool result = parse_sdn2(start, end, doc);

    if (!result) {
        MMA1_THROW(RuntimeException()) << WhatCInfo("Parse error");
    }

    return doc;
}


}}
