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

#include <memoria/v1/api/datatypes/type_signature.hpp>

#include <unordered_map>

namespace memoria {
namespace v1 {

class SDNValue;

class TypedSDNArray {
    std::vector<SDNValue> array_;
    boost::optional<DataTypeDeclaration> type_;
public:
    friend struct boost::fusion::extension::access;

    std::vector<SDNValue>& array() {return array_;}
    boost::optional<DataTypeDeclaration>& type() {return type_;}

    const std::vector<SDNValue>& array() const {return array_;}
    const boost::optional<DataTypeDeclaration>& type() const {return type_;}
};

class SDNEntry {
    U8String key_;
    std::vector<SDNValue> value_;
public:
    friend struct boost::fusion::extension::access;

    const U8String & key() const {
        return key_;
    }

    const SDNValue & value() const;
    SDNValue& value();
};


class TypedSDNMap {
    std::vector<SDNEntry> entries_;
    std::unordered_map<U8String, SDNValue*> map_;
    boost::optional<DataTypeDeclaration> type_;
public:
    friend struct boost::fusion::extension::access;

    const std::unordered_map<U8String, SDNValue*>& map() const {return map_;}

    std::vector<SDNEntry>& entries() {return entries_;}
    boost::optional<DataTypeDeclaration>& type() {return type_;}

    const std::vector<SDNEntry>& entries() const {return entries_;}
    const boost::optional<DataTypeDeclaration>& type() const {return type_;}

    void build_index();
};


class SDNTypeDictionaryEntry {
    U8String type_id_;
    DataTypeDeclaration type_;
public:
    friend struct boost::fusion::extension::access;

    const U8String & type_id() const {
        return type_id_;
    }

    const DataTypeDeclaration& type() const {return type_;}
    DataTypeDeclaration& type() {return type_;}
};


class SDNTypeDictionary
{
    std::vector<SDNTypeDictionaryEntry> entries_;
    std::unordered_map<U8String, DataTypeDeclaration*> map_;

public:
    friend struct boost::fusion::extension::access;

    const std::unordered_map<U8String, DataTypeDeclaration*>& map() const {return map_;}

    std::vector<SDNTypeDictionaryEntry>& entries() {return entries_;}
    const std::vector<SDNTypeDictionaryEntry>& entries() const {return entries_;}

    void build_index();
    void pretty_print(SBuf& buf, int32_t indent, int32_t initial_indent) const;
    void to_string(SBuf& buf) const;
};

enum class SDNValueType: int32_t {
    TYPED_STRING_VALUE = 0, LONG, DOUBLE, NAME_TOKEN, ARRAY, MAP, TYPE_DECLARATION
};

class SDNValue {
public:
    using Value = boost::variant<
        TypedStringValue,
        int64_t,
        double,
        NameToken,
        TypedSDNArray,
        TypedSDNMap,
        DataTypeDeclaration
    >;

private:
    Value value_;
public:
    friend struct boost::fusion::extension::access;

    static SDNValue parse(U8StringView text);

    auto& value() {
        return value_;
    }

    const auto& value() const {
        return value_;
    }

    U8String to_string() const
    {
        SBuf buf;
        to_string(buf);
        return U8String(std::move(buf.str()));
    }

    void to_string(SBuf& buf) const;

    U8String pretty_print(int32_t indent = 4) const
    {
        SBuf buf;
        pretty_print(buf, indent);
        return U8String(std::move(buf.str()));
    }

    void pretty_print(SBuf& buf, int32_t indent = 4, int32_t initial_indent = -1) const;

    void resolve_maps();

    SDNValueType type() const {
        return static_cast<SDNValueType>(value_.which());
    }

    bool is_null_token() const
    {
        if (type() == SDNValueType::NAME_TOKEN)
        {
            return boost::get<NameToken>(value_).is_null_token();
        }

        return false;
    }

    bool is_scalar() const
    {
        SDNValueType type = this->type();
        return type == SDNValueType::DOUBLE || type == SDNValueType::LONG || type == SDNValueType::TYPED_STRING_VALUE;
    }
};

inline const SDNValue & SDNEntry::value() const {
    return value_[0];
}

inline SDNValue & SDNEntry::value() {
    return value_[0];
}

class SDNDocument {
    SDNValue value_;
    boost::optional<SDNTypeDictionary> type_dictionary_;
public:
    friend struct boost::fusion::extension::access;

    static SDNDocument parse(U8StringView text);

    SDNValue& value() {return value_;}
    const SDNValue& value() const {return value_;}

    U8String to_string() const
    {
        SBuf buf;
        to_string(buf);
        return U8String(std::move(buf.str()));
    }

    void to_string(SBuf& buf) const;

    U8String pretty_print(int32_t indent = 4) const
    {
        SBuf buf;
        pretty_print(buf, indent);
        return U8String(std::move(buf.str()));
    }


    void pretty_print(SBuf& buf, int32_t indent = 4, int32_t initial_indent = -1) const;

    void resolve_maps();
};

}}
