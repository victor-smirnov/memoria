
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/reflection/reflection.hpp>

#include <memoria/core/hermes/datatype.hpp>

namespace memoria {

template <typename T>
class HermesTypedValueReflectionImpl: public TypeCodeTypeReflectionImplBase<T> {
public:
    virtual void hermes_stringify_value(
            size_t vs_tag,
            hermes::ValueStorage& ptr,
            hermes::HermesCtr* doc,
            LWMemHolder* ref_holder,

            std::ostream& out,
            hermes::DumpFormatState& state
    ) {
        hermes::DataObjectView<T> dtt(ptr.addr, doc, ref_holder);
        dtt.stringify(out, state);
    }

    virtual bool hermes_is_simple_layout(
            void* ptr,
            hermes::HermesCtr* doc,
            LWMemHolder* ref_holder
    ) {
        hermes::DataObjectView<T> dtt(ptr, doc, ref_holder);
        return dtt.is_simple_layout();
    }
};


template <typename T>
class DataTypeReflectionImpl: public TypeCodeTypeReflectionImplBase<T> {

    virtual void hermes_stringify_value(
            size_t vs_tag,
            hermes::ValueStorage& ptr,
            hermes::HermesCtr* doc,
            LWMemHolder* ref_holder,

            std::ostream& out,
            hermes::DumpFormatState& state
    ) {
        MEMORIA_MAKE_GENERIC_ERROR("Hermes support for datatype {} is not impleneted", this->str());
    }
};


}
