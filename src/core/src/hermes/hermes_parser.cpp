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

//#define BOOST_SPIRIT_UNICODE

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

#include <boost/phoenix/bind.hpp>

#include <boost/variant/recursive_variant.hpp>

#include <boost/optional/optional_io.hpp>

#include <memoria/core/flat_map/flat_hash_map.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>

namespace memoria {
namespace hermes {

namespace qi = boost::spirit::qi;
namespace enc = qi::standard;
namespace bf = boost::fusion;
namespace bp = boost::phoenix;

class DocumentBuilder {

    ArenaBuffer<char> string_buffer_;
    HermesDocView& doc_;

    ska::flat_hash_map<U8String, DatatypePtr> type_registry_;
    std::unordered_map<U8StringView, StringValuePtr, arena::DefaultHashFn<U8StringView>> string_registry_;

public:

    DocumentBuilder(HermesDocView& doc):
        doc_(doc)
    {}


    StringValuePtr resolve_string(U8StringView strv)
    {
        auto ii = string_registry_.find(strv);
        if (ii != string_registry_.end()) {
            return ii->second;
        }
        return StringValuePtr{};
    }

    U8StringView to_string_view(Span<const char> span) {
        return U8StringView(span.data(), span.size());
    }

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
        U8StringView sv = to_string_view(span);
        StringValuePtr str = resolve_string(sv);

        if (str->is_null()) {
            str = doc_.new_dataobject<Varchar>(sv);
            string_registry_[str->view()] = str;
        }

        return str;
    }

    auto new_varchar(U8StringView sv)
    {
        StringValuePtr str = resolve_string(sv);

        if (str->is_null()) {
            str = doc_.new_dataobject<Varchar>(sv);
            string_registry_[str->view()] = str;
        }

        return str;
    }

    auto new_identifier()
    {
        return new_varchar();
    }

    auto new_bigint(int64_t v) {
        return doc_.new_dataobject<BigInt>(v);
    }

    auto new_double(double v) {
        return doc_.new_dataobject<Double>(v);
    }

    auto new_boolean(bool v) {
        return doc_.new_dataobject<BigInt>((int64_t)v);
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

    void add_type_directory_entry(StringValuePtr id, DatatypePtr datatype)
    {
        auto ii = type_registry_.find(id->view());
        if (ii == type_registry_.end()) {
            type_registry_[id->view()] = datatype;
        }
    }

    DatatypePtr resolve_typeref(U8StringView typeref)
    {
        auto ii = type_registry_.find(typeref);
        if (ii != type_registry_.end())
        {
            return ii->second;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("No datatype with reference '{}' in the registry", typeref).do_throw();
        }
    }

    bool check_typeref(U8StringView typeref)
    {
        auto ii = type_registry_.find(typeref);
        return (ii != type_registry_.end());
    }

    void append_entry(GenericMapPtr& map, const StringValuePtr& name, const ValuePtr& value) {
        map->put(name, value);
    }

    template <typename DT>
    DataObjectPtr<DT> new_dataobject(DTTViewType<DT> view) {
        return doc_.template new_dataobject<DT>(view);
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

namespace  {

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
        return DocumentBuilder::current()->template new_dataobject<DT>(value_)->as_value();
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
    void insert(iterator, II, II){}

    void operator=(value_type value) {
        builder_->append_char(value);
    }

    void operator=(StringValuePtr) {
        // FIXME this method should be removed
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


class QualNameValue {
    bool first_{true};
    U8String buffer_;

    const char* type_name_{};

public:
    QualNameValue() {}
    QualNameValue(const char* str): type_name_(str) {}

    using value_type = StringValuePtr;
    using iterator = EmptyType;

    iterator end() {return iterator{};}

    void insert (iterator, value_type value)
    {
        if (!first_) {
            buffer_.to_std_string().push_back(':');
            buffer_.to_std_string().push_back(':');
        }
        else {
            first_ = false;
        }

        for (auto ch: value->view()) {
            buffer_.to_std_string().push_back(ch);
        }
    }


    auto finish() const {
        if (type_name_) {
            return DocumentBuilder::current()->new_varchar(type_name_);
        }
        return DocumentBuilder::current()->new_varchar(buffer_);
    }

    operator StringValuePtr() const {
        return finish();
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
        DocumentBuilder* builder = DocumentBuilder::current();
        DatatypePtr decl = builder->new_datatype(datatype_name_);

        for (auto& td: params_) {
            builder->add_type_decl_param(decl, td);
        }

        for (auto& ctr_arg: ctr_args_) {
            builder->add_type_decl_ctr_arg(decl, ctr_arg);
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

std::ostream& operator<<(std::ostream& out, const TypeDeclarationValue&) {
    return out << "TypeDeclarationValue";
}

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

    void operator()(NullValue&) {}

    void operator()(GenericMapPtr&) {}

    void operator()(StringValuePtr& v) {
        value = v->as_value();
    }

    void operator()(ValuePtr& v){
        value = v;
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

        void operator()(StringValuePtr ref)
        {
            DatatypePtr datatype = DocumentBuilder::current()->resolve_typeref(ref->view());
            typed_value = DocumentBuilder::current()->new_typed_value(
                        datatype,
                        ctr_value_
            )->as_value();
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
        builder_->add_type_directory_entry(
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

    iterator end() const {return iterator();}

    void insert(iterator, value_type)
    {}

    void operator=(value_type) {}
};

template <typename Iterator>
using SkipperT = qi::rule<Iterator>;

template <typename Iterator>
struct HermesDocParser : qi::grammar<Iterator, pool::SharedPtr<HermesDoc>(), SkipperT<Iterator>>
{
    using Skipper = SkipperT<Iterator>;

    HermesDocParser() : HermesDocParser::base_type(hermes_document)
    {
        using qi::long_;
        using qi::lit;
        using qi::lexeme;
        using qi::standard::char_;
        using qi::_val;
        using qi::eol;
        using qi::eoi;
        using qi::_pass;
        using qi::fail;
        using qi::_1;

        using bp::construct;
        using bp::val;


        auto dummy = [](const auto&, auto&){};

        static auto finish_identifier = [](auto& attrib, auto& ctx) {
            bf::at_c<0>(ctx.attributes) = attrib.finish();
        };

        static auto finish_string = [](auto& attrib, auto& ctx) {
            bf::at_c<0>(ctx.attributes) = attrib.finish();
        };

        static auto set_bool_true = [](const auto&, auto& ctx) {
            bf::at_c<0>(ctx.attributes) = true;
        };

        static auto set_bool_false = [](const auto&, auto& ctx) {
            bf::at_c<0>(ctx.attributes) = false;
        };

        static auto clear_string_buffer = [](const auto&, const auto&){
            DocumentBuilder::current()->clear_string_buffer();
        };

        static auto finish_value = [](auto& attrib, auto& ctx){
            ValueVisitor visitor;
            boost::apply_visitor(visitor, attrib);
            bf::at_c<0>(ctx.attributes) = visitor.value;
        };

        static auto set_doc_value = [](auto& attrib, auto&){
            DocumentBuilder::current()->set_doc_value(bf::at_c<1>(attrib));
        };


        quoted_string    = lexeme['\'' >> *(char_ - '\'' - "\\\'" | '\\' >> char_('\'')) >> '\'']
                           | lexeme['"' >> *(char_ - '"' - "\\\"" | '\\' >> char_('"')) >> '"'];

        hermes_string    = qi::eps[clear_string_buffer] >> quoted_string [finish_string];

        identifier       = (lexeme[(enc::alpha | char_('_')) >> *(enc::alnum | char_('_'))]
                            - "null" - "true" - "false" - "const" - "volatile" - "signed"
                            - "unsigned" - "int" - "long" - "char" - "double" - "float" - "short" - "bool"
                            );

        hermes_identifier = qi::eps[clear_string_buffer] >> identifier [finish_identifier];

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

        datatype_name  = (hermes_identifier % "::") | cxx_basic_type;


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

        auto check_typeref = [](auto& typeref) {
            return DocumentBuilder::current()->check_typeref(typeref->view());
        };

        type_decl_or_reference = type_declaration | (type_reference[_val = _1, _pass = boost::phoenix::bind(check_typeref,_1)]);

        typed_value      = '@' > type_decl_or_reference > "=" > hermes_value;

        hermes_string_or_typed_value = hermes_string >> -('@' > type_decl_or_reference);


        null_value   = lexeme[lit("null")][dummy];

        array        = ('[' >> (hermes_value % ',') > ']') |
                                            (lit('[') >> ']');

        map_entry    = ((hermes_string | hermes_identifier) > ':' > hermes_value);

        map          = ('{' >> (map_entry % ',') > '}') |
                                            (lit('{') >> '}');

        type_directory_entry = hermes_identifier >> ':' >> type_declaration;

        type_directory = ("#{" >> (type_directory_entry % ',') >> '}') | (lit("#{") > "}");

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

        hermes_value =  (hermes_string_or_typed_value |
                                        strict_double |
                                        integer_value |
                                        map |
                                        null_value |
                                        array |
                                        type_declaration |
                                        typed_value |
                                        bool_value
                                      )[finish_value];

        hermes_document = (-type_directory > hermes_value) [set_doc_value];

        single_line_comment = "//" >> *(char_ - eol) >> (eol|eoi);
        skipper = qi::space | single_line_comment;

        standalone_type_decl = type_declaration;
        standalone_hermes_value = hermes_value;

        BOOST_SPIRIT_DEBUG_NODE(hermes_document);
        BOOST_SPIRIT_DEBUG_NODE(type_declaration);
        BOOST_SPIRIT_DEBUG_NODE(cxx_basic_type);

        hermes_document.name("hermes_document");
        hermes_value.name("hermes_value");
        standalone_hermes_value.name("standalone_hermes_value");

        array.name("array");
        map.name("map");
        map_entry.name("map_entry");
        null_value.name("null_value");
        quoted_string.name("quoted_string");
        hermes_string.name("hermes_string");
        hermes_string_or_typed_value.name("hermes_string_or_typed_value");
        identifier.name("identifier");
        hermes_identifier.name("hermes_identifier");
        cxx_basic_type.name("cxx_basic_type");
        type_declaration.name("type_declaration");

        standalone_type_decl.name("standalone_type_decl");
        typed_value.name("typed_value");
        type_directory.name("type_directory");
        type_directory_entry.name("type_directory_entry");
        type_reference.name("type_reference");
        type_decl_or_reference.name("type_decl_or_reference");
        bool_value_true.name("bool_value_true");
        bool_value_false.name("bool_value_false");
        bool_value.name("bool_value");
        skipper.name("skipper");
        single_line_comment.name("single_line_comment");
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

    qi::rule<Iterator, pool::SharedPtr<HermesDoc>(), Skipper> hermes_document;
    qi::rule<Iterator, ValuePtr(), Skipper>             hermes_value;
    qi::rule<Iterator, ValuePtr(), Skipper>             standalone_hermes_value;

    qi::rule<Iterator, ArrayValue(), Skipper>           array;
    qi::rule<Iterator, MapValue(), Skipper>             map;

    qi::rule<Iterator, MapEntryTuple(), Skipper>        map_entry;

    qi::rule<Iterator, NullValue(), Skipper>            null_value;

    qi::rule<Iterator, ParsedStringValue(), Skipper>    quoted_string;
    qi::rule<Iterator, StringValuePtr(), Skipper>       hermes_string;

    qi::rule<Iterator, StringOrTypedValue(), Skipper>   hermes_string_or_typed_value;

    qi::rule<Iterator, ParsedStringValue(), Skipper>    identifier;
    qi::rule<Iterator, ParsedStringValue(), Skipper>    hermes_identifier;

    qi::rule<Iterator, const char*(), Skipper>          cxx_basic_type;
    qi::rule<Iterator, QualNameValue(), Skipper>        datatype_name;

    qi::rule<Iterator, PtrSpecifier(), Skipper>         ref_ptr_const_qual;

    qi::rule<Iterator, TypeDeclarationValue(), Skipper> type_declaration;
    qi::rule<Iterator, ValuePtr(), Skipper>             type_parameter;
    qi::rule<Iterator, DatatypePtr(), Skipper>          standalone_type_decl;

    qi::rule<Iterator, TypedValueValue(), Skipper>      typed_value;

    qi::rule<Iterator, TypeDirectoryValue(), Skipper>   type_directory;

    qi::rule<Iterator, TypeDirectoryMapEntry(), Skipper> type_directory_entry;


    qi::rule<Iterator, StringValuePtr(), Skipper>       type_reference;
    qi::rule<Iterator, TypeDeclOrReference(), Skipper>  type_decl_or_reference;

    qi::rule<Iterator, bool(), Skipper> bool_value_true;
    qi::rule<Iterator, bool(), Skipper> bool_value_false;
    qi::rule<Iterator, bool(), Skipper> bool_value;

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
        ptrdiff_t pos = err_head - start;

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

        std::stringstream buf;
        bool add_ellipsis = true;
        for (size_t c = 0; c < 125; c++) {
            buf << *err_head;
            if (++err_head == end) {
                add_ellipsis = false;
                break;
            }
        }

        if (add_ellipsis) {
            buf << "...";
        }

        return ErrDescripton{line, column, (size_t)pos, buf.str()};
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
struct RawHermesDocValueParser : qi::grammar<Iterator, ValuePtr(), SkipperT<Iterator>>
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


struct DocumentBuilderCleanup {
    ~DocumentBuilderCleanup() noexcept
    {
        DocumentBuilder::current(nullptr, true);
    }
};



}

template <typename Iterator>
void parse_hermes_document(Iterator& first, Iterator& last, HermesDoc& doc)
{
    DocumentBuilder builder(doc);
    DocumentBuilder::current(&builder);
    DocumentBuilderCleanup cleanup;

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
void parse_datatype_decl(Iterator& first, Iterator& last, HermesDoc& doc)
{
    DocumentBuilder builder(doc);
    DocumentBuilder::current(&builder);
    DocumentBuilderCleanup cleanup;

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

        builder.set_doc_value(type_decl->as_value());
    }
    catch (const ExpectationException<Iterator>& ex) {
        ErrorMessageResolver::instance().do_throw(first, ex);
    }
}


template <typename Iterator>
void parse_raw_datatype_decl(Iterator& first, Iterator& last, HermesDocView& doc, DatatypePtr& datatype)
{
    DocumentBuilder builder(doc);
    DocumentBuilder::current(&builder);
    DocumentBuilderCleanup cleanup;

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
void parse_raw_value0(Iterator& first, Iterator& last, HermesDocView& doc, ValuePtr& value)
{
    DocumentBuilder builder(doc);
    DocumentBuilder::current(&builder);
    DocumentBuilderCleanup cleanup;

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

PoolSharedPtr<HermesDoc> HermesDoc::parse(CharIterator start, CharIterator end, const ParserConfiguration&)
{
    PoolSharedPtr<HermesDoc> doc = TL_allocate_shared<HermesDoc>();
    parse_hermes_document(start, end, *doc);
    return doc;
}

PoolSharedPtr<HermesDoc> HermesDoc::parse_datatype(CharIterator start, CharIterator end, const ParserConfiguration&)
{
    PoolSharedPtr<HermesDoc> doc = TL_allocate_shared<HermesDoc>();
    parse_datatype_decl(start, end, *doc);
    return doc;
}

DatatypePtr HermesDocView::parse_raw_datatype(CharIterator start, CharIterator end, const ParserConfiguration&)
{
    DatatypePtr datatype{};
    parse_raw_datatype_decl(start, end, *this, datatype);
    return datatype;
}

ValuePtr HermesDocView::parse_raw_value(CharIterator start, CharIterator end, const ParserConfiguration&)
{
    ValuePtr value{};
    parse_raw_value0(start, end, *this, value);
    return value;
}

bool HermesDocView::is_identifier(CharIterator start, CharIterator end)
{
    return parse_identifier(start, end);
}

void HermesDocView::assert_identifier(U8StringView name)
{
    if (!is_identifier(name)) {
        MEMORIA_MAKE_GENERIC_ERROR("Supplied value '{}' is not a valid Hermes identifier", name).do_throw();
    }
}

void HermesDoc::init_hermes_doc_parser() {
    // Init Resolver
    ErrorMessageResolver::instance();
}

}}
