
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

#include <memoria/v1/core/datatypes/default_datatype_ops.hpp>
#include <memoria/v1/core/datatypes/type_registry.hpp>
#include <memoria/v1/core/datatypes/datum.hpp>

#include <memoria/v1/core/linked/document/ld_common.hpp>

#include <memoria/v1/core/datatypes/default_datatype_ops.hpp>

#include <string>


namespace memoria {
namespace v1 {

template <>
struct DataTypeOperationsImpl<LDArray>: SimpleDataTypeOperationsImpl<LDArray> {

    virtual void dump(
            const LDDocumentView* doc,
            LDPtrHolder ptr,
            std::ostream& out,
            LDDumpFormatState& state,
            LDDumpState& dump_state
    ){
        LDDArrayView array(doc, ptr);
        array.dump(out, state, dump_state);
    }

    virtual LDPtrHolder deep_copy_to(
            const LDDocumentView* src,
            LDPtrHolder ptr,
            LDDocumentView* tgt,
            ld_::LDArenaAddressMapping& mapping
    ) {
        LDDArrayView array(src, ptr);
        return array.deep_copy_to(tgt, mapping);
    }

    virtual LDPtrHolder construct_from(
            LDDocumentView* doc,
            const LDDValueView& value
    ) {
        return value.value_ptr_;
    }
};


template <>
struct DataTypeOperationsImpl<LDMap>: SimpleDataTypeOperationsImpl<LDMap> {

    virtual void dump(
            const LDDocumentView* doc,
            LDPtrHolder ptr,
            std::ostream& out,
            LDDumpFormatState& state,
            LDDumpState& dump_state
    ){
        LDDMapView map(doc, ptr);
        map.dump(out, state, dump_state);
    }

    virtual LDPtrHolder deep_copy_to(
            const LDDocumentView* src,
            LDPtrHolder ptr,
            LDDocumentView* tgt,
            ld_::LDArenaAddressMapping& mapping
    ) {
        LDDMapView map(src, ptr);
        return map.deep_copy_to(tgt, mapping);
    }

    virtual LDPtrHolder construct_from(
            LDDocumentView* doc,
            const LDDValueView& value
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
        LDTypeDeclarationView td(doc, ptr);
        td.dump(out, state, dump_state);
    }

    virtual LDPtrHolder deep_copy_to(
            const LDDocumentView* src,
            LDPtrHolder ptr,
            LDDocumentView* tgt,
            ld_::LDArenaAddressMapping& mapping
    ) {
        LDTypeDeclarationView td(src, ptr);
        return td.deep_copy_to(tgt, mapping);
    }

    virtual LDPtrHolder construct_from(
            LDDocumentView* doc,
            const LDDValueView& value
    ) {
        return value.value_ptr_;
    }
};

template <>
struct DataTypeOperationsImpl<LDTypedValue>: SimpleDataTypeOperationsImpl<LDTypedValue> {

    virtual void dump(
            const LDDocumentView* doc,
            LDPtrHolder ptr,
            std::ostream& out,
            LDDumpFormatState& state,
            LDDumpState& dump_state
    ){
        LDDTypedValueView tv(doc, ptr);
        tv.dump(out, state, dump_state);
    }

    virtual LDPtrHolder deep_copy_to(
            const LDDocumentView* src,
            LDPtrHolder ptr,
            LDDocumentView* tgt,
            ld_::LDArenaAddressMapping& mapping
    ) {
        LDDTypedValueView tv(src, ptr);
        return tv.deep_copy_to(tgt, mapping);
    }

    virtual LDPtrHolder construct_from(
            LDDocumentView* doc,
            const LDDValueView& value
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
        LDStringView str(doc, ptr);
        str.dump(out, state, dump_state);
    }

    virtual LDPtrHolder deep_copy_to(
            const LDDocumentView* src,
            LDPtrHolder ptr,
            LDDocumentView* tgt,
            ld_::LDArenaAddressMapping& mapping
    ) {
        LDStringView str(src, ptr);
        return str.deep_copy_to(tgt, mapping);
    }

    virtual LDPtrHolder construct_from(
            LDDocumentView* doc,
            const LDDValueView& value
    ) {
        return value.value_ptr_;
    }
};


void InitCoreLDDatatypes()
{
    DataTypeRegistryStore::NoTCtrOpsInitializer<LDTypedValue> ldd_tv_;
    DataTypeRegistryStore::NoTCtrOpsInitializer<LDArray> ldd_array_;
    DataTypeRegistryStore::NoTCtrOpsInitializer<LDMap> ldd_map_;
    DataTypeRegistryStore::NoTCtrOpsInitializer<LDTypeDeclaration> ldd_td_;
    DataTypeRegistryStore::NoTCtrOpsInitializer<LDString> ldd_string_;

    DataTypeRegistry::local().refresh();
}


}}
