
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

#include <memoria/core/types.hpp>

#include <memoria/core/datatypes/datatype_ptrs.hpp>

#include <memoria/core/arena/arena.hpp>
#include <memoria/core/arena/relative_ptr.hpp>

#include <memoria/core/memory/object_pool.hpp>

#include <memoria/core/hermes/value.hpp>
#include <memoria/core/hermes/datatype.hpp>
#include <memoria/core/hermes/array.hpp>
#include <memoria/core/hermes/map.hpp>

#include <memoria/core/hermes/traits.hpp>

namespace memoria {
namespace hermes {

class HermesDoc;

class HermesDocView: public HoldingView {

protected:    

    struct DocumentHeader {
        arena::RelativePtr<void> value;
        arena::RelativePtr<void> types;
        arena::RelativePtr<void> strings;
    };

    arena::ArenaAllocator* arena_;
    DocumentHeader* header_;
public:
    HermesDocView(): arena_(), header_() {}


    HermesDocView(DocumentHeader* header, ViewPtrHolder* ref_holder) noexcept:
        HoldingView(ref_holder),
        header_(header)
    {}

    virtual ~HermesDocView() noexcept = default;


    bool is_mutable() const noexcept {
        return arena_ != nullptr;
    }

    arena::ArenaAllocator* arena() noexcept {
        return arena_;
    }

    ViewPtr<Value> value() noexcept {
        return ViewPtr<Value>(Value(header_->value.get(), this, ptr_holder_));
    }

    ViewPtr<HermesDocView, true> self_ptr();

    U8String to_string()
    {
        DumpFormatState fmt = DumpFormatState().simple();
        std::stringstream ss;
        stringify(ss, fmt);
        return ss.str();
    }

    U8String to_pretty_string()
    {
        DumpFormatState fmt = DumpFormatState();
        std::stringstream ss;
        stringify(ss, fmt);
        return ss.str();
    }

    void stringify(std::ostream& out)
    {
        DumpFormatState state;
        DumpState dump_state(*this);
        stringify(out, state, dump_state);
    }

    void stringify(std::ostream& out, DumpFormatState& format)
    {
        DumpState dump_state(*this);
        stringify(out, format, dump_state);
    }

    void stringify(std::ostream& out, DumpFormatState& state, DumpState& dump_state) {

//        if (has_type_dictionary()) {
//            do_dump_dictionary(out, state, dump_state);
//        }

        value()->stringify(out, state, dump_state);
    }

};

inline ViewPtr<HermesDocView, true> HermesDocView::self_ptr() {
    return ViewPtr<HermesDocView>(HermesDocView(header_, ptr_holder_));
}

class HermesDoc final: public HermesDocView, public pool::enable_shared_from_this<HermesDoc> {
    arena::ArenaAllocator arena_;

    ViewPtrHolder view_ptr_holder_;
public:

    HermesDoc() {
        HermesDocView::arena_ = &arena_;
        header_ = arena_.allocate_object_untagged<DocumentHeader>();
        ptr_holder_ = &view_ptr_holder_;
    }

    HermesDoc(size_t chunk_size):
        arena_(chunk_size)
    {
        HermesDocView::arena_ = &arena_;
        header_ = arena_.allocate_object_untagged<DocumentHeader>();
        ptr_holder_ = &view_ptr_holder_;
    }


    ViewPtr<Datatype<Varchar>> set_varchar(U8StringView str)
    {
        using DTCtr = Datatype<Varchar>;
        auto arena_str = arena_.allocate_tagged_object<typename DTCtr::ArenaDTContainer>(
                    TypeHashV<DTCtr>,
                    str
        );
        header_->value = arena_str;

        return ViewPtr<DTCtr>{DTCtr(arena_str, this, ptr_holder_)};
    }

    template <typename DT>
    ViewPtr<Datatype<DT>> set_datatype(DTTViewType<DT> str)
    {
        using DTCtr = Datatype<DT>;
        auto arena_str = arena_.allocate_tagged_object<typename DTCtr::ArenaDTContainer>(
                    TypeHashV<DTCtr>,
                    str
        );
        header_->value = arena_str;

        return ViewPtr<DTCtr>{DTCtr(arena_str, this, ptr_holder_)};
    }


    ViewPtr<Array<Value>> set_generic_array()
    {
        using Arr = Array<Value>;

        auto arena_array = arena_.allocate_tagged_object<typename Arr::ArenaArray>(TypeHashV<Arr>);
        header_->value = arena_array;

        return ViewPtr<Arr>{Arr(arena_array, this, ptr_holder_)};
    }

    ViewPtr<Map<Varchar, Value>> set_generic_map()
    {
        using MapT = Map<Varchar, Value>;

        auto arena_map = arena_.allocate_tagged_object<typename MapT::ArenaMap>(TypeHashV<MapT>);
        header_->value = arena_map;

        return ViewPtr<MapT>{MapT(arena_map, this, ptr_holder_)};
    }


    static pool::SharedPtr<HermesDoc> make_pooled(ObjectPools& pool = thread_local_pools());
    static pool::SharedPtr<HermesDoc> make_new(size_t initial_capacity = 4096);

protected:


    void configure_refholder(SharedPtrHolder* ref_holder) {
        view_ptr_holder_.set_owner(ref_holder);
    }
};

}}
