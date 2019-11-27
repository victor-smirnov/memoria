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

#include <memoria/v1/core/linked/document/linked_document.hpp>

#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <unordered_map>

namespace memoria {
namespace v1 {

class TypeSignature {
    U8String name_;
public:
    TypeSignature(U8StringView name) {
        name_ = LDDocument::parse_type_decl(name).value().to_standard_string();
    }

    const U8String& name() const {
        return name_;
    }

    bool equals(const TypeSignature& other) const {
        return name_ == other.name_;
    }

    bool operator==(const TypeSignature& other) const {
        return name_ == other.name_;
    }

    LDTypeDeclaration parse() const {
        return LDDocument::parse_type_decl(name_).value().as_type_decl();
    }

    static LDTypeDeclaration parse(U8StringView str) {
        return LDDocument::parse_type_decl(str).value().as_type_decl();
    }
};

}}
