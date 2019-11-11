
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

#include <memoria/v1/core/linked/document/ld_common.hpp>
#include <memoria/v1/core/linked/document/ld_value.hpp>
#include <memoria/v1/core/linked/document/ld_string.hpp>
#include <memoria/v1/core/linked/document/ld_identifier.hpp>
#include <memoria/v1/core/linked/document/ld_map.hpp>
#include <memoria/v1/core/linked/document/ld_array.hpp>
#include <memoria/v1/core/linked/document/ld_type_decl.hpp>
#include <memoria/v1/core/linked/document/ld_typed_value.hpp>


#include <memoria/v1/core/tools/optional.hpp>

#include <boost/variant.hpp>

namespace memoria {
namespace v1 {

inline LDDArray LDDValue::as_array() const noexcept {
    ld_::ldd_assert_tag<LDDArray>(type_tag_);
    return LDDArray(doc_, value_ptr_);
}


inline LDDMap LDDValue::as_map() const noexcept {
    ld_::ldd_assert_tag<LDDMap>(type_tag_);
    return LDDMap(doc_, value_ptr_);
}

inline LDDValue LDDArray::get(size_t idx) const
{
    ld_::LDDPtrHolder ptr = array_.access(idx);
    return LDDValue{doc_, ptr};
}

inline void LDDArray::for_each(std::function<void(LDDValue)> fn) const
{
    array_.for_each([&](const auto& value){
        fn(LDDValue{doc_, value});
    });
}


inline bool LDDArray::is_simple_layout() const noexcept
{
    if (size() > 3) {
        return false;
    }

    bool simple = true;

    for_each([&](auto vv){
        simple = simple && vv.is_simple_layout();
    });

    return simple;
}



inline LDTypeDeclaration LDDValue::as_type_decl() const noexcept {
    ld_::ldd_assert_tag<LDTypeDeclaration>(type_tag_);
    return LDTypeDeclaration(doc_, value_ptr_);
}

inline LDDTypedValue LDDValue::as_typed_value() const noexcept {
    ld_::ldd_assert_tag<LDDTypedValue>(type_tag_);
    return LDDTypedValue(doc_, value_ptr_);
}




inline LDDValue LDDocumentView::value() const {
    return LDDValue{const_cast<LDDocumentView*>(this), state()->value};
}


inline LDString::operator LDDValue() const {
    return LDDValue{doc_, string_.get(), LDDValueTraits<LDString>::ValueTag};
}

inline LDDArray::operator LDDValue() const {
    return LDDValue{doc_, array_.ptr(), LDDValueTraits<LDDArray>::ValueTag};
}


namespace ld_ {

template <typename ElementType>
struct LDDValueDeepCopyHelperBase {

    const LDDocumentView* src_doc_;
    LDDocumentView* dst_doc_;

public:

    LDDValueDeepCopyHelperBase(const LDDocumentView* src_doc, LDDocumentView* dst_doc):
        src_doc_(src_doc), dst_doc_(dst_doc)
    {}

    template <typename State>
    LDPtr<State> allocate_root(LDArenaView* dst, const State& state)
    {
        LDPtr<State> root = allocate_tagged<State>(sizeof(LDDValueTag), dst, state);
        ld_::ldd_set_tag(dst, root.get(), LDDValueTraits<ElementType>::ValueTag);
        return root;
    }

    template <
            template <typename, typename, typename> class PtrT,
            typename ElementT,
            typename HolderT,
            typename Arena
    >
    PtrT<ElementT, HolderT, Arena> do_deep_copy(
            LDArenaView* dst,
            const LDArenaView* src,
            PtrT<ElementT, HolderT, Arena> element,
            LDArenaAddressMapping& mapping
    )
    {
        LDDValue src_val(src_doc_, element.get());
        return src_val.deep_copy_to(dst_doc_, mapping);
    }
};





inline LDArenaAddressMapping::LDArenaAddressMapping(const LDDocumentView& src):
    copying_type_(LDDCopyingType::EXPORT)
{
    src.for_each_named_type([&, this](auto name, LDTypeDeclaration td){
        this->type_names_[td.state_.get()] = TypeNameData{name, false};
    });
}


inline LDArenaAddressMapping::LDArenaAddressMapping(const LDDocumentView& src, const LDDocumentView& dst):
    copying_type_(LDDCopyingType::IMPORT)
{
    src.for_each_named_type([&, this](auto name, LDTypeDeclaration td){
        this->type_names_[td.state_.get()] = TypeNameData{name, false};
    });

    dst.for_each_named_type([&, this](auto name, LDTypeDeclaration td){
        U8String type_data = td.to_standard_string();
        this->types_by_data_[type_data] = td.state_.get();
    });
}


}


}}
