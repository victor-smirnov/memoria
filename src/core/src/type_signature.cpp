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
#include <boost/spirit/include/phoenix_object.hpp>

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

namespace qi    = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace bp    = boost::phoenix;

using TypeCtrParams = std::vector<DataTypeCtrArg>;

struct TypeDeclarationStruct;
using TypeParams = std::vector<TypeDeclarationStruct>;

struct TypeDeclarationStruct {
    std::vector<RawToken> name_tokens_;

    boost::optional<TypeParams> parameters_;
    boost::optional<TypeCtrParams> constructor_args_;

    DataTypeDeclaration convert()
    {
        DataTypeDeclaration decl;

        for (auto& token: name_tokens_) {
            decl.name_tokens().emplace_back(token.text());
        }

        if (constructor_args_.has_value()) {
            decl.constructor_args() =  std::move(constructor_args_);
        }

        if (parameters_.has_value())
        {
            auto& args = parameters_.get();
            std::vector<DataTypeDeclaration> dt_args;
            for (auto& arg: args)
            {
                dt_args.emplace_back(arg.convert());
            }

            decl.parameters() = std::move(dt_args);
        }

        return decl;
    }
};

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
        using qi::on_error;
        using qi::fail;
        using namespace qi::labels;

        using bp::construct;
        using bp::val;

        identifier    = lexeme[(qi::alpha | char_('_'))[_val += _1] >> *(qi::alnum | char_('_'))[_val += _1]];

        quoted_string %= lexeme['\'' >> +(char_ - '\'') >> '\''];

        type_name     = +identifier;

        constructor_arg %= identifier | strict_double | long_ | quoted_string | array;

        array %= '[' >> (constructor_arg % ',') >> ']' | lit('[') >> lit(']');

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
        BOOST_SPIRIT_DEBUG_NODE(constructor_args);
        BOOST_SPIRIT_DEBUG_NODE(type_parameters);
        BOOST_SPIRIT_DEBUG_NODE(type_declaration);
    }

    qi::real_parser<double, qi::strict_real_policies<double>> strict_double;

    qi::rule<Iterator, std::string(), qi::space_type> quoted_string;
    qi::rule<Iterator, RawToken(), qi::space_type> identifier;
    qi::rule<Iterator, std::vector<RawToken>(), qi::space_type> type_name;
    qi::rule<Iterator, DataTypeCtrArg(), qi::space_type> constructor_arg;
    qi::rule<Iterator, TypeCtrParams(), qi::space_type> array;
    qi::rule<Iterator, TypeCtrParams(), qi::space_type> constructor_args;
    qi::rule<Iterator, TypeParams(), qi::space_type> type_parameters;

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

TypeSignature::TypeSignature(U8StringView name): name_(parse(name).to_standard_string())
{}

DataTypeDeclaration TypeSignature::parse() const
{
    return parse(name_.to_std_string());
}

DataTypeDeclaration TypeSignature::parse(U8StringView str)
{
    static thread_local TypeSignatureParser<U8StringView::const_iterator> const grammar;

    TypeDeclarationStruct decl;

    auto ii = str.begin();
    bool result = qi::phrase_parse(ii, str.end(), grammar, qi::space_type(), decl);

    if ((!result) || ii != str.end())
    {
        MMA1_THROW(RuntimeException()) << fmt::format_ex(u"Can't parse data type signature: \"{}\", unparsed: \"{}\"", (std::string)str, (std::string)U8StringView(ii));
    }
    else {
        return decl.convert();
    }
}


class ArgPrintingVisitor: public boost::static_visitor<>
{
    SBuf& buf_;

public:
    ArgPrintingVisitor(SBuf& buf): buf_(buf)
    {}

    void operator()(long i) const
    {
        buf_ << i;
    }

    void operator()(double i) const
    {
        buf_ << std::scientific << i;
    }

    void operator()(const U8String& str) const
    {
        buf_ << "'" << str << "'";
    }

    void operator()(const RawToken& token) const
    {
        buf_ << token.text();
    }

    void operator()(const std::vector<DataTypeCtrArg>& array) const
    {
        buf_ << "[";

        bool first = true;

        for (auto& vv: array)
        {
            if (!first) {
                buf_ << ", ";
            }
            else {
                first = false;
            }

            boost::apply_visitor(*this, vv);
        }

        buf_ << "]";
    }
};

void DataTypeDeclaration::to_standard_string(SBuf& buf) const
{
    bool first_nm = true;
    for (auto& name_token: name_tokens_) {
        if (!first_nm) {
            buf << " ";
        }
        else {
            first_nm = false;
        }

        buf << name_token;
    }

    bool first_par = true;
    if (parameters_.has_value())
    {
        buf << "<";

        for (auto& param: parameters_.get())
        {
            if (!first_par) {
                buf << ", ";
            }
            else {
                first_par = false;
            }

            param.to_standard_string(buf);
        }

        buf << ">";
    }

    bool first_arg = true;
    if (constructor_args_.has_value())
    {
        buf << "(";

        ArgPrintingVisitor arg_visitor(buf);

        for (auto& arg: constructor_args_.get())
        {
            if (!first_arg) {
                buf << ", ";
            }
            else {
                first_arg = false;
            }

            boost::apply_visitor(arg_visitor, arg);
        }

        buf << ")";
    }
}


void DataTypeDeclaration::to_typedecl_string(SBuf& buf) const
{
    bool first_nm = true;
    for (auto& name_token: name_tokens_) {
        if (!first_nm) {
            buf << " ";
        }
        else {
            first_nm = false;
        }

        buf << name_token;
    }

    bool first_par = true;
    if (parameters_.has_value())
    {
        buf << "<";

        for (auto& param: parameters_.get())
        {
            if (!first_par) {
                buf << ", ";
            }
            else {
                first_par = false;
            }

            param.to_typedecl_string(buf);
        }

        buf << ">";
    }
}

U8String DataTypeDeclaration::full_type_name() const
{
    SBuf buf;
    bool first = true;

    for (auto& token: name_tokens_) {
        if (!first) {
            buf << " ";
        }
        else {
            first = false;
        }

        buf << token;
    }

    return buf.str();
}

}}
