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


#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace memoria {
namespace v1 {

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

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

using TypeCtrArg = boost::variant<U8String, int64_t, double, RawToken>;


struct TypeDeclarationStruct {
    std::vector<U8String> name_tokens_;
    bool default_{true};
    std::vector<TypeCtrArg> constructor_args_;
    std::vector<TypeDeclarationStruct> parameters_;
};



class TypeDeclaration {
    std::vector<U8String> name_tokens_;
    bool default_{true};
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

using TypeDeclTuple = std::tuple<std::vector<std::string>(), std::vector<TypeCtrArg>()>;

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

        identifier    = (qi::alpha | char_('_')) >> *(qi::alnum | char_('_'));
        quoted_string = lexeme['\'' >> +(char_ - '\'') >> '\''];
        type_name     = +identifier;
        constructor_arg = identifier | double_ | quoted_string;

        constructor_args_list = constructor_arg % ',';

        constructor_args = lit('(') >> ')' |
                           '(' >> constructor_args_list >> ')';

        type_parameters = lit('<') >> lit('>') |
                          lit('<') >> type_declaration % lit(',')  >> lit('>');

        type_declaration =
            type_name
            >> -type_parameters
            >> -constructor_args
        ;
    }

    qi::rule<Iterator, std::string(), qi::space_type> quoted_string;
    qi::rule<Iterator, std::string(), qi::space_type> identifier;
    qi::rule<Iterator, std::vector<std::string>(), qi::space_type> type_name;
    qi::rule<Iterator, std::vector<TypeCtrArg>(), qi::space_type> constructor_args;
    qi::rule<Iterator, TypeCtrArg(), qi::space_type> constructor_arg;
    qi::rule<Iterator, std::vector<TypeCtrArg>(), qi::space_type> constructor_args_list;
    qi::rule<Iterator, std::vector<TypeDeclarationStruct>(), qi::space_type> type_parameters;

    qi::rule<Iterator, TypeDeclarationStruct(), qi::space_type> type_declaration;
};

void TypeSignature::parse(absl::string_view str)
{
    std::string text = "Real<Boo<bb>>(1, 2, foo, '4', 5)";

    str = text;

    TypeSignatureParser<const char*> grammar;

    TypeDeclarationStruct decl;

    bool result = qi::phrase_parse(str.begin(), str.end(), grammar, qi::space_type(), decl);

    std::cout << (result ? "SUCCESS" : "FAIL") << std::endl;
}


}}
