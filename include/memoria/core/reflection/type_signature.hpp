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

#include <memoria/core/types.hpp>

#include <memoria/core/memory/smart_ptrs.hpp>
#include <memoria/core/strings/string.hpp>
#include <memoria/core/strings/string_buffer.hpp>
#include <memoria/core/tools/type_name.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/memory/memory.hpp>

#include <memoria/core/reflection/reflection.hpp>

#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <boost/fusion/include/adapt_struct.hpp>


#include <unordered_map>

namespace memoria {

namespace hermes {
class HermesCtrView;
}

class TypeSignature {
    U8String name_;

    TypeSignature(U8StringView name, int):
        name_(name)
    {}

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

    static TypeSignature make_normalized(U8StringView str);

    hermes::HermesCtr parse() const;
    static hermes::HermesCtr parse(U8StringView str);
};


template<typename T>
TypeSignature make_datatype_signature(T obj)
{
    auto name = TypeNameFactory<T>::name();
    return TypeSignature::make_normalized(name);
}

template<typename T>
TypeSignature make_datatype_signature()
{
    auto name = TypeNameFactory<T>::name();
    return TypeSignature::make_normalized(name);
}

template<typename T>
U8String make_datatype_signature_string()
{
    auto name = TypeNameFactory<T>::name();
    return normalize_type_declaration(name);
}

}
