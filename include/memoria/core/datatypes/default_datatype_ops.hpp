
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

#include <memoria/core/datatypes/type_registry.hpp>

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/strings/format.hpp>

#include <memoria/core/datatypes/datum.hpp>

namespace memoria {

class LDDValueView;
class LDTypeDeclarationView;

template <typename T>
struct SimpleDataTypeOperationsImpl: DataTypeOperations {

    virtual LDDValueTag type_hash() {
        return TypeHash<T>::Value & 0xFFFFFFFFFFFF;
    }

    virtual U8String full_type_name() {
        SBuf buf;
        DataTypeTraits<T>::create_signature(buf);
        return buf.str();
    }

    virtual boost::any create_cxx_instance(const LDTypeDeclarationView& typedecl) {
        MMA_THROW(UnsupportedOperationException())
                << format_ex("DataTypeOperationsImpl<{}>::from_ld_document", full_type_name());
    }

    virtual boost::any create_cxx_instance(const hermes::Datatype& typedecl) {
        MMA_THROW(UnsupportedOperationException())
                << format_ex("DataTypeOperationsImpl<{}>::from_ld_document", full_type_name());
    }

    virtual AnyDatum from_ld_document(const LDDValueView& value) {
        MMA_THROW(UnsupportedOperationException())
                << format_ex("DataTypeOperationsImpl<{}>::from_ld_document", full_type_name());
    }
};


template <typename T>
struct CtrDataTypeOperationsImpl: DataTypeOperations {

    virtual LDDValueTag type_hash() {
        return TypeHash<T>::Value & 0xFFFFFFFFFFFF;
    }

    virtual U8String full_type_name() {
        SBuf buf;
        DataTypeTraits<T>::create_signature(buf);
        return buf.str();
    }

    virtual boost::any create_cxx_instance(const LDTypeDeclarationView& typedecl)
    {
        if (!typedecl.is_stateless()) {
            MMA_THROW(RuntimeException()) << format_ex("Datatype {} is stateless", full_type_name());
        }

        return boost::any(T{});
    }

    virtual boost::any create_cxx_instance(const hermes::Datatype& typedecl)
    {
        return boost::any(T{});
    }

    virtual AnyDatum from_ld_document(const LDDValueView& value) {
        MMA_THROW(UnsupportedOperationException())
                << format_ex("DataTypeOperationsImpl<{}>::from_ld_document", full_type_name());
    }

    virtual void dump(
            const LDDocumentView* doc,
            LDPtrHolder ptr,
            std::ostream& out,
            LDDumpFormatState& state,
            LDDumpState& dump_state
    ) {
        MMA_THROW(UnsupportedOperationException())
                << format_ex("DataTypeOperationsImpl<{}>::dump", full_type_name());
    }

    virtual LDPtrHolder deep_copy_to(
            const LDDocumentView* src,
            LDPtrHolder ptr,
            LDDocumentView* tgt,
            ld_::LDArenaAddressMapping& mapping
    ) {
        MMA_THROW(UnsupportedOperationException())
                << format_ex("DataTypeOperationsImpl<{}>::deep_copy_to", full_type_name());
    }

    virtual LDDValueView construct_from(
            LDDocumentView* doc,
            const LDDValueView& value
    ) {
        MMA_THROW(UnsupportedOperationException())
                << format_ex("DataTypeOperationsImpl<{}>::construct_from", full_type_name());
    }
};

}
