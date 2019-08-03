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

#ifndef MMA1_NO_REACTOR
#   include <memoria/v1/reactor/reactor.hpp>
#endif

#include <memoria/v1/api/datatypes/type_signature.hpp>
#include <memoria/v1/api/datatypes/sdn.hpp>

#include <memoria/v1/core/iovector/io_buffer_base.hpp>

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
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

namespace qi    = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace bp    = boost::phoenix;
namespace bs    = boost::spirit;

}}

BOOST_FUSION_ADAPT_STRUCT(memoria::v1::DataTypeCtrArg, value_);
BOOST_FUSION_ADAPT_STRUCT(memoria::v1::DataTypeArgMapEntry<memoria::v1::DataTypeCtrArg>, key_, value_);
BOOST_FUSION_ADAPT_STRUCT(memoria::v1::DataTypeArgMap<memoria::v1::DataTypeCtrArg>, entries_);

BOOST_FUSION_ADAPT_STRUCT(memoria::v1::DataTypeDeclaration, name_tokens_, parameters_, constructor_args_);
BOOST_FUSION_ADAPT_STRUCT(memoria::v1::TypedStringValue, text_, type_);

BOOST_FUSION_ADAPT_STRUCT(memoria::v1::SDNValue, value_);
BOOST_FUSION_ADAPT_STRUCT(memoria::v1::SDNEntry, key_, value_);
BOOST_FUSION_ADAPT_STRUCT(memoria::v1::TypedSDNArray, array_, type_);
BOOST_FUSION_ADAPT_STRUCT(memoria::v1::TypedSDNMap, entries_, type_);

BOOST_FUSION_ADAPT_STRUCT(memoria::v1::SDNTypeDictionaryEntry, type_id_, type_);
BOOST_FUSION_ADAPT_STRUCT(memoria::v1::SDNTypeDictionary, entries_);
BOOST_FUSION_ADAPT_STRUCT(memoria::v1::SDNDocument, type_dictionary_, value_);


namespace absl {

std::ostream& operator<<(std::ostream& out, const ::absl::string_view& str)
{
    return out.write(str.begin(), str.size());
}

std::stringstream& operator<<(std::stringstream& out, const ::absl::string_view& str)
{
    out.write(str.begin(), str.size());

    return out;
}

}

namespace memoria {
namespace v1 {

std::ostream& operator<<(std::ostream& out, const boost::recursive_wrapper<memoria::v1::DataTypeDeclaration>& decl)
{
    return out;
}

template <typename Iterator>
struct TypeSignatureParser : qi::grammar<Iterator, DataTypeDeclaration(), qi::standard::space_type>
{
    TypeSignatureParser() : TypeSignatureParser::base_type(type_declaration)
    {
        using qi::long_;
        using qi::lit;
        using qi::lexeme;
        using qi::standard::char_;
        using qi::_1;
        using qi::_val;

        using bp::construct;
        using bp::val;

        identifier    = lexeme[(qi::alpha | char_('_'))[_val += _1] >> *(qi::alnum | char_('_'))[_val += _1]];

        quoted_string = lexeme['\'' >> *(char_ - '\'' - "\\\'" | '\\' >> char_('\'')) >> '\'']
                        | lexeme['"' >> *(char_ - '"' - "\\\"" | '\\' >> char_('"')) >> '"'] ;

        type_name     = +identifier;

        typed_string = quoted_string >> -('@' >> type_declaration);

        arg_map_entry = (quoted_string | identifier) >> ':' >> constructor_arg;

        arg_map = '{' >> (arg_map_entry % ',') >> '}' | lit('{') >> '}';

        constructor_arg = identifier | strict_double | long_ | typed_string  | array | arg_map;

        array = '[' >> (constructor_arg % ',') >> ']' | lit('[') >> lit(']');

        constructor_args = '(' >> (constructor_arg % ',') >> ')' | lit('(') >> lit(')');

        type_parameters = lit('<') >> lit('>') |
                '<' >> type_declaration % ','  >> '>';

        type_declaration =
            type_name
            >> -type_parameters
            >> -constructor_args
        ;

        BOOST_SPIRIT_DEBUG_NODE(identifier);
        BOOST_SPIRIT_DEBUG_NODE(quoted_string);
        BOOST_SPIRIT_DEBUG_NODE(type_name);
        BOOST_SPIRIT_DEBUG_NODE(array);
        BOOST_SPIRIT_DEBUG_NODE(typed_string);
        BOOST_SPIRIT_DEBUG_NODE(constructor_arg);
        BOOST_SPIRIT_DEBUG_NODE(constructor_args);
        BOOST_SPIRIT_DEBUG_NODE(type_parameters);
        BOOST_SPIRIT_DEBUG_NODE(type_declaration);
        BOOST_SPIRIT_DEBUG_NODE(arg_map_entry);
        BOOST_SPIRIT_DEBUG_NODE(arg_map);
    }

    qi::real_parser<double, qi::strict_real_policies<double>> strict_double;

    qi::rule<Iterator, std::string(), qi::space_type> quoted_string;
    qi::rule<Iterator, NameToken(), qi::space_type> identifier;
    qi::rule<Iterator, std::vector<NameToken>(), qi::space_type> type_name;
    qi::rule<Iterator, TypedStringValue(), qi::space_type> typed_string;
    qi::rule<Iterator, DataTypeCtrArg(), qi::space_type> constructor_arg;
    qi::rule<Iterator, std::vector<DataTypeCtrArg>(), qi::space_type> array;
    qi::rule<Iterator, DataTypeCtrArgs(), qi::space_type> constructor_args;
    qi::rule<Iterator, DataTypeParams(), qi::space_type> type_parameters;

    qi::rule<Iterator, DataTypeArgMapEntry<DataTypeCtrArg>(), qi::space_type> arg_map_entry;
    qi::rule<Iterator, DataTypeArgMap<DataTypeCtrArg>(), qi::space_type> arg_map;


    qi::rule<Iterator, DataTypeDeclaration(), qi::space_type> type_declaration;
};


template <typename Iterator>
struct SDNParser : qi::grammar<Iterator, SDNDocument(), qi::standard::space_type>
{
    TypeSignatureParser<Iterator> ts_parser_;

    SDNParser() : SDNParser::base_type(sdn_document)
    {
        using qi::long_;
        using qi::lit;

        auto& identifier        = ts_parser_.identifier;
        auto& strict_double     = ts_parser_.strict_double;
        auto& typed_string      = ts_parser_.typed_string;
        auto& type_declaration  = ts_parser_.type_declaration;
        auto& quoted_string     = ts_parser_.quoted_string;

        sdn_value = identifier | strict_double | long_
                    | typed_string
                    | sdn_array | sdn_map
                    | '@' >> type_declaration;

        sdn_document = -sdn_type_dictionary >> sdn_value;

        sdn_array_data  = '[' >> (sdn_value % ',') >> ']' | lit('[') >> ']';
        sdn_array       = sdn_array_data >>  -('@' >> type_declaration);

        sdn_map_entry   = (quoted_string | identifier) >> ':' >> sdn_value;
        sdn_map_entries = '{' >> (sdn_map_entry % ',') >> '}' | lit('{') >> '}';
        sdn_map         = sdn_map_entries >> -('@' >> type_declaration);

        sdn_dictionary_entry   = identifier >> ':' >> type_declaration;
        sdn_dictionary_entries = "#{" >> (sdn_dictionary_entry % ',') >> '}' | lit("#{") >> '}';
        sdn_type_dictionary    = sdn_dictionary_entries;



        BOOST_SPIRIT_DEBUG_NODE(sdn_array_data);
        BOOST_SPIRIT_DEBUG_NODE(sdn_array);
        BOOST_SPIRIT_DEBUG_NODE(sdn_map_entry);
        BOOST_SPIRIT_DEBUG_NODE(sdn_map_entries);
        BOOST_SPIRIT_DEBUG_NODE(sdn_map);
        BOOST_SPIRIT_DEBUG_NODE(sdn_value);
        BOOST_SPIRIT_DEBUG_NODE(sdn_dictionary_entry);
        BOOST_SPIRIT_DEBUG_NODE(sdn_dictionary_entries);
        BOOST_SPIRIT_DEBUG_NODE(sdn_type_dictionary);
        BOOST_SPIRIT_DEBUG_NODE(sdn_document);
    }

    qi::rule<Iterator, std::vector<SDNValue>(), qi::space_type> sdn_array_data;
    qi::rule<Iterator, TypedSDNArray(), qi::space_type> sdn_array;

    qi::rule<Iterator, SDNEntry(), qi::space_type> sdn_map_entry;
    qi::rule<Iterator, std::vector<SDNEntry>(), qi::space_type> sdn_map_entries;
    qi::rule<Iterator, TypedSDNMap(), qi::space_type> sdn_map;

    qi::rule<Iterator, SDNValue(), qi::space_type> sdn_value;

    qi::rule<Iterator, SDNTypeDictionaryEntry(), qi::space_type> sdn_dictionary_entry;
    qi::rule<Iterator, std::vector<SDNTypeDictionaryEntry>(), qi::space_type> sdn_dictionary_entries;
    qi::rule<Iterator, SDNTypeDictionary(), qi::space_type> sdn_type_dictionary;

    qi::rule<Iterator, SDNDocument(), qi::space_type> sdn_document;
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

    DataTypeDeclaration decl;

    auto ii = str.begin();
    bool result = qi::phrase_parse(ii, str.end(), grammar, qi::standard::space_type(), decl);

    if ((!result) || ii != str.end())
    {
        MMA1_THROW(RuntimeException()) << fmt::format_ex(u"Can't parse data type signature: \"{}\", unparsed: \"{}\"", (std::string)str, (std::string)U8StringView(ii));
    }
    else {
        return std::move(decl);
    }
}

SDNDocument SDNDocument::parse(U8StringView str)
{
    static thread_local SDNParser<U8StringView::const_iterator> const grammar;

    SDNDocument decl;

    auto ii = str.begin();
    bool result = qi::phrase_parse(ii, str.end(), grammar, qi::standard::space_type(), decl);

    if ((!result) || ii != str.end())
    {
        MMA1_THROW(RuntimeException()) << fmt::format_ex(u"Can't parse data type signature: \"{}\", unparsed: \"{}\"", (std::string)str, (std::string)U8StringView(ii));
    }
    else {
        decl.resolve_maps();
        return std::move(decl);
    }
}


class VisitorBase: public boost::static_visitor<> {
protected:
    mutable io::DefaultIOBuffer<char> escaping_{64};


    bool has_quotes(U8StringView str) const
    {
        for (auto& ch: str) {
            if (ch == '\'') return true;
        }

        return false;
    }

public:
    U8StringView escape_quotes(const U8String& str) const
    {
        if (!has_quotes(str.to_std_string())) {
            return str;
        }
        else {
            escaping_.clear();

            for (auto& ch: str.to_std_string())
            {
                if (MMA1_UNLIKELY(ch == '\''))
                {
                    escaping_.append_value('\\');
                }

                escaping_.append_value(ch);
            }

            escaping_.append_value(0);

            return U8StringView(escaping_.tail_ptr(), escaping_.size() - 1);
        }
    }
};


class ArgPrintingVisitor: public VisitorBase
{
protected:
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

    void operator()(const TypedStringValue& str) const
    {
        buf_ << "'" << escape_quotes(str) << "'";
        if (str.has_type())
        {
            buf_ << "@";
            str.type().to_standard_string(buf_);
        }
    }

    void operator()(const NameToken& token) const
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

            boost::apply_visitor(*this, vv.value());
        }

        buf_ << "]";
    }

    void operator()(const DataTypeArgMap<DataTypeCtrArg>& map) const
    {
        buf_ << "{";

        bool first = true;

        for (auto& entry: map.entries())
        {
            if (!first) {
                buf_ << ", ";
            }
            else {
                first = false;
            }

            buf_ << escape_quotes(entry.key()) << ": ";

            boost::apply_visitor(*this, entry.value().value());
        }

        buf_ << "}";
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

            boost::apply_visitor(arg_visitor, arg.value());
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





struct SDNValuePrintingVisitor: ArgPrintingVisitor
{
    using ArgPrintingVisitor::operator();

    SDNValuePrintingVisitor(SBuf& buf): ArgPrintingVisitor(buf)
    {}

    void operator()(const DataTypeDeclaration& decl)
    {
        buf_ << '@';
        decl.to_standard_string(buf_);
    }

    void operator()(const TypedSDNArray& array)
    {
        buf_ << "[";

        bool first = true;

        for (auto& vv: array.array())
        {
            if (!first) {
                buf_ << ", ";
            }
            else {
                first = false;
            }

            vv.to_string(buf_);
        }

        buf_ << "]";

        if (array.type().has_value())
        {
            buf_ << "@";
            array.type().get().to_standard_string(buf_);
        }
    }

    void operator()(const TypedSDNMap& map)
    {
        buf_ << "{";

        bool first = true;

        for (auto& vv: map.entries())
        {
            if (!first) {
                buf_ << ", ";
            }
            else {
                first = false;
            }

            buf_ << '\'' << escape_quotes(vv.key()) << '\'' << ": ";

            vv.value().to_string(buf_);
        }

        buf_ << "}";

        if (map.type().has_value())
        {
            buf_ << "@";
            map.type().get().to_standard_string(buf_);
        }
    }
};





void SDNValue::to_string(SBuf& buf) const
{
    SDNValuePrintingVisitor visitor(buf);
    boost::apply_visitor(visitor, value_);
}





struct SDNValuePrettyPrintingVisitor: ArgPrintingVisitor
{
    int32_t indent_;
    int32_t current_indent_;

    using ArgPrintingVisitor::operator();

    SDNValuePrettyPrintingVisitor(SBuf& buf, int32_t indent, int32_t current_indent):
        ArgPrintingVisitor(buf),
        indent_(indent),
        current_indent_(current_indent)
    {}

    void operator()(const DataTypeDeclaration& decl)
    {
        buf_ << '@';
        decl.to_standard_string(buf_);
    }

    void operator()(const TypedSDNArray& array)
    {
        buf_ << "[";

        if (indent_ > 0 && array.array().size() > 0) {
            buf_ << '\n';
        }

        bool first = true;

        for (auto& vv: array.array())
        {
            if (!first) {
                buf_ << ",\n";
            }
            else {
                first = false;
            }

            indent();

            vv.pretty_print(buf_, indent_, current_indent_ + indent_);
        }

        if (indent_ > 0 && array.array().size() > 0) {
            buf_ << '\n';
            indent(-indent_);
        }

        buf_ << "]";

        if (array.type().has_value())
        {
            buf_ << "@";
            array.type().get().to_standard_string(buf_);
        }
    }

    void operator()(const TypedSDNMap& map)
    {
        buf_ << "{";

        if (indent_ > 0 && map.entries().size() > 0) {
            buf_ << '\n';
        }

        bool first = true;

        for (auto& vv: map.entries())
        {
            if (!first) {
                buf_ << ",\n";
            }
            else {
                first = false;
            }

            indent();

            buf_ << '\'' << escape_quotes(vv.key()) << '\'' << ": ";

            vv.value().pretty_print(buf_, indent_, current_indent_ + indent_);
        }

        if (indent_ > 0 && map.entries().size() > 0)
        {
            buf_ << '\n';
            indent(-indent_);
        }

        buf_ << "}";

        if (map.type().has_value())
        {
            buf_ << "@";
            map.type().get().to_standard_string(buf_);
        }
    }

    void indent(int32_t offset = 0) const {
        for (int32_t c = 0; c < current_indent_ + offset; c++) {
            buf_ << ' ';
        }
    }
};


void SDNValue::pretty_print(SBuf& buf, int32_t indent, int32_t initial_indent) const
{
    SDNValuePrettyPrintingVisitor visitor(buf, indent, initial_indent > 0 ? initial_indent : indent);
    boost::apply_visitor(visitor, value_);
}





class ResolveMapsVisitor: public boost::static_visitor<>
{
protected:

public:
    ResolveMapsVisitor()
    {}

    void operator()(long i) const {}
    void operator()(double i) const {}
    void operator()(const TypedStringValue& str) const {}
    void operator()(const NameToken& token) const {}

    void operator()(const DataTypeDeclaration& decl) const {}

    void operator()(TypedSDNArray& array) const
    {
        for (auto& element: array.array()) {
            boost::apply_visitor(*this, element.value());
        }
    }

    void operator()(TypedSDNMap& map) const
    {
        map.build_index();
    }
};

void TypedSDNMap::build_index()
{
    map_.clear();
    for (auto& entry: entries_)
    {
        map_[entry.key()] = &entry.value();
        entry.value().resolve_maps();
    }
}

void SDNValue::resolve_maps() {
    boost::apply_visitor(ResolveMapsVisitor(), this->value_);
}

void SDNDocument::resolve_maps()
{
    value_.resolve_maps();
}

void SDNTypeDictionary::pretty_print(SBuf& buf, int32_t indent, int32_t initial_indent) const
{
    if (entries_.size() > 0)
    {
        for (int c = 0; c < initial_indent; c++) buf << ' ';
        buf << "#{\n";

        for (auto& entry: entries_)
        {
            for (int c = 0; c < initial_indent + indent; c++) buf << ' ';

            buf << entry.type_id() << ": ";
            entry.type().to_standard_string(buf);
            buf << ",\n";
        }

        for (int c = 0; c < initial_indent; c++) buf << ' ';
        buf << "}";
    }
    else {
        for (int c = 0; c < initial_indent; c++) buf << ' ';
        buf << "#{}";
    }
}


void SDNTypeDictionary::to_string(SBuf& buf) const
{
    buf << "#{";

    for (auto& entry: entries_)
    {
        buf << entry.type_id() << ": ";
        entry.type().to_standard_string(buf);
        buf << ",";
    }

    buf << "}";
}

void SDNDocument::pretty_print(SBuf &buf, int32_t indent, int32_t initial_indent) const
{
    if (type_dictionary_.has_value())
    {
        type_dictionary_.get().pretty_print(buf, indent, initial_indent);
        buf << "\n";
    }

    value_.pretty_print(buf, indent, initial_indent);
}


void SDNDocument::to_string(SBuf &buf) const
{
    if (type_dictionary_.has_value())
    {
        type_dictionary_.get().to_string(buf);
    }

    value_.to_string(buf);
}

}}