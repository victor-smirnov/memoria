
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

    virtual LDDValueView construct_from(
            LDDocumentView* doc,
            const LDDValueView& value
    ) {
        if (value.is_array()) {
            return value;
        }
        else {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Unsupported constructor arg type");
        }
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

    virtual LDDValueView construct_from(
            LDDocumentView* doc,
            const LDDValueView& value
    ) {
        if (value.is_map()) {
            return value;
        }
        else {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Unsupported constructor arg type");
        }
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

    virtual LDDValueView construct_from(
            LDDocumentView* doc,
            const LDDValueView& value
    ) {
        if (value.is_type_decl()) {
            return value;
        }
        else {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Unsupported constructor arg type");
        }
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

    virtual LDDValueView construct_from(
            LDDocumentView* doc,
            const LDDValueView& value
    ) {
        if (value.is_typed_value()) {
            return value;
        }
        else {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Unsupported constructor arg type");
        }
    }
};


template <>
struct DataTypeOperationsImpl<Varchar>: SimpleDataTypeOperationsImpl<Varchar> {

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

    virtual LDDValueView construct_from(
            LDDocumentView* doc,
            const LDDValueView& value
    ) {
        if (value.is_varchar()) {
            return value;
        }
        else {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Unsupported constructor arg type");
        }
    }
};


template <>
struct DataTypeOperationsImpl<Boolean>: SimpleDataTypeOperationsImpl<Boolean> {

    virtual void dump(
            const LDDocumentView* doc,
            LDPtrHolder ptr,
            std::ostream& out,
            LDDumpFormatState& state,
            LDDumpState& dump_state
    ){
        using StorageType = DTTLDStorageType<Boolean>;
        StorageType val = *(doc->arena_.template get<StorageType>(ptr));
        out << (val ? "true" : "false");
    }

    virtual LDPtrHolder deep_copy_to(
            const LDDocumentView* src,
            LDPtrHolder ptr,
            LDDocumentView* tgt,
            ld_::LDArenaAddressMapping& mapping
    ) {
        using StorageType = DTTLDStorageType<Boolean>;
        StorageType val = *(src->arena_.template get<StorageType>(ptr));
        return tgt->template new_value<Boolean>(val);
    }

    virtual LDDValueView construct_from(
            LDDocumentView* doc,
            const LDDValueView& value
    ) {
        if (value.is_varchar())
        {
            U8StringView view = value.as_varchar().view();

            if ("true" == view) {
                return LDDValueView{doc, doc->new_value<Boolean>(true), ld_tag_value<Boolean>()};
            }
            else if ("false" == view) {
                return LDDValueView{doc, doc->new_value<Boolean>(false), ld_tag_value<Boolean>()};
            }
            else {
                MMA1_THROW(RuntimeException()) << fmt::format_ex(u"Unsupported boolean string value: {}", view);
            }
        }
        else {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Unsupported linked data value type");
        }
    }
};


void InitCoreLDDatatypes()
{
    register_notctr_operations<LDTypedValue>();
    register_notctr_operations<LDArray>();
    register_notctr_operations<LDMap>();
    register_notctr_operations<LDTypeDeclaration>();
    register_notctr_operations<Varchar>();
    register_notctr_operations<Boolean>();
}


}}
