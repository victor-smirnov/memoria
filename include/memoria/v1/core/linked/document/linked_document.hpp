
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

inline LDDArrayView LDDValueView::as_array() const {
    ld_::ldd_assert_tag<LDDArrayView>(type_tag_);
    return LDDArrayView(doc_, value_ptr_);
}


inline LDDMapView LDDValueView::as_map() const {
    ld_::ldd_assert_tag<LDDMapView>(type_tag_);
    return LDDMapView(doc_, value_ptr_);
}

inline LDDValueView LDDArrayView::get(size_t idx) const
{
    ld_::LDDPtrHolder ptr = array_.access_checked(idx);
    return LDDValueView{doc_, ptr};
}

inline void LDDArrayView::for_each(std::function<void(LDDValueView)> fn) const
{
    array_.for_each([&](const auto& value){
        fn(LDDValueView{doc_, value});
    });
}


inline bool LDDArrayView::is_simple_layout() const noexcept
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



inline LDTypeDeclarationView LDDValueView::as_type_decl() const {
    ld_::ldd_assert_tag<LDTypeDeclarationView>(type_tag_);
    return LDTypeDeclarationView(doc_, value_ptr_);
}

inline LDDTypedValueView LDDValueView::as_typed_value() const {
    ld_::ldd_assert_tag<LDDTypedValueView>(type_tag_);
    return LDDTypedValueView(doc_, value_ptr_);
}




inline LDDValueView LDDocumentView::value() const noexcept {
    return LDDValueView{const_cast<LDDocumentView*>(this), state()->value};
}


inline LDStringView::operator LDDValueView() const noexcept {
    return LDDValueView{doc_, string_.get(), ld_tag_value<LDStringView>()};
}

inline LDDArrayView::operator LDDValueView() const noexcept {
    return LDDValueView{doc_, array_.ptr(), ld_tag_value<LDDArrayView>()};
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
        LDPtr<State> root = allocate_tagged<State>(ld_tag_size<ElementType>(), dst, state);
        ld_::ldd_set_tag(dst, root.get(), ld_tag_value<ElementType>());
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
        LDDValueView src_val(src_doc_, element.get());
        return src_val.deep_copy_to(dst_doc_, mapping);
    }
};





inline LDArenaAddressMapping::LDArenaAddressMapping(const LDDocumentView& src):
    copying_type_(LDDCopyingType::EXPORT)
{
    src.for_each_named_type([&, this](auto name, LDTypeDeclarationView td){
        this->type_names_[td.state_.get()] = TypeNameData{name, false};
    });
}


inline LDArenaAddressMapping::LDArenaAddressMapping(const LDDocumentView& src, const LDDocumentView& dst):
    copying_type_(LDDCopyingType::IMPORT)
{
    src.for_each_named_type([&, this](auto name, LDTypeDeclarationView td){
        this->type_names_[td.state_.get()] = TypeNameData{name, false};
    });

    dst.for_each_named_type([&, this](auto name, LDTypeDeclarationView td){
        U8String type_data = td.to_standard_string();
        this->types_by_data_[type_data] = td.state_.get();
    });
}


}


}}
