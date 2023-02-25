
// Copyright 2017 Victor Smirnov
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

#include <cstring>
#include <type_traits>

namespace memoria {

template <typename TypeTo, typename TypeFrom>
TypeTo* ptr_cast(TypeFrom* ptr) noexcept
{
    static_assert(
        (std::is_const<TypeFrom*>::value && std::is_const<TypeTo*>::value) ||
        ((!std::is_const<TypeFrom*>::value) && (!std::is_const<TypeTo*>::value))
    , "");

    static_assert(std::is_pointer<TypeFrom*>::value, "");
    static_assert(std::is_pointer<TypeTo*>::value, "");

    // FIXME: We need to restrict memory laundering for polymorphic types
    //static_assert(!std::is_polymorphic<TypeTo>::value, "");
    //static_assert(!std::is_polymorphic<TypeFrom>::value, "");
    
    TypeTo* result;
    
    std::memcpy(&result, &ptr, sizeof(ptr));
    
    return result;
}

template <typename TypeTo, typename TypeFrom>
TypeTo value_cast(const TypeFrom& ptr) noexcept
{
    static_assert(sizeof(TypeFrom) == sizeof(TypeTo), "");

    static_assert(std::is_trivially_copy_constructible<TypeTo>::value, "");
    static_assert(std::is_trivially_copy_constructible<TypeFrom>::value, "");

    TypeTo result;

    std::memcpy(&result, &ptr, sizeof(TypeFrom));

    return result;
}

}
