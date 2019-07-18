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
#include <memoria/v1/core/tools/type_name.hpp>

namespace memoria {
namespace v1 {

class TypeSignature {
    U8String name_;
public:
    TypeSignature(U8String name):
        name_(name)
    {}

    const U8String& name() const {
        return name_;
    }

    bool equals(const TypeSignature& other) const {
        return name_ == other.name_;
    }

    static void parse(U8StringView str);
};

static inline SharedPtr<TypeSignature> make_type_signature(U8StringView str){
    return MakeShared<TypeSignature>(str);
}

template <typename T>
SharedPtr<TypeSignature> make_type_signature(){
    return MakeShared<TypeSignature>(StandardType2Str<T>().to_u8());
}


}}
