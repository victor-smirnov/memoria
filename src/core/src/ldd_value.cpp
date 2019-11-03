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

#ifndef MMA1_NO_REACTOR
#   include <memoria/v1/reactor/reactor.hpp>
#endif

#include <memoria/v1/core/linked/document/linked_document.hpp>

namespace memoria {
namespace v1 {

std::ostream& LDDValue::dump(std::ostream& out, LDDumpState& state) const
{
    if (is_null()) {
        out << "null";
    }
    else if (is_string()) {
        out << "'" << as_string().view() << "'";
    }
    else if (is_integer()) {
        out << as_integer();
    }
    else if (is_double()) {
        out << as_double();
    }
    else if (is_map()) {
        as_map().dump(out, state);
    }
    else if (is_array()) {
        as_array().dump(out, state);
    }
    else if (is_type_decl()) {
        as_type_decl().dump(out, state);
    }
    else if (is_typed_value()) {
        as_typed_value().dump(out, state);
    }
    else {
        out << "<unknown type>";
    }

    return out;
}


bool LDDValue::is_simple_layout() const noexcept {
    if (is_string() || is_integer() || is_null()) {
        return true;
    }

    if (is_map()) {
        return as_map().is_simple_layout();
    }

    if (is_array()) {
        return as_array().is_simple_layout();
    }

    if (is_type_decl()) {
        return as_type_decl().is_simple_layout();
    }

    if (is_typed_value()) {
        return as_typed_value().is_simple_layout();
    }

    return false;
}

}}
