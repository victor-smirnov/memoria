
// Copyright 2021 Victor Smirnov
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
#include <memoria/core/tools/uuid.hpp>

#include <memoria/core/strings/string.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>

#include <memory>

PYBIND11_DECLARE_HOLDER_TYPE(T, memoria::SharedPtr<T>);
PYBIND11_DECLARE_HOLDER_TYPE(T, memoria::LocalSharedPtr<T>);

namespace pybind11 {
namespace detail {
template <typename CharT, class Traits>
struct type_caster<boost::basic_string_view<CharT, Traits>, enable_if_t<is_std_char_type<CharT>::value>>
    : string_caster<boost::basic_string_view<CharT, Traits>, true> {};
} // namespace detail
} // namespace pybind11


namespace pybind11 {
namespace detail {
template <>
struct type_caster<memoria::U8String>
    : string_caster<memoria::U8String, false> {};
} // namespace detail
} // namespace pybind11

namespace memoria {

template <typename T> struct PythonAPIBinder;

class GlobalBindings {};

template <typename Profile>
class GenericProfileBindings {};

std::string get_script_name(const std::string& ts);

template <typename DT>
std::string get_datatype_script_name(std::string prefix)
{
    SBuf buf;
    DataTypeTraits<DT>::create_signature(buf);
    return prefix + get_script_name(buf.str());
}

}
