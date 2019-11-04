
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

#include <memoria/v1/core/linked/document/ld_value.hpp>
#include <memoria/v1/core/linked/document/ld_string.hpp>
#include <memoria/v1/core/linked/document/ld_identifier.hpp>
#include <memoria/v1/core/linked/document/ld_map.hpp>
#include <memoria/v1/core/linked/document/ld_array.hpp>
#include <memoria/v1/core/linked/document/ld_type_decl.hpp>
#include <memoria/v1/core/linked/document/ld_typed_value.hpp>


#include <memoria/v1/core/tools/optional.hpp>

#include <boost/variant.hpp>

namespace memoria {
namespace v1 {

inline LDDArray LDDValue::as_array() const noexcept {
    return LDDArray(doc_, value_ptr_);
}


inline LDDMap LDDValue::as_map() const noexcept {
    return LDDMap(doc_, value_ptr_);
}

inline LDDValue LDDArray::get(size_t idx) const
{
    SDN2PtrHolder ptr = array_.access(idx);
    return LDDValue{doc_, ptr};
}

inline void LDDArray::for_each(std::function<void(LDDValue)> fn) const
{
    array_.for_each([&](const auto& value){
        fn(LDDValue{doc_, value});
    });
}


inline bool LDDArray::is_simple_layout() const noexcept
{
    if (size() > 3) {
        return false;
    }

    bool simple = true;

    for_each([&](auto vv){
        simple = simple && vv.is_simple_layout();
    });

    return simple;
}



inline LDTypeDeclaration LDDValue::as_type_decl() const noexcept {
    return LDTypeDeclaration(doc_, value_ptr_);
}

inline LDDTypedValue LDDValue::as_typed_value() const noexcept {
    return LDDTypedValue(doc_, value_ptr_);
}







inline LDDValue LDDocument::value() const {
    return LDDValue{const_cast<LDDocument*>(this), state()->value};
}







inline LDString::operator LDDValue() const {
    return LDDValue{doc_, string_.get()};
}

inline LDDArray::operator LDDValue() const {
    return LDDValue{doc_, array_.ptr()};
}

}}
