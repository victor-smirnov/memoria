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



#pragma once

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/core/memory/smart_ptrs.hpp>
#include <memoria/v1/core/strings/string.hpp>
#include <memoria/v1/core/strings/string_buffer.hpp>
#include <memoria/v1/core/tools/type_name.hpp>

#include <memoria/v1/core/types/typelist.hpp>

#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <unordered_map>

namespace memoria {
namespace v1 {

class DataTypeDeclaration;

class TypeSignature {
    U8String name_;
public:
    TypeSignature(U8StringView name);

    const U8String& name() const {
        return name_;
    }

    bool equals(const TypeSignature& other) const {
        return name_ == other.name_;
    }

    bool operator==(const TypeSignature& other) const {
        return name_ == other.name_;
    }

    DataTypeDeclaration parse() const;

    static DataTypeDeclaration parse(U8StringView str);
};




class NameToken {
    U8String text_;
public:
    NameToken(U8String text): text_(text) {}
    NameToken() {}

    const U8String& text() const {
        return text_;
    }

    U8String& text() {
        return text_;
    }

    void operator+=(char ch)
    {
        text_.to_std_string().push_back(ch);
    }

    bool is_null_token() const {
        return text_ == "null";
    }

    operator U8String&() {return text_;}
    operator const U8String&() const {return text_;}
    operator U8String&&() {return std::move(text_);}
};

static inline std::ostream& operator<<(std::ostream& out, const NameToken& tk) {
    out << "NameToken[" << tk.text() << "]";
    return out;
}

class TypedStringValue {
    U8String text_;
    boost::optional<boost::recursive_wrapper<DataTypeDeclaration>> type_;
public:
    friend struct boost::fusion::extension::access;

    TypedStringValue() {}
    TypedStringValue(const U8String& text): text_(text) {}
    TypedStringValue(U8String&& text): text_(std::move(text)) {}
    TypedStringValue(const U8String& text, const DataTypeDeclaration& decl);
    TypedStringValue(U8String&& text, DataTypeDeclaration&& decl);

    const U8String& text() const {
        return text_;
    }

    operator const U8String&() const {
        return text_;
    }

    bool has_type() const {
        return type_.has_value();
    }

    const DataTypeDeclaration& type() const {
        return type_.get().get();
    }
};

template <typename ArgType>
class DataTypeArgMapEntry {
    U8String key_;
    ArgType value_;
public:
    friend struct boost::fusion::extension::access;

    const U8String& key() const {return key_;}
    const ArgType& value() const {return value_;}

    const U8String& key() {return key_;}
    const ArgType& value() {return value_;}
};

template <typename ArgType>
class DataTypeArgMap {
    std::vector<DataTypeArgMapEntry<ArgType>> entries_;
    std::unordered_map<U8String, ArgType*> map_;

public:
    friend struct boost::fusion::extension::access;

    std::vector<DataTypeArgMapEntry<ArgType>>& entries() {return entries_;}
    const std::vector<DataTypeArgMapEntry<ArgType>>& entries() const {return entries_;}
    const std::unordered_map<U8String, ArgType*>& map() const {return map_;}

    void build_map()
    {
        // TODO: Implement
    }
};


class DataTypeCtrArg {
    using ArgType = boost::variant<
        TypedStringValue, int64_t, double, NameToken, std::vector<DataTypeCtrArg>, DataTypeArgMap<DataTypeCtrArg>
    >;

    ArgType value_;
public:
    friend struct boost::fusion::extension::access;

    ArgType& value() {return value_;}
    const ArgType& value() const {return value_;}
};

using DataTypeCtrArgs = boost::optional<std::vector<DataTypeCtrArg>>;

class DataTypeDeclaration;
using DataTypeParams = boost::optional<std::vector<DataTypeDeclaration>>;




class DataTypeDeclaration {
    std::vector<U8String> name_tokens_;

    DataTypeCtrArgs constructor_args_;
    DataTypeParams parameters_;

public:
    friend struct boost::fusion::extension::access;

    std::vector<U8String>& name_tokens() {return name_tokens_;}
    const std::vector<U8String>& name_tokens() const {return name_tokens_;}

    DataTypeCtrArgs& constructor_args() {return constructor_args_;}
    const DataTypeCtrArgs& constructor_args() const {return constructor_args_;}

    DataTypeParams& parameters() {return parameters_;}
    const DataTypeParams& parameters() const {return parameters_;}

    U8String full_type_name() const;

    U8String to_standard_string() const
    {
        SBuf buf;
        to_standard_string(buf);
        return buf.str();
    }

    U8String to_typedecl_string() const
    {
        SBuf buf;
        to_typedecl_string(buf);
        return buf.str();
    }

    void to_standard_string(SBuf& buf) const;
    void to_typedecl_string(SBuf& buf) const;
};

inline TypedStringValue::TypedStringValue(const U8String& text, const DataTypeDeclaration& decl):
    text_(text), type_(decl)
{}

inline TypedStringValue::TypedStringValue(U8String&& text, DataTypeDeclaration&& decl):
    text_(std::move(text)), type_(std::move(decl))
{}





}}
