
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

#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/datatypes/default_datatype_ops.hpp>
#include <memoria/core/datatypes/type_registry.hpp>
#include <memoria/core/datatypes/datum.hpp>

#include <memoria/core/linked/document/ld_common.hpp>

#include <memoria/core/datatypes/default_datatype_ops.hpp>

#include <string>


namespace memoria {

template <>
struct DataTypeOperationsImpl<ld::LDArray>: SimpleDataTypeOperationsImpl<ld::LDArray> {

    virtual void dump(
            const ld::LDDocumentView* doc,
            ld::LDPtrHolder ptr,
            std::ostream& out,
            ld::LDDumpFormatState& state,
            ld::LDDumpState& dump_state
    ){
        ld::LDDArrayView array(doc, ptr);
        array.dump(out, state, dump_state);
    }

    virtual ld::LDPtrHolder deep_copy_to(
            const ld::LDDocumentView* src,
            ld::LDPtrHolder ptr,
            ld::LDDocumentView* tgt,
            ld::ld_::LDArenaAddressMapping& mapping
    ) {
        ld::LDDArrayView array(src, ptr);
        return array.deep_copy_to(tgt, mapping);
    }

    virtual ld::LDDValueView construct_from(
            ld::LDDocumentView* doc,
            const ld::LDDValueView& value
    ) {
        if (value.is_array()) {
            return value;
        }
        else {
            MMA_THROW(RuntimeException()) << WhatCInfo("Unsupported constructor arg type");
        }
    }
};


template <>
struct DataTypeOperationsImpl<ld::LDMap>: SimpleDataTypeOperationsImpl<ld::LDMap> {

    virtual void dump(
            const ld::LDDocumentView* doc,
            ld::LDPtrHolder ptr,
            std::ostream& out,
            ld::LDDumpFormatState& state,
            ld::LDDumpState& dump_state
    ){
        ld::LDDMapView map(doc, ptr);
        map.dump(out, state, dump_state);
    }

    virtual ld::LDPtrHolder deep_copy_to(
            const ld::LDDocumentView* src,
            ld::LDPtrHolder ptr,
            ld::LDDocumentView* tgt,
            ld::ld_::LDArenaAddressMapping& mapping
    ) {
        ld::LDDMapView map(src, ptr);
        return map.deep_copy_to(tgt, mapping);
    }

    virtual ld::LDDValueView construct_from(
            ld::LDDocumentView* doc,
            const ld::LDDValueView& value
    ) {
        if (value.is_map()) {
            return value;
        }
        else {
            MMA_THROW(RuntimeException()) << WhatCInfo("Unsupported constructor arg type");
        }
    }
};

template <>
struct DataTypeOperationsImpl<ld::LDTypeDeclaration>: SimpleDataTypeOperationsImpl<ld::LDTypeDeclaration> {

    virtual void dump(
            const ld::LDDocumentView* doc,
            ld::LDPtrHolder ptr,
            std::ostream& out,
            ld::LDDumpFormatState& state,
            ld::LDDumpState& dump_state
    ){
        ld::LDTypeDeclarationView td(doc, ptr);
        td.dump(out, state, dump_state);
    }

    virtual ld::LDPtrHolder deep_copy_to(
            const ld::LDDocumentView* src,
            ld::LDPtrHolder ptr,
            ld::LDDocumentView* tgt,
            ld::ld_::LDArenaAddressMapping& mapping
    ) {
        ld::LDTypeDeclarationView td(src, ptr);
        return td.deep_copy_to(tgt, mapping);
    }

    virtual ld::LDDValueView construct_from(
            ld::LDDocumentView* doc,
            const ld::LDDValueView& value
    ) {
        if (value.is_type_decl()) {
            return value;
        }
        else {
            MMA_THROW(RuntimeException()) << WhatCInfo("Unsupported constructor arg type");
        }
    }
};

template <>
struct DataTypeOperationsImpl<ld::LDTypedValue>: SimpleDataTypeOperationsImpl<ld::LDTypedValue> {

    virtual void dump(
            const ld::LDDocumentView* doc,
            ld::LDPtrHolder ptr,
            std::ostream& out,
            ld::LDDumpFormatState& state,
            ld::LDDumpState& dump_state
    ){
        ld::LDDTypedValueView tv(doc, ptr);
        tv.dump(out, state, dump_state);
    }

    virtual ld::LDPtrHolder deep_copy_to(
            const ld::LDDocumentView* src,
            ld::LDPtrHolder ptr,
            ld::LDDocumentView* tgt,
            ld::ld_::LDArenaAddressMapping& mapping
    ) {
        ld::LDDTypedValueView tv(src, ptr);
        return tv.deep_copy_to(tgt, mapping);
    }

    virtual ld::LDDValueView construct_from(
            ld::LDDocumentView* doc,
            const ld::LDDValueView& value
    ) {
        if (value.is_typed_value()) {
            return value;
        }
        else {
            MMA_THROW(RuntimeException()) << WhatCInfo("Unsupported constructor arg type");
        }
    }
};


template <>
struct DataTypeOperationsImpl<Varchar>: SimpleDataTypeOperationsImpl<Varchar> {

    virtual void dump(
            const ld::LDDocumentView* doc,
            ld::LDPtrHolder ptr,
            std::ostream& out,
            ld::LDDumpFormatState& state,
            ld::LDDumpState& dump_state
    ){
        ld::LDStringView str(doc, ptr);
        str.dump(out, state, dump_state);
    }

    virtual ld::LDPtrHolder deep_copy_to(
            const ld::LDDocumentView* src,
            ld::LDPtrHolder ptr,
            ld::LDDocumentView* tgt,
            ld::ld_::LDArenaAddressMapping& mapping
    ) {
        ld::LDStringView str(src, ptr);
        return str.deep_copy_to(tgt, mapping);
    }

    virtual ld::LDDValueView construct_from(
            ld::LDDocumentView* doc,
            const ld::LDDValueView& value
    ) {
        if (value.is_varchar()) {
            return value;
        }
        else {
            MMA_THROW(RuntimeException()) << WhatCInfo("Unsupported constructor arg type");
        }
    }
};


template <>
struct DataTypeOperationsImpl<Boolean>: SimpleDataTypeOperationsImpl<Boolean> {

    virtual void dump(
            const ld::LDDocumentView* doc,
            ld::LDPtrHolder ptr,
            std::ostream& out,
            ld::LDDumpFormatState& state,
            ld::LDDumpState& dump_state
    ){
        using StorageType = DTTLDStorageType<Boolean>;
        StorageType val = *(doc->arena_.template get<StorageType>(ptr));
        out << (val ? "true" : "false");
    }

    virtual ld::LDPtrHolder deep_copy_to(
            const ld::LDDocumentView* src,
            ld::LDPtrHolder ptr,
            ld::LDDocumentView* tgt,
            ld::ld_::LDArenaAddressMapping& mapping
    ) {
        using StorageType = DTTLDStorageType<Boolean>;
        StorageType val = *(src->arena_.template get<StorageType>(ptr));
        return tgt->template new_value<Boolean>(val);
    }

    virtual ld::LDDValueView construct_from(
            ld::LDDocumentView* doc,
            const ld::LDDValueView& value
    ) {
        if (value.is_varchar())
        {
            U8StringView view = value.as_varchar().view();

            if ("true" == view) {
                return ld::LDDValueView{doc, doc->new_value<Boolean>(true), ld::ld_tag_value<Boolean>()};
            }
            else if ("false" == view) {
                return ld::LDDValueView{doc, doc->new_value<Boolean>(false), ld::ld_tag_value<Boolean>()};
            }
            else {
                MMA_THROW(RuntimeException()) << format_ex("Unsupported boolean string value: {}", view);
            }
        }
        else {
            MMA_THROW(RuntimeException()) << WhatCInfo("Unsupported linked data value type");
        }
    }
};


void InitCoreLDDatatypes()
{
    register_notctr_operations<ld::LDTypedValue>();
    register_notctr_operations<ld::LDArray>();
    register_notctr_operations<ld::LDMap>();
    register_notctr_operations<ld::LDTypeDeclaration>();
    register_notctr_operations<Varchar>();
    register_notctr_operations<Boolean>();
}



}
