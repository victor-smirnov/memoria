
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

#include <memoria/v1/core/linked/datatypes/type_registry.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/strings/format.hpp>

#include <memoria/v1/core/linked/datatypes/datum.hpp>

namespace memoria {
namespace v1 {

class LDDValue;
class LDTypeDeclaration;

template <typename T>
struct SimpleDataTypeOperationsImpl: DataTypeOperations {

    virtual LDDValueTag type_hash() {
        return TypeHash<T>::Value & 0xFFFFFFFFFFFF;
    }

    virtual U8String full_type_name() {
        return make_datatype_signature<T>().name();
    }

    virtual boost::any create_cxx_instance(const LDTypeDeclaration& typedecl) {
        MMA1_THROW(UnsupportedOperationException()) << fmt::format_ex(u"DataTypeOperationsImpl<{}>::from_ld_document", full_type_name());
    }

    virtual AnyDatum from_ld_document(const LDDValue& value) {
        MMA1_THROW(UnsupportedOperationException())
                << fmt::format_ex(u"DataTypeOperationsImpl<{}>::from_ld_document", full_type_name());
    }
};

}}
