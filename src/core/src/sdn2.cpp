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

#include <memoria/v1/core/linked/document/linked_document.hpp>


#include <memoria/v1/core/tools/type_name.hpp>

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/std_tuple.hpp>


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
namespace ascii = x3::ascii;


}}



namespace memoria {
namespace v1 {


class LDDocumentBuilder {

    ArenaBuffer<char> string_buffer_;
    LDDocument& doc_;

public:

    LDDocumentBuilder(LDDocument& doc):
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


        LDCharBufferBase()
        {
            builder_ = LDDocumentBuilder::current();
            builder_->clear_string_buffer();
        }

        using iterator = EmptyType;

        iterator end() {return iterator{};}

        void insert(iterator, value_type value) {
            builder_->append_char(value);
        }

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



    struct LDDValueVisitor: boost::static_visitor<> {

        LDDValue value;

        void operator()(long v){
            value = LDDocumentBuilder::current()->new_integer(v);
        }

        void operator()(double v){
            value = LDDocumentBuilder::current()->new_double(v);
        }

        void operator()(LDNullValue& v) {}

        void operator()(LDDMap& v) {}

        void operator()(LDString& v) {
            value = v;
        }

        template <typename V>
        void operator()(V& v){
            value = v.finish();
        }
    };


    using x3::lexeme;
    using x3::double_;
    using x3::lit;
    using ascii::char_;

    const auto dummy = [](auto){};

    const auto finish_string = [](auto& ctx) {
        x3::_val(ctx) = x3::_attr(ctx).finish();
    };

    const auto finish_identifier = [](auto& ctx) {
        x3::_val(ctx) = x3::_attr(ctx).finish();
    };


    const auto set_doc_value = [](auto& ctx){
        LDDocumentBuilder::current()->set_doc_value(x3::_attr(ctx));
    };

    const auto finish_value = [](auto& ctx){
        LDDValueVisitor visitor;
        boost::apply_visitor(visitor, x3::_attr(ctx));
        x3::_val(ctx) = visitor.value;
    };

    x3::real_parser<double, x3::strict_real_policies<double> > const strict_double_ = {};

    x3::rule<class doc>   const sdn_document = "sdn_document";
    x3::rule<class value, LDDValue> const sdn_value = "sdn_value";
    x3::rule<class array, LDDArrayValue> const array = "array";
    x3::rule<class map,   LDDMapValue> const map = "map";

    x3::rule<class map_entry, MapEntryTuple> const map_entry = "map_entry";

    x3::rule<class null_value, LDNullValue> const null_value = "null_value";

    x3::rule<class string_value, LDStringValue> const quoted_string = "quoted_string";
    x3::rule<class string_value, LDString> const sdn_string = "sdn_string";

    x3::rule<class identifier_value, LDIdentifierValue> const identifier = "identifier";
    x3::rule<class identifier_value, LDIdentifier> const sdn_identifier = "sdn_identifier";


    const auto quoted_string_def    = (lexeme['\'' >> +(char_ - '\'') >> '\''] | lexeme['"' >> +(char_ - '"') >> '"']);
    const auto sdn_string_def       = quoted_string [finish_string];


    const auto identifier_def       = (lexeme[(x3::alpha | char_('_')) >> *(x3::alnum | char_('_'))] - "null");
    const auto sdn_identifier_def   = identifier [finish_identifier];

    const auto null_value_def   = lexeme[lit("null")][dummy];

    const auto array_def        = '[' >> (sdn_value % ',') >> ']' |
                                        lit('[') >> ']';

    const auto map_entry_def    = (sdn_string  >> ':' >> sdn_value);

    const auto map_def          = '{' >> (map_entry % ',') >> '}' |
                                        lit('{') >> '}';

    const auto sdn_value_def    = (sdn_string |
                                    strict_double_ |
                                    x3::int64 | map |
                                    null_value |
                                    array
                                    )[finish_value];

    const auto sdn_document_def = sdn_value[set_doc_value];

    BOOST_SPIRIT_DEFINE(
            sdn_document, sdn_value, array, map, map_entry,
            identifier, sdn_identifier, quoted_string,
            sdn_string, null_value
    );
}

template <typename Iterator>
bool parse_sdn2(Iterator& first, Iterator& last, LDDocument& doc)
{
    using x3::double_;
    using x3::phrase_parse;
    using x3::_attr;
    using ascii::space;

    LDDocumentBuilder builder(doc);

    LDDocumentBuilder::current(&builder);

    bool r = phrase_parse(first, last,

        //  Begin grammar
        parser::sdn_document,
        //  End grammar

        space);

    LDDocumentBuilder::current(nullptr, true);

    if (first != last)
        return false;

    return r;
}

}


LDDocument LDDocument::parse(U8StringView::const_iterator start, U8StringView::const_iterator end)
{
    std::vector<double> nums;

    LDDocument doc;

    bool result = parse_sdn2(start, end, doc);

    if (!result) {
        MMA1_THROW(RuntimeException()) << WhatCInfo("Parse error");
    }

    return doc;
}


}}
