
// Copyright 2019-2022 Victor Smirnov
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

#include <memoria/core/linked/document/ld_document.hpp>
#include <memoria/core/linked/document/ld_array.hpp>
#include <memoria/core/linked/document/ld_value.hpp>

#include <memoria/core/datatypes/core.hpp>

namespace memoria {

class LDDMapView {
public:
    using ValueMap = ld_::ValueMap;
private:

    friend class LDTypeDeclarationView;
    friend class LDDArrayView;
    friend class LDDocumentView;
    friend class LDDMapValue;

    using PtrHolder = typename ld_::LDArenaView::PtrHolderT;
    const LDDocumentView* doc_;
    ValueMap map_;
public:
    LDDMapView(): doc_(), map_() {}

    LDDMapView(const LDDocumentView* doc, ld_::LDPtr<ValueMap::State> map, LDDValueTag tag = 0):
        doc_(doc), map_(&doc_->arena_, map)
    {}

    LDDMapView(const LDDocumentView* doc, ValueMap map):
        doc_(doc), map_(map)
    {}

    operator LDDValueView() const {
        return *as_object();
    }

    bool operator==(const LDDMapView& other) const noexcept {
        return doc_->equals(other.doc_) && map_.ptr() == other.map_.ptr();
    }

    ViewPtr<LDDValueView> as_object() const {
        return ViewPtr<LDDValueView>(
            LDDValueView{doc_, map_.ptr()},
            doc_->owner_
        );
    }

    ViewPtr<LDDValueView> get(U8StringView name) const
    {
        Optional<ld_::LDGenericPtr<ld_::GenericValue>> ptr = map_.get(name);
        if (ptr) {
            return ViewPtr<LDDValueView>(LDDValueView(doc_, ptr.get()), doc_->owner_);
        }
        else {
            return ViewPtr<LDDValueView>{};
        }
    }



    void set_varchar(U8StringView name, DTTViewType<Varchar> value)
    {
        set_value<Varchar>(name, value);
    }

    void set_bigint(U8StringView name, int64_t value)
    {
        set_value<BigInt>(name, value);
    }

    void set_double(U8StringView name, double value)
    {
        set_value<Double>(name, value);
    }

    void set_boolean(U8StringView name, bool value)
    {
        set_value<Boolean>(name, value);
    }


    template <typename T, typename... Args>
    ViewPtr<LDDValueView> set_value(U8StringView name, Args&&... args)
    {
        LDDocumentView* mutable_doc = doc_->make_mutable();

        LDStringView name_str  = mutable_doc->new_varchar(name);
        auto vv  = mutable_doc->template new_value<T>(std::forward<Args>(args)...);

        map_.put(name_str.string_, vv);

        return ViewPtr<LDDValueView>(
            LDDValueView{doc_, vv, ld_tag_value<T>()},
            doc_->owner_
        );
    }


    ViewPtr<LDDMapView, false> set_map(U8StringView name)
    {
        return set_value<LDMap>(name)->as_map();
    }

    ViewPtr<LDDArrayView, false> set_array(U8StringView name)
    {
        return set_value<LDArray>(name)->as_array();
    }

    ViewPtr<LDDValueView> set_sdn(U8StringView name, U8StringView sdn)
    {
        LDDocumentView* mutable_doc = doc_->make_mutable();

        LDStringView name_str   = mutable_doc->new_varchar(name);
        LDDValueView value      = mutable_doc->parse_raw_value(sdn.begin(), sdn.end());

        map_.put(name_str.string_, value.value_ptr_);

        return ViewPtr<LDDValueView>(value, doc_->owner_);
    }

    ViewPtr<LDDValueView> set_document(U8StringView name, const LDDocument& source)
    {
        LDDocumentView* dst_doc = doc_->make_mutable();

        auto name_str = dst_doc->new_varchar(name).ptr();

        ld_::LDArenaAddressMapping mapping(source, *dst_doc);
        ld_::LDDPtrHolder ptr = source.value()->deep_copy_to(dst_doc, mapping);

        map_.put(name_str, ptr);

        return ViewPtr<LDDValueView>(LDDValueView{doc_, ptr}, doc_->owner_);
    }

    ViewPtr<LDDValueView> set_null(U8StringView name)
    {
        LDDocumentView* dst_doc = doc_->make_mutable();

        auto name_str = dst_doc->new_varchar(name).ptr();

        map_.put(name_str, 0);

        return ViewPtr<LDDValueView>(LDDValueView{doc_, 0}, doc_->owner_);
    }

    void remove(U8StringView name)
    {
        map_.remove(name);
    }

    size_t size() const {
        return map_.size();
    }

    void for_each(std::function<void(U8StringView, LDDValueView)> fn) const
    {
        map_.for_each([&](const auto& key, const auto& value){
            U8StringView kk = key.get(&doc_->arena_)->view();
            fn(kk, LDDValueView{doc_, value});
        });
    }

    std::ostream& dump(std::ostream& out) const
    {
        LDDumpFormatState state;
        LDDumpState dump_state(*doc_);
        dump(out, state, dump_state);
        return out;
    }

    std::ostream& dump(std::ostream& out, LDDumpFormatState& format) const
    {
        LDDumpState dump_state(*doc_);
        dump(out, format, dump_state);
        return out;
    }

    std::ostream& dump(std::ostream& out, LDDumpFormatState state, LDDumpState& dump_state) const
    {
        if (state.indent_size() == 0 || !is_simple_layout()) {
            do_dump(out, state, dump_state);
        }
        else {
            LDDumpFormatState simple_state = state.simple();
            do_dump(out, simple_state, dump_state);
        }

        return out;
    }


    bool is_simple_layout() const noexcept
    {
        if (size() > 2) {
            return false;
        }

        bool simple = true;

        for_each([&](auto key, auto vv){
            simple = simple && vv.is_simple_layout();
        });

        return simple;
    }

    ld_::LDPtr<ValueMap::State> deep_copy_to(LDDocumentView* tgt, ld_::LDArenaAddressMapping& mapping) const;

    PoolSharedPtr<LDDocument> clone(bool compactify = true) const {
        LDDValueView vv = *this;
        return vv.clone(compactify);
    }

private:

    void set_ld_value(LDStringView name, LDDValueView value)
    {
        map_.put(name.string_, value.value_ptr_);
    }

    void do_dump(std::ostream& out, LDDumpFormatState state, LDDumpState& dump_state) const;
};

static inline std::ostream& operator<<(std::ostream& out, const LDDMapView& value) {
    LDDumpFormatState format = LDDumpFormatState().simple();
    value.dump(out, format);
    return out;
}

template <>
struct DataTypeTraits<LDMap> {
    static constexpr bool isDataType = true;
    using LDStorageType = NullType;
    using LDViewType = LDDMapView;

    using SharedPtrT = ViewPtr<LDViewType>;
    using ConstSharedPtrT = DTConstSharedPtr<LDViewType>;

    using SpanT = DTViewSpan<LDViewType, SharedPtrT>;
    using ConstSpanT = DTConstViewSpan<LDViewType, ConstSharedPtrT>;

    static void create_signature(SBuf& buf) {
        buf << "LDMap";
    }
};

template <typename Selector>
struct LDStorageAllocator<LDMap, Selector> {
    template <typename Arena>
    static auto allocate_and_construct(Arena* arena)
    {
        return LDDMapView::ValueMap::create_ptr(arena, ld_tag_size<LDMap>());
    }
};

}
