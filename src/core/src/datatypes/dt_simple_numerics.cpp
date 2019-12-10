
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

#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/linked/datatypes/default_datatype_ops.hpp>
#include <memoria/v1/core/linked/datatypes/type_registry.hpp>
#include <memoria/v1/core/linked/datatypes/datum.hpp>

#include <memoria/v1/core/linked/document/ld_common.hpp>

#include <memoria/v1/core/linked/datatypes/default_datatype_ops.hpp>

#include <string>

namespace memoria {
namespace v1 {


template <typename T>
struct NumericDataTypeOperationsImpl: DataTypeOperations {

    virtual LDDValueTag type_hash() {
        return TypeHash<T>::Value & 0xFFFFFFFFFFFF;
    }

    virtual U8String full_type_name() {
        SBuf buf;
        DataTypeTraits<T>::create_signature(buf);
        return buf.str();
    }

    virtual boost::any create_cxx_instance(const LDTypeDeclaration& typedecl) {
        return boost::any(T{});
    }

    virtual AnyDatum from_ld_document(const LDDValue& value) {
        MMA1_THROW(UnsupportedOperationException());
    }
};


template <>
struct DataTypeOperationsImpl<UTinyInt>: NumericDataTypeOperationsImpl<UTinyInt> {

    virtual void dump(
            const LDDocumentView* doc,
            LDPtrHolder ptr,
            std::ostream& out,
            LDDumpFormatState& state,
            LDDumpState& dump_state
    ){
        uint8_t val = *(doc->arena_.template get<uint8_t>(ptr));
        out << '\'' << (uint32_t)val << "'@UTinyInt";
    }

    virtual LDPtrHolder deep_copy_to(
            const LDDocumentView* src,
            LDPtrHolder ptr,
            LDDocumentView* tgt,
            ld_::LDArenaAddressMapping& mapping
    ) {
        uint8_t val = *(src->arena_.template get<uint8_t>(ptr));
        LDPtrHolder new_ptr = tgt->template new_value_raw<UTinyInt>(val);
        return new_ptr;
    }

    virtual LDPtrHolder construct_from(
            LDDocumentView* doc,
            const LDDValue& value
    ) {
        if (value.is_string())
        {
            U8String str = value.as_string().view();
            uint8_t vv = std::stoi(str.to_std_string(), nullptr, 0);

            LDPtrHolder new_ptr = doc->template new_value_raw<UTinyInt>(vv);
            return new_ptr;
        }
        else {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Unsupported liked data value");
        }
    }
};

void InitSimpleNumericDatatypes()
{
    DataTypeRegistryStore::NoTCtrOpsInitializer<UTinyInt> ldd_u_tiny_int_;
}

}}
