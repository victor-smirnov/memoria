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

#pragma once

#define BOOST_SPIRIT_UNICODE

#include <memoria/core/hermes/hermes.hpp>

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

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/adapted/std_pair.hpp>


#include <boost/config/warning_disable.hpp>
#include <boost/phoenix/bind.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/optional/optional_io.hpp>


#include "path/parser/appendutf8action.h"
#include "path/parser/encodesurrogatepairaction.h"
#include "path/parser/appendescapesequenceaction.h"

#include "path/ast/rawstringnode.h"
#include "path/ast/identifiernode.h"

#include "hermes_grammar_strings.hpp"
#include "hermes_ctr_builder.hpp"




namespace memoria::hermes {

namespace qi = boost::spirit::qi;
namespace bf = boost::fusion;
namespace bp = boost::phoenix;




template <typename T, typename DT>
class Integer {
    T value_;
public:
    Integer(): value_() {}
    Integer(T value): value_(value) {}

    operator T() const {
        return value_;
    }

    ValuePtr finish() const {
        return HermesCtrBuilder::current().template new_dataobject<DT>(value_)->as_value();
    }
};

struct HermesIdentifier {
    std::string identifier;
};


class ArrayValue {
    GenericArrayPtr array_;

public:
    using value_type = ViewPtr<Value>;
    using iterator = EmptyType;

    ArrayValue() {
        array_ = HermesCtrBuilder::current().new_array();
    }

    iterator end() {return iterator{};}

    void insert(iterator, value_type value) {
        HermesCtrBuilder::current().append_value(array_, value);
    }

    auto finish() {
        return std::move(array_);
    }
};


class QualNameValue {
    bool first_{true};
    U8String buffer_;

    const char* type_name_{};

public:
    QualNameValue() {}
    QualNameValue(const char* str): type_name_(str) {}

    using value_type = std::string;
    using iterator   = EmptyType;

    iterator end() {return iterator{};}

    void insert (iterator, const value_type& value)
    {
        if (!first_) {
            buffer_.to_std_string().push_back(':');
            buffer_.to_std_string().push_back(':');
        }
        else {
            first_ = false;
        }

        for (auto ch: value) {
            buffer_.to_std_string().push_back(ch);
        }
    }


    auto finish() const {
        if (type_name_) {
            return HermesCtrBuilder::current().new_varchar(type_name_);
        }
        return HermesCtrBuilder::current().new_varchar(buffer_);
    }

    operator StringValuePtr() const {
        return finish();
    }
};


using MapEntryTuple = boost::phoenix::vector2<std::string, ValuePtr>;

class MapValue {
    GenericMapPtr value_;
public:
    using value_type = MapEntryTuple;
    using iterator = EmptyType;

    MapValue() {
        value_ = HermesCtrBuilder::current().new_map();
    }

    iterator end() const {return iterator{};}

    void insert(iterator, const value_type& entry) {
        HermesCtrBuilder::current().append_entry(value_, boost::fusion::at_c<0>(entry), boost::fusion::at_c<1>(entry));
    }

    GenericMapPtr finish() {
        return std::move(value_);
    }
};

class PtrSpecifier {
    uint8_t is_const_ : 1;
    uint8_t is_volatile_ : 1;
public:
    PtrSpecifier():
        is_const_(), is_volatile_()
    {}

    bool is_volatile() const {return is_volatile_;}
    bool is_const() const {return is_const_;}

    void set_const(bool v) {
        is_const_ = v;
    }
    void set_volatile(bool v) {
        is_volatile_ = v;
    }
};

class TypeDeclarationValue {

    StringValuePtr datatype_name_;
    std::vector<ValuePtr> params_;
    std::vector<ValuePtr> ctr_args_;
    std::vector<PtrSpecifier> ptr_specs_;
    bool is_const_{false};
    bool is_volatile_{false};
    int32_t refs_{};

public:
    void set_datatype_name(StringValuePtr ii) {
        datatype_name_ = ii;
    }

    void add_datatype_parameter(ValuePtr value) {
        params_.push_back(value);
    }

    void add_constructor_arg(ValuePtr value) {
        ctr_args_.push_back(value);
    }

    void add_ptr_specifier(PtrSpecifier value) {
        ptr_specs_.push_back(value);
    }

    void set_const(bool v) {
        is_const_ = v;
    }

    void set_volatile(bool v) {
        is_volatile_ = v;
    }

    void add_refs() {
        refs_++;
    }

    DatatypePtr finish() const
    {
        HermesCtrBuilder& builder = HermesCtrBuilder::current();
        DatatypePtr decl = builder.new_datatype(datatype_name_);

        for (auto& td: params_) {
            builder.add_type_decl_param(decl, td);
        }

        for (auto& ctr_arg: ctr_args_) {
            builder.add_type_decl_ctr_arg(decl, ctr_arg);
        }

        for (auto& ptr_spec: ptr_specs_) {
            decl->add_ptr_spec(PtrQualifier(ptr_spec.is_const(), ptr_spec.is_volatile()));
        }

        decl->set_const(is_const_);
        decl->set_volatile(is_volatile_);

        decl->set_refs(refs_);

        return decl;
    }

    operator DatatypePtr() const {
        return finish();
    }
};

static inline std::ostream& operator<<(std::ostream& out, const TypeDeclarationValue&) {
    return out << "TypeDeclarationValue";
}

struct TypeReference {
    StringValuePtr id{};

    void operator=(StringValuePtr id) {
        this->id = id;
    }
};

using TypeDeclOrReference = boost::variant<std::string, TypeDeclarationValue>;


struct ValueVisitor: boost::static_visitor<> {

    ValuePtr value;

    void operator()(long long v) {
        value = HermesCtrBuilder::current().new_bigint(v)->as_value();
    }

    void operator()(double v) {
        value = HermesCtrBuilder::current().new_double(v)->as_value();
    }

    void operator()(bool v) {
        value = HermesCtrBuilder::current().new_boolean(v)->as_value();
    }

    void operator()(GenericMapPtr&) {}

    void operator()(StringValuePtr& v) {
        value = v->as_value();
    }

    void operator()(ValuePtr& v) {
        value = v;
    }

    template <typename V>
    void operator()(V& v) {
        value = v.finish()->as_value();
    }
};

using TypedValueValueBase = boost::fusion::vector2<TypeDeclOrReference, ValuePtr>;


struct TypedValueValue: TypedValueValueBase {

    struct Visitor: public boost::static_visitor<> {

        ValuePtr ctr_value_;
        ValuePtr typed_value;

        Visitor(ValuePtr ctr_value): ctr_value_(ctr_value) {}

        void operator()(const std::string& ref)
        {
            DatatypePtr datatype = HermesCtrBuilder::current().resolve_typeref(ref);
            typed_value = HermesCtrBuilder::current().new_typed_value(
                        datatype,
                        ctr_value_
            )->as_value();
        }

        void operator()(DatatypePtr type_decl){
            typed_value = HermesCtrBuilder::current().new_typed_value(
                        type_decl,
                        ctr_value_
            )->as_value();
        }
    };

    ValuePtr finish()
    {
        Visitor vv(boost::fusion::at_c<1>(*this));
        boost::apply_visitor(vv, bf::at_c<0>(*this));
        return vv.typed_value;
    }
};


using StringOrTypedValueBase = boost::fusion::vector2<std::string, Optional<TypeDeclOrReference>>;

struct StringOrTypedValue: StringOrTypedValueBase {
    ValuePtr finish()
    {
        const auto& str  = bf::at_c<0>(*this);
        auto h_str = HermesCtrBuilder::current().new_varchar(str);
        const auto& type = bf::at_c<1>(*this);

        if (MMA_LIKELY(!type)) {

            return h_str->as_value();
        }

        TypedValueValue typed_value;

        bf::at_c<0>(typed_value) = type.get();
        bf::at_c<1>(typed_value) = h_str->as_value();

        return typed_value.finish();
    }
};

using TypeDirectoryMapEntry = boost::fusion::vector2<std::string, DatatypePtr>;


struct TypeDirectoryValue {
    using value_type = TypeDirectoryMapEntry;
    using iterator = EmptyType;
public:

    iterator end() {return iterator{};}

    void insert(iterator, const TypeDirectoryMapEntry& entry)
    {
        HermesCtrBuilder::current().add_type_directory_entry(
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

    void insert(iterator, value_type)
    {}

    void operator=(value_type) {}
};


template <typename Iterator, typename Skipper>
using ValueStringRuleSet = memoria::hermes::parser::StringsRuleSet<
    Iterator,
    Skipper,
    path::parser::AppendUtf8Action<>,
    path::parser::AppendEscapeSequenceAction<>,
    path::parser::EncodeSurrogatePairAction,
    std::string,
    path::ast::RawStringNode,
    HermesIdentifier
>;



template <typename Iterator, typename Skipper>
struct HermesValueRulesLib: ValueStringRuleSet<Iterator, Skipper> {

    using StringLib = ValueStringRuleSet<Iterator, Skipper>;

    using StringLib::m_identifierRule;
    using StringLib::m_rawStringRule;
    using StringLib::m_quotedStringRule;

    HermesValueRulesLib()
    {
        namespace enc = qi::unicode;
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

        auto dummy = [](const auto&, auto&){};

        static auto set_bool_true = [](const auto&, auto& ctx) {
            bf::at_c<0>(ctx.attributes) = true;
        };

        static auto set_bool_false = [](const auto&, auto& ctx) {
            bf::at_c<0>(ctx.attributes) = false;
        };

        static auto finish_value = [](auto& attrib, auto& ctx){
            ValueVisitor visitor;
            boost::apply_visitor(visitor, attrib);
            bf::at_c<0>(ctx.attributes) = visitor.value;
        };

        static auto finish_raw_string = [](auto& attrib, auto& ctx) {
            bf::at_c<0>(ctx.attributes) = attrib.rawString;
        };

        static auto finish_quoted_string = [](auto& attrib, auto& ctx) {
            bf::at_c<0>(ctx.attributes) = attrib;
        };

        hermes_string = m_quotedStringRule [finish_quoted_string] | m_rawStringRule [finish_raw_string];

        static auto finish_identifier = [](auto& attrib, auto& ctx) {
            bf::at_c<0>(ctx.attributes) = attrib.identifier;
        };

        hermes_identifier = m_identifierRule [finish_identifier];

        cxx_basic_type =    (lit("long") >> "double") [_val = "long double"] |

                            (lit("unsigned") >> "long" >> "int") [_val = "unsigned long"] ||
                            (lit("unsigned") >> "long" >> "long") [_val = "unsigned long long"] ||
                            (lit("unsigned") >> "long") [_val = "unsigned long"] ||
                            (lit("unsigned") >> "int") [_val = "unsigned int"] ||
                            (lit("unsigned") >> "short" >> "int") [_val = "unsigned short"] ||
                            (lit("unsigned") >> "short") [_val = "unsigned short"] ||
                            (lit("unsigned") >> "char") [_val = "unsigned char"] ||
                            (lit("unsigned")) [_val = "unsigned int"] ||

                            (lit("signed") >> "long" >> "long") [_val = "long long"] ||
                            (lit("signed") >> "long" >> "int") [_val = "long"] ||
                            (lit("signed") >> "long") [_val = "long"] ||
                            (lit("signed") >> "int") [_val = "int"] ||
                            (lit("signed") >> "short int") [_val = "short"] ||
                            (lit("signed") >> "short") [_val = "short"] ||
                            (lit("signed") >> "char") [_val = "char"] ||
                            lit("signed") [_val = "int"] ||

                            lit("char") [_val = "char"] ||
                            (lit("short") >> "int") [_val = "short"] ||
                            lit("short") [_val = "short"] ||
                            lit("int") [_val = "int"] ||
                            (lit("long") >> "int" )[_val = "long"] ||
                            (lit("long") >> "long" )[_val = "long long"] ||
                            lit("long")[_val = "long"] ||

                            lit("double") [_val = "double"] ||
                            lit("float") [_val = "float"] ||
                            lit("bool") [_val = "bool"]
                         ;

        datatype_name = -(lit("struct") | "class" | "union") >> (hermes_identifier % "::") | cxx_basic_type;

        auto set_const_to_qual = [](auto&, auto& ctx){
            bf::at_c<0>(ctx.attributes).set_const(true);
        };

        auto set_volatile_to_qual = [](auto&, auto& ctx){
            bf::at_c<0>(ctx.attributes).set_volatile(true);
        };

        ref_ptr_const_qual = -lit("const")[set_const_to_qual]
                    >> -lit("volatile")[set_volatile_to_qual]
                    >>  lit('*');


        auto add_const_to_typedecl = [](auto&, auto& ctx){
            bf::at_c<0>(ctx.attributes).set_const(true);
        };

        auto add_volatile_to_typedecl = [](auto&, auto& ctx){
            bf::at_c<0>(ctx.attributes).set_volatile(true);
        };

        auto add_datatype = [](auto& attr, auto& ctx){
            bf::at_c<0>(ctx.attributes).set_datatype_name(attr);
        };

        auto add_type_parameter = [](auto& attr, auto& ctx){
            bf::at_c<0>(ctx.attributes).add_datatype_parameter(attr);
        };

        auto add_constructor_arg = [](auto& attr, auto& ctx){
            bf::at_c<0>(ctx.attributes).add_constructor_arg(attr);
        };

        auto add_ptr_specifier = [](auto& attr, auto& ctx){
            bf::at_c<0>(ctx.attributes).add_ptr_specifier(attr);
        };

        auto add_refs = [](auto&, auto& ctx){
            bf::at_c<0>(ctx.attributes).add_refs();
        };

        type_parameter   = (type_declaration | bool_value | integer_value) [finish_value];

        type_declaration =  datatype_name[add_datatype]
                            > -('<' >> (type_parameter[add_type_parameter] % ',') > '>')
                            >> -(('(' >> -(hermes_value[add_constructor_arg] % ',') > ')') |
                                 (*ref_ptr_const_qual[add_ptr_specifier] >>
                                 -((-lit("const")[add_const_to_typedecl] >> -lit("volatile")[add_volatile_to_typedecl]) >> lit("&")[add_refs] >> -lit("&")[add_refs])) |
                                 (lit("const")[add_const_to_typedecl] >> -lit("volatile")[add_volatile_to_typedecl]) |
                                 lit("volatile")[add_volatile_to_typedecl]
                                 )
                            ;

        type_reference   = '#' > hermes_identifier;

        auto check_typeref = [](const auto& typeref) {
            return HermesCtrBuilder::current().check_typeref(typeref);
        };

        type_decl_or_reference =  type_declaration[_val = _1] | (type_reference[_val = _1, _pass = boost::phoenix::bind(check_typeref,_1)]);

        typed_value      = '@' > type_decl_or_reference > "=" > hermes_value;

        hermes_string_or_typed_value = hermes_string >> -('@' > type_decl_or_reference);


        null_value   = lexeme[lit("null")][dummy];

        array        = ('[' >> (hermes_value % ',') > ']') |
                                            (lit('[') >> ']');

        map_entry    = ((hermes_string | hermes_identifier) > ':' > hermes_value);

        map          = ('{' >> (map_entry % ',') > '}') |
                                            (lit('{') >> '}');

        bool_value_true      = lit("true") [set_bool_true];
        bool_value_false     = lit("false") [set_bool_false];
        bool_value           = bool_value_true | bool_value_false;

        uint64_parser = ("0x" >> hex_uint64_parser |
                         "0b" >> bin_uint64_parser|
                         &lit('0') >> oct_uint64_parser |
                         dec_uint64_parser
                        ) >> "ull";

        int64_parser  = dec_int64_parser;

        integer_value = (uint64_parser | int64_parser) [finish_value];

        static auto finish_parameter = [](auto& attrib, auto& ctx) {
                    bf::at_c<0>(ctx.attributes) = HermesCtrBuilder::current().new_parameter(attrib.identifier)->as_value();
        };
        parameter = lit('?') >> m_identifierRule[finish_parameter];

        auto enter_builder = [](const auto& attrib, const auto& ctx){
            HermesCtrBuilder::enter();
        };

        auto exit_builder = [](const auto& attrib, const auto& ctx){
            HermesCtrBuilder::current().exit();
        };

        hermes_value =  qi::eps[enter_builder] >> (
                                        hermes_string_or_typed_value
                                        | strict_double
                                        | integer_value
                                        | map
                                        | null_value
                                        | array
                                        | type_declaration
                                        | typed_value
                                        | bool_value
                                        | parameter
                                      )[finish_value] >> qi::eps[exit_builder];

        standalone_type_decl = type_declaration;
        standalone_hermes_value = hermes_value;

        BOOST_SPIRIT_DEBUG_NODE(type_declaration);
        BOOST_SPIRIT_DEBUG_NODE(cxx_basic_type);

        hermes_value.name("hermes_value");
        standalone_hermes_value.name("standalone_hermes_value");

        array.name("array");
        map.name("map");
        map_entry.name("map_entry");
        null_value.name("null_value");
        hermes_string.name("hermes_string");
        hermes_string_or_typed_value.name("hermes_string_or_typed_value");
        hermes_identifier.name("hermes_identifier");
        cxx_basic_type.name("cxx_basic_type");
        type_declaration.name("type_declaration");

        standalone_type_decl.name("standalone_type_decl");
        typed_value.name("typed_value");
        type_reference.name("type_reference");
        type_decl_or_reference.name("type_decl_or_reference");
        bool_value_true.name("bool_value_true");
        bool_value_false.name("bool_value_false");
        bool_value.name("bool_value");
    }

    qi::real_parser<double, qi::strict_real_policies<double>> strict_double;

    qi::uint_parser<uint64_t, 16> hex_uint64_parser;
    qi::uint_parser<uint64_t, 2>  bin_uint64_parser;
    qi::uint_parser<uint64_t, 8>  oct_uint64_parser;
    qi::uint_parser<uint64_t, 10> dec_uint64_parser;

    qi::int_parser<int64_t, 10>  dec_int64_parser;

    qi::rule<Iterator, Integer<uint64_t, UBigInt>(), Skipper> uint64_parser;
    qi::rule<Iterator, Integer<int64_t, BigInt>(), Skipper>  int64_parser;
    qi::rule<Iterator, ValuePtr, Skipper>  integer_value;


    qi::rule<Iterator, ValuePtr(), Skipper>             hermes_value;
    qi::rule<Iterator, ValuePtr(), Skipper>             standalone_hermes_value;

    qi::rule<Iterator, ArrayValue(), Skipper>           array;
    qi::rule<Iterator, MapValue(), Skipper>             map;

    qi::rule<Iterator, MapEntryTuple(), Skipper>        map_entry;

    qi::rule<Iterator, ValuePtr(), Skipper>             null_value;
    qi::rule<Iterator, std::string(), Skipper>          hermes_string;
    qi::rule<Iterator, std::string(), Skipper>          hermes_identifier;

    qi::rule<Iterator, StringOrTypedValue(), Skipper>   hermes_string_or_typed_value;

    qi::rule<Iterator, const char*(), Skipper>          cxx_basic_type;
    qi::rule<Iterator, QualNameValue(), Skipper>        datatype_name;

    qi::rule<Iterator, PtrSpecifier(), Skipper>         ref_ptr_const_qual;

    qi::rule<Iterator, TypeDeclarationValue(), Skipper> type_declaration;
    qi::rule<Iterator, ValuePtr(), Skipper>             type_parameter;
    qi::rule<Iterator, DatatypePtr(), Skipper>          standalone_type_decl;

    qi::rule<Iterator, ValuePtr(), Skipper>             parameter;

    qi::rule<Iterator, TypedValueValue(), Skipper>      typed_value;

    qi::rule<Iterator, std::string(), Skipper>          type_reference;
    qi::rule<Iterator, TypeDeclOrReference(), Skipper>  type_decl_or_reference;

    qi::rule<Iterator, bool(), Skipper> bool_value_true;
    qi::rule<Iterator, bool(), Skipper> bool_value_false;
    qi::rule<Iterator, bool(), Skipper> bool_value;
};

}

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::HermesIdentifier,
    (std::string, identifier)
)
