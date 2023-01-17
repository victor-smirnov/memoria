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
class TypedDataObject {
    T value_;
public:
    TypedDataObject(): value_() {}
    TypedDataObject(T value): value_(value) {}

    operator T() const {
        return value_;
    }

    Object finish() const
    {
        return current_ctr().make_t<DT>(value_);
    }
};

struct HermesIdentifier {
    std::string identifier;
};


class ArrayValue {
    ObjectArray array_;

public:
    using value_type = Object;
    using iterator = EmptyType;

    ArrayValue() {
        array_ = current_ctr().make_object_array();
    }

    iterator end() {return iterator{};}

    void insert(iterator, value_type value) {
        array_.push_back(value);
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

    operator Object() const {
        return finish();
    }
};

struct NumericTypeParamValue: boost::fusion::vector2<Optional<QualNameValue>, Object> {
    Object finish()
    {
        //auto& cast = boost::fusion::at_c<0>(*this);
        //auto& value  = boost::fusion::at_c<1>(*this);
        return boost::fusion::at_c<1>(*this);
    }
};


using MapEntryTuple = boost::phoenix::vector2<std::string, Object>;

class MapValue {
    ObjectMap value_;
public:
    using value_type = MapEntryTuple;
    using iterator = EmptyType;

    MapValue() {
        value_ = current_ctr().make_object_map();
    }

    iterator end() const {return iterator{};}

    void insert(iterator, const value_type& entry) {
        value_.put(boost::fusion::at_c<0>(entry), boost::fusion::at_c<1>(entry));
    }

    ObjectMap finish() {
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

    Object datatype_name_;
    std::vector<Object> params_;
    std::vector<Object> ctr_args_;
    std::vector<PtrSpecifier> ptr_specs_;
    bool is_const_{false};
    bool is_volatile_{false};
    int32_t refs_{};

public:
    void set_datatype_name(Object ii) {
        datatype_name_ = ii;
    }

    void add_datatype_parameter(Object value) {
        params_.push_back(value);
    }

    void add_constructor_arg(Object value) {
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

    Datatype finish() const
    {
        HermesCtrBuilder& builder = HermesCtrBuilder::current();
        Datatype decl = builder.make_datatype(datatype_name_);

        for (auto& td: params_) {
            builder.add_type_decl_param(decl, td);
        }

        for (auto& ctr_arg: ctr_args_) {
            builder.add_type_decl_ctr_arg(decl, ctr_arg);
        }

        for (auto& ptr_spec: ptr_specs_) {
            decl.add_ptr_spec(PtrQualifier(ptr_spec.is_const(), ptr_spec.is_volatile()));
        }

        decl.set_const(is_const_);
        decl.set_volatile(is_volatile_);

        decl.set_refs(refs_);

        return decl;
    }

    operator Datatype() const {
        return finish();
    }
};

static inline std::ostream& operator<<(std::ostream& out, const TypeDeclarationValue&) {
    return out << "TypeDeclarationValue";
}

struct TypeReference {
    Object id{};

    void operator=(const Object& id) {
        this->id = id;
    }
};

struct ValueVisitor: boost::static_visitor<> {

    Object value;

    void operator()(long long v) {
        value = HermesCtrView::wrap_dataobject<BigInt>(v).as_object();
    }

    void operator()(double v) {
        value = HermesCtrView::wrap_dataobject<Double>(v).as_object();
    }

    void operator()(bool v) {
        value = HermesCtrView::wrap_dataobject<Boolean>(v).as_object();
    }

    void operator()(ObjectMap&) {}

    void operator()(Datatype& v) {
        value = v.as_object();
    }

    void operator()(Object& v) {
        value = v;
    }

    template <typename V>
    void operator()(V& v) {
        value = v.finish().as_object();
    }
};

struct TypedValueValue: boost::fusion::vector2<Datatype, Object> {
    Object finish()
    {
        auto& type = boost::fusion::at_c<0>(*this);
        auto& ctr  = boost::fusion::at_c<1>(*this);

        auto ctr_hash = type.cxx_type_hash();
        if (has_type_reflection(ctr_hash))
        {
            if (ctr.is_varchar()) {
                return get_type_reflection(ctr_hash).datatype_convert_from_plain_string(ctr.as_varchar());
            }
            else {
                // FIXME: implement object construction from generic
                // Hermes value
                return HermesCtrBuilder::current().new_typed_value(
                        type,
                        ctr
                ).as_object();
            }
        }
        else {
            return HermesCtrBuilder::current().new_typed_value(
                    type,
                    ctr
            ).as_object();
        }
    }
};

struct StringOrTypedValue: boost::fusion::vector2<std::string, Optional<Datatype>> {
    Object finish()
    {
        const auto& str  = bf::at_c<0>(*this);
        auto h_str = HermesCtrBuilder::current().new_varchar(str);
        const auto& type = bf::at_c<1>(*this);

        if (MMA_LIKELY(!type)) {
            return h_str.as_object();
        }

        auto ctr_hash = type.get().cxx_type_hash();
        if (has_type_reflection(ctr_hash))
        {
            return get_type_reflection(ctr_hash).datatype_convert_from_plain_string(str);
        }
        else {
            return HermesCtrBuilder::current().new_typed_value(
                    type.get(),
                    HermesCtrView::wrap_dataobject<Varchar>(str).as_object()
            ).as_object();
        }

        return Object{};
    }
};


using TypedMapEntry = boost::phoenix::vector2<Object, Object>;

class TypedContainerValue {
    GenericArrayPtr array_;
    GenericMapPtr   map_;

    std::vector<Object> type_params_;
public:

    enum TypedContainerType {
        HERMES_TC_ARRAY, HERMES_TC_MAP
    };

    void add_type(const Object& type){
        type_params_.push_back(type);
    }

    void create_ctr(TypedContainerType ctr_type)
    {       
        if (ctr_type == HERMES_TC_ARRAY)
        {
            U8String class_txt = "memoria::Own<memoria::hermes::ArrayView<";
            class_txt += type_params_[0].as_datatype().to_cxx_string();
            class_txt += ">, 3>";

            array_ = get_type_reflection(get_cxx_type_hash(class_txt)).hermes_make_container(
                &HermesCtrBuilder::current().doc()
            )->as_array();
        }
        else {
            U8String class_txt = "memoria::Own<memoria::hermes::MapView<";

            class_txt += type_params_[0].as_datatype().to_cxx_string();
            class_txt += ", memoria::Own<memoria::hermes::ObjectView, 3>>, 3>";

            map_ = get_type_reflection(get_cxx_type_hash(class_txt)).hermes_make_container(
                &HermesCtrBuilder::current().doc()
            )->as_map();
        }
    }

    void push_back(const Object& value) {
        array_->push_back(value);
    }

    void push_back(const TypedMapEntry& value) {
        map_->put(bf::at_c<0>(value), bf::at_c<1>(value));
    }

    Object finish()
    {
        if (!array_.is_null()) {
            return array_->as_object();
        }
        else if (!map_.is_null()) {
            return map_->as_object();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Hermes Typed container is null").do_throw();
        }
    }
};



using TypeDirectoryMapEntry = boost::fusion::vector2<std::string, Datatype>;

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

        numeric_type_parameter = -(lit('(') >> datatype_name >> ")") >> integer_value;

        type_parameter = (type_declaration | bool_value | numeric_type_parameter | type_reference) [finish_value];

        type_declaration_dto =  datatype_name[add_datatype]
                            > -('<' >> (type_parameter[add_type_parameter] % ',') > '>')
                            >> -(('(' >> -(hermes_value[add_constructor_arg] % ',') > ')') |
                                 (*ref_ptr_const_qual[add_ptr_specifier] >>
                                 -((-lit("const")[add_const_to_typedecl] >> -lit("volatile")[add_volatile_to_typedecl]) >> lit("&")[add_refs] >> -lit("&")[add_refs])) |
                                 (lit("const")[add_const_to_typedecl] >> -lit("volatile")[add_volatile_to_typedecl]) |
                                 lit("volatile")[add_volatile_to_typedecl]
                                 )
                            ;

        auto typedecl_to_datatype_fn = [](auto& attrib, auto& ctx){
            bf::at_c<0>(ctx.attributes) = attrib.finish();
        };
        type_declaration = type_declaration_dto[typedecl_to_datatype_fn];

        auto resolve_typeref_fn = [](auto& attrib, auto& ctx){
            Datatype datatype = HermesCtrBuilder::current().resolve_typeref(attrib);
            bf::at_c<0>(ctx.attributes) = datatype;
        };
        type_reference   = ('#' > hermes_identifier) [resolve_typeref_fn];

        type_decl_or_reference =  type_declaration | type_reference;

        typed_value      = '@' > type_decl_or_reference > "=" > hermes_value;

        hermes_string_or_typed_value = hermes_string >> -('@' > type_decl_or_reference);


        null_value   = lexeme[lit("null")][dummy];

        array        = ('[' >> (hermes_value % ',') > ']') |
                                            (lit('[') >> ']');

        auto add_tc_type_fn = [](auto& attrib, auto& ctx){
            bf::at_c<0>(ctx.attributes).add_type(attrib);
        };

        auto create_typed_array_ctr_fn = [](auto&, auto& ctx){
            bf::at_c<0>(ctx.attributes).create_ctr(TypedContainerValue::HERMES_TC_ARRAY);
        };

        auto create_typed_map_ctr_fn = [](auto&, auto& ctx){
            bf::at_c<0>(ctx.attributes).create_ctr(TypedContainerValue::HERMES_TC_MAP);
        };

        auto add_array_element_fn = [](auto& attrib, auto& ctx){
            bf::at_c<0>(ctx.attributes).push_back(attrib);
        };

        auto add_map_element_fn = [](auto& attrib, auto& ctx){
            bf::at_c<0>(ctx.attributes).push_back(attrib);
        };

        typed_ctr = '<' >> (type_parameter[add_tc_type_fn] % ',') >> '>' >>  ((
                            lit('[') [create_typed_array_ctr_fn] >> (hermes_value [add_array_element_fn] % ',') >> ']' |
                            lit('[') [create_typed_array_ctr_fn] >> ']'
                        ) | (lit('{') [create_typed_map_ctr_fn] >> (typed_map_entry [add_map_element_fn] % ',') >> '}' |
                             lit('{') [create_typed_map_ctr_fn] >> '}'
                             ));

        map_entry    = ((hermes_string | hermes_identifier) > ':' > hermes_value);
        typed_map_entry = hermes_value > ':' > hermes_value;

        map          = ('{' >> (map_entry % ',') > '}') |
                                            (lit('{') >> '}');

        bool_value_true      = lit("true") [set_bool_true];
        bool_value_false     = lit("false") [set_bool_false];
        bool_value           = bool_value_true | bool_value_false;

        uint64_parser = ("0x" >> hex_uint64_parser |
                         "0b" >> bin_uint64_parser|
                         &lit('0') >> oct_uint64_parser |
                         dec_uint64_parser
                        ) >> (lit("ull") | lit("ul") | "_u64");

        uint32_parser = ("0x" >> hex_uint32_parser |
                         "0b" >> bin_uint32_parser|
                         &lit('0') >> oct_uint32_parser |
                         dec_uint32_parser
                        ) >> (lit("u") | "_u32");

        uint16_parser = ("0x" >> hex_uint16_parser |
                         "0b" >> bin_uint16_parser|
                         &lit('0') >> oct_uint16_parser |
                         dec_uint16_parser
                        ) >> "_u16";

        uint8_parser  = ("0x" >> hex_uint8_parser |
                         "0b" >> bin_uint8_parser|
                         &lit('0') >> oct_uint8_parser |
                         dec_uint8_parser
                        ) >> "_u8";

        int64_parser  = dec_int64_parser >> (lit("ll") | "_s64");
        int32_parser  = dec_int32_parser >> -lit("_s32");
        int16_parser  = dec_int16_parser >> "_s16";
        int8_parser   = dec_int8_parser >> "_s8";

        integer_value = (
                            uint64_parser |
                            int64_parser |
                            uint32_parser |
                            uint16_parser |
                            int16_parser |
                            uint8_parser |
                            int8_parser |
                            int32_parser
                    ) [finish_value];

        f64_parser = strict_f64 >> "d";
        f32_parser = strict_f32 >> -lit("f");

        fp_value = (
                    f64_parser |
                    f32_parser
                    ) [finish_value];

        static auto finish_parameter = [](auto& attrib, auto& ctx) {
                    bf::at_c<0>(ctx.attributes) = HermesCtrBuilder::current().new_parameter(attrib.identifier).as_object();
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
                                        | fp_value
                                        | integer_value
                                        | map
                                        | null_value
                                        | array
                                        | type_declaration
                                        | typed_value
                                        | bool_value
                                        | parameter
                                        | typed_ctr
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

    qi::real_parser<double, qi::strict_real_policies<double>> strict_f64;
    qi::real_parser<float, qi::strict_real_policies<float>>   strict_f32;

    qi::uint_parser<uint64_t, 16> hex_uint64_parser;
    qi::uint_parser<uint64_t, 2>  bin_uint64_parser;
    qi::uint_parser<uint64_t, 8>  oct_uint64_parser;
    qi::uint_parser<uint64_t, 10> dec_uint64_parser;

    qi::uint_parser<uint32_t, 16> hex_uint32_parser;
    qi::uint_parser<uint32_t, 2>  bin_uint32_parser;
    qi::uint_parser<uint32_t, 8>  oct_uint32_parser;
    qi::uint_parser<uint32_t, 10> dec_uint32_parser;

    qi::uint_parser<uint16_t, 16> hex_uint16_parser;
    qi::uint_parser<uint16_t, 2>  bin_uint16_parser;
    qi::uint_parser<uint16_t, 8>  oct_uint16_parser;
    qi::uint_parser<uint16_t, 10> dec_uint16_parser;

    qi::uint_parser<uint8_t, 16> hex_uint8_parser;
    qi::uint_parser<uint8_t, 2>  bin_uint8_parser;
    qi::uint_parser<uint8_t, 8>  oct_uint8_parser;
    qi::uint_parser<uint8_t, 10> dec_uint8_parser;

    qi::int_parser<int64_t, 10>  dec_int64_parser;
    qi::int_parser<int32_t, 10>  dec_int32_parser;
    qi::int_parser<int16_t, 10>  dec_int16_parser;
    qi::int_parser<int8_t, 10>   dec_int8_parser;


    qi::rule<Iterator, TypedDataObject<uint64_t, UBigInt>(), Skipper>   uint64_parser;
    qi::rule<Iterator, TypedDataObject<uint32_t, UInteger>(), Skipper>  uint32_parser;
    qi::rule<Iterator, TypedDataObject<uint16_t, USmallInt>(), Skipper> uint16_parser;
    qi::rule<Iterator, TypedDataObject<uint8_t,  UTinyInt>(), Skipper>  uint8_parser;


    qi::rule<Iterator, TypedDataObject<int64_t, BigInt>(), Skipper> int64_parser;
    qi::rule<Iterator, TypedDataObject<int32_t, memoria::Integer>(), Skipper> int32_parser;
    qi::rule<Iterator, TypedDataObject<int16_t, memoria::SmallInt>(), Skipper> int16_parser;
    qi::rule<Iterator, TypedDataObject<int8_t, memoria::TinyInt>(), Skipper> int8_parser;

    qi::rule<Iterator, TypedDataObject<float,  memoria::Real>(), Skipper> f32_parser;
    qi::rule<Iterator, TypedDataObject<double, memoria::Double>(), Skipper> f64_parser;

    qi::rule<Iterator, Object, Skipper>  integer_value;
    qi::rule<Iterator, Object, Skipper>  fp_value;


    qi::rule<Iterator, Object(), Skipper>            hermes_value;
    qi::rule<Iterator, Object(), Skipper>            standalone_hermes_value;

    qi::rule<Iterator, ArrayValue(), Skipper>           array;
    qi::rule<Iterator, MapValue(), Skipper>             map;

    qi::rule<Iterator, MapEntryTuple(), Skipper>        map_entry;
    qi::rule<Iterator, TypedMapEntry(), Skipper>        typed_map_entry;

    qi::rule<Iterator, Object(), Skipper>            null_value;
    qi::rule<Iterator, std::string(), Skipper>          hermes_string;
    qi::rule<Iterator, std::string(), Skipper>          hermes_identifier;

    qi::rule<Iterator, StringOrTypedValue(), Skipper>   hermes_string_or_typed_value;

    qi::rule<Iterator, const char*(), Skipper>          cxx_basic_type;
    qi::rule<Iterator, QualNameValue(), Skipper>        datatype_name;

    qi::rule<Iterator, PtrSpecifier(), Skipper>         ref_ptr_const_qual;

    qi::rule<Iterator, TypeDeclarationValue(), Skipper> type_declaration_dto;
    qi::rule<Iterator, Datatype(), Skipper>          type_declaration;

    qi::rule<Iterator, NumericTypeParamValue(), Skipper>  numeric_type_parameter;

    qi::rule<Iterator, Object(), Skipper>            type_parameter;
    qi::rule<Iterator, Datatype(), Skipper>          standalone_type_decl;

    qi::rule<Iterator, Object(), Skipper>            parameter;

    qi::rule<Iterator, TypedValueValue(), Skipper>      typed_value;
    qi::rule<Iterator, TypedContainerValue(), Skipper>  typed_ctr;

    qi::rule<Iterator, Datatype(), Skipper>          type_reference;
    qi::rule<Iterator, Datatype(), Skipper>          type_decl_or_reference;

    qi::rule<Iterator, bool(), Skipper> bool_value_true;
    qi::rule<Iterator, bool(), Skipper> bool_value_false;
    qi::rule<Iterator, bool(), Skipper> bool_value;
};

}

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::HermesIdentifier,
    (std::string, identifier)
)
