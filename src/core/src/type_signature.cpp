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

#include <memoria/v1/api/datatypes/type_signature.hpp>

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
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
namespace ascii = boost::spirit::ascii;
namespace bp = boost::phoenix;

class RawToken {
    U8String text_;
public:
    RawToken(U8String text): text_(text) {}

    const U8String& text() const {
        return text_;
    }

    U8String& text() {
        return text_;
    }
};

std::ostream& operator<<(std::ostream& out, const RawToken& tk) {
    out << "RawToken[" << tk.text() << "]";
    return out;
}

using TypeCtrArg = boost::variant<U8String, int64_t, double, RawToken>;
using TypeCtrParams = boost::optional<std::vector<TypeCtrArg>>;



struct TypeDeclarationStruct {
    std::vector<U8String> name_tokens_;

    TypeCtrParams constructor_args_;
    std::vector<TypeDeclarationStruct> parameters_;
};



class TypeDeclaration {
    std::vector<U8String> name_tokens_;
    bool default_{};
    std::vector<TypeCtrArg> constructor_args_;
    std::vector<TypeDeclaration> parameters_;
public:

    std::vector<U8String>& name_tokens() {return name_tokens_;}
    const std::vector<U8String>& name_tokens() const {return name_tokens_;}

    bool is_default() const {
        return default_;
    }

    void set_non_default()
    {
        this->default_ = false;
    }

    std::vector<TypeCtrArg>& constructor_args() {return constructor_args_;}
    const std::vector<TypeCtrArg>& constructor_args() const {return constructor_args_;}

    std::vector<TypeDeclaration>& parameters() {return parameters_;}
    const std::vector<TypeDeclaration>& parameters() const {return parameters_;}
};

//using TypeDeclTuple = std::tuple<std::vector<U8String>(), std::vector<TypeCtrArg>()>;

}}

BOOST_FUSION_ADAPT_STRUCT(memoria::v1::TypeDeclarationStruct, name_tokens_, parameters_, constructor_args_)

namespace memoria {
namespace v1 {

template <typename Iterator>
struct TypeSignatureParser : qi::grammar<Iterator, TypeDeclarationStruct(), qi::space_type>
{
    TypeSignatureParser() : TypeSignatureParser::base_type(type_declaration)
    {
        using qi::long_;
        using qi::lit;
        using qi::double_;
        using qi::lexeme;
        using ascii::char_;
        using qi::eps;
        using qi::_1;
        using qi::_val;

        identifier    %= lexeme[(qi::alpha | char_('_')) >> *(qi::alnum | char_('_'))];

        quoted_string %= lexeme['\'' >> +(char_ - '\'') >> '\''];
        type_name     = +identifier;
        constructor_arg %= identifier | strict_double | long_ | quoted_string;

        //constructor_args_list %= constructor_arg % ',';

        constructor_args %= '(' >> (constructor_arg % ',') >> ')' | lit('(') >> lit(')');

        type_parameters %= lit('<') >> lit('>') |
                          '<' >> type_declaration % ','  >> '>';

        type_declaration %=
            type_name
            >> -type_parameters
            >> -constructor_args
        ;

        BOOST_SPIRIT_DEBUG_NODE(identifier);
        BOOST_SPIRIT_DEBUG_NODE(quoted_string);
        BOOST_SPIRIT_DEBUG_NODE(type_name);
        BOOST_SPIRIT_DEBUG_NODE(constructor_arg);
//        BOOST_SPIRIT_DEBUG_NODE(constructor_args_list);
        BOOST_SPIRIT_DEBUG_NODE(constructor_args);
        BOOST_SPIRIT_DEBUG_NODE(type_parameters);
        BOOST_SPIRIT_DEBUG_NODE(type_declaration);

//        debug(identifier);
//        debug(quoted_string);
//        debug(type_name);
//        debug(constructor_arg);
//        debug(constructor_args_list);
//        debug(constructor_args);
//        debug(type_parameters);
//        debug(type_declaration);
    }

    qi::real_parser<double, qi::strict_real_policies<double>> strict_double;

    qi::rule<Iterator, std::string(), qi::space_type> quoted_string;
    qi::rule<Iterator, std::string(), qi::space_type> identifier;
    qi::rule<Iterator, std::vector<U8String>(), qi::space_type> type_name;
    qi::rule<Iterator, std::vector<TypeCtrArg>, qi::space_type> constructor_args;
    qi::rule<Iterator, TypeCtrArg(), qi::space_type> constructor_arg;
//    qi::rule<Iterator, std::vector<TypeCtrArg>(), qi::space_type> constructor_args_list;
    qi::rule<Iterator, std::vector<TypeDeclarationStruct>(), qi::space_type> type_parameters;

    qi::rule<Iterator, TypeDeclarationStruct(), qi::space_type> type_declaration;
};

template <typename TT>
std::ostream& operator<<(std::ostream& out, const std::vector<TT>& vec)
{
    out << "vector[";

    bool first = true;

    for (auto& vv: vec)
    {
        if (!first) {
            out << ", ";
        }
        else {
            first = false;
        }

        out << vv;
    }

    out << "]";
    return out;
}

void TypeSignature::parse(absl::string_view str)
{
    TypeSignatureParser<const char*> grammar;

    TypeDeclarationStruct decl;

    auto ii = str.begin();
    bool result = qi::phrase_parse(ii, str.end(), grammar, qi::space_type(), decl);

    result = result && ii == str.end();

    std::cerr << "VArIANT: " << decl.constructor_args_ << std::endl;

    std::cerr << std::flush;

    std::cerr << (result ? "SUCCESS" : "FAIL") << std::endl;
}


}}
