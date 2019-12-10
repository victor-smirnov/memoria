
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

template <>
struct DataTypeOperationsImpl<LDDArray>: SimpleDataTypeOperationsImpl<LDDArray> {

    virtual void dump(
            const LDDocumentView* doc,
            LDPtrHolder ptr,
            std::ostream& out,
            LDDumpFormatState& state,
            LDDumpState& dump_state
    ){
        LDDArray array(doc, ptr);
        array.dump(out, state, dump_state);
    }

    virtual LDPtrHolder deep_copy_to(
            const LDDocumentView* src,
            LDPtrHolder ptr,
            LDDocumentView* tgt,
            ld_::LDArenaAddressMapping& mapping
    ) {
        LDDArray array(src, ptr);
        return array.deep_copy_to(tgt, mapping);
    }

    virtual LDPtrHolder construct_from(
            LDDocumentView* doc,
            const LDDValue& value
    ) {
        return value.value_ptr_;
    }
};


template <>
struct DataTypeOperationsImpl<LDDMap>: SimpleDataTypeOperationsImpl<LDDMap> {

    virtual void dump(
            const LDDocumentView* doc,
            LDPtrHolder ptr,
            std::ostream& out,
            LDDumpFormatState& state,
            LDDumpState& dump_state
    ){
        LDDMap map(doc, ptr);
        map.dump(out, state, dump_state);
    }

    virtual LDPtrHolder deep_copy_to(
            const LDDocumentView* src,
            LDPtrHolder ptr,
            LDDocumentView* tgt,
            ld_::LDArenaAddressMapping& mapping
    ) {
        LDDMap map(src, ptr);
        return map.deep_copy_to(tgt, mapping);
    }

    virtual LDPtrHolder construct_from(
            LDDocumentView* doc,
            const LDDValue& value
    ) {
        return value.value_ptr_;
    }
};

template <>
struct DataTypeOperationsImpl<LDTypeDeclaration>: SimpleDataTypeOperationsImpl<LDTypeDeclaration> {

    virtual void dump(
            const LDDocumentView* doc,
            LDPtrHolder ptr,
            std::ostream& out,
            LDDumpFormatState& state,
            LDDumpState& dump_state
    ){
        LDTypeDeclaration td(doc, ptr);
        td.dump(out, state, dump_state);
    }

    virtual LDPtrHolder deep_copy_to(
            const LDDocumentView* src,
            LDPtrHolder ptr,
            LDDocumentView* tgt,
            ld_::LDArenaAddressMapping& mapping
    ) {
        LDTypeDeclaration td(src, ptr);
        return td.deep_copy_to(tgt, mapping);
    }

    virtual LDPtrHolder construct_from(
            LDDocumentView* doc,
            const LDDValue& value
    ) {
        return value.value_ptr_;
    }
};

template <>
struct DataTypeOperationsImpl<LDDTypedValue>: SimpleDataTypeOperationsImpl<LDDTypedValue> {

    virtual void dump(
            const LDDocumentView* doc,
            LDPtrHolder ptr,
            std::ostream& out,
            LDDumpFormatState& state,
            LDDumpState& dump_state
    ){
        LDDTypedValue tv(doc, ptr);
        tv.dump(out, state, dump_state);
    }

    virtual LDPtrHolder deep_copy_to(
            const LDDocumentView* src,
            LDPtrHolder ptr,
            LDDocumentView* tgt,
            ld_::LDArenaAddressMapping& mapping
    ) {
        LDDTypedValue tv(src, ptr);
        return tv.deep_copy_to(tgt, mapping);
    }

    virtual LDPtrHolder construct_from(
            LDDocumentView* doc,
            const LDDValue& value
    ) {
        return value.value_ptr_;
    }
};


template <>
struct DataTypeOperationsImpl<LDString>: SimpleDataTypeOperationsImpl<LDString> {

    virtual void dump(
            const LDDocumentView* doc,
            LDPtrHolder ptr,
            std::ostream& out,
            LDDumpFormatState& state,
            LDDumpState& dump_state
    ){
        LDString str(doc, ptr);
        str.dump(out, state, dump_state);
    }

    virtual LDPtrHolder deep_copy_to(
            const LDDocumentView* src,
            LDPtrHolder ptr,
            LDDocumentView* tgt,
            ld_::LDArenaAddressMapping& mapping
    ) {
        LDString str(src, ptr);
        return str.deep_copy_to(tgt, mapping);
    }

    virtual LDPtrHolder construct_from(
            LDDocumentView* doc,
            const LDDValue& value
    ) {
        return value.value_ptr_;
    }
};


void InitCoreLDDatatypes()
{
    DataTypeRegistryStore::NoTCtrOpsInitializer<LDDTypedValue> ldd_tv_;
    DataTypeRegistryStore::NoTCtrOpsInitializer<LDDArray> ldd_array_;
    DataTypeRegistryStore::NoTCtrOpsInitializer<LDDMap> ldd_map_;
    DataTypeRegistryStore::NoTCtrOpsInitializer<LDTypeDeclaration> ldd_td_;
    DataTypeRegistryStore::NoTCtrOpsInitializer<LDString> ldd_string_;

    DataTypeRegistry::local().refresh();
}


}}
