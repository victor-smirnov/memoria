
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
#include <memoria/core/hermes/data_object.hpp>
#include <memoria/core/hermes/typed_value.hpp>

#include <memoria/core/hermes/traits.hpp>

namespace memoria {
namespace hermes {

class HermesDoc;
class DocumentBuilder;

class ParserConfiguration {
public:
    ParserConfiguration() {}
};

class HermesDocView: public HoldingView {

protected:    

    struct DocumentHeader {
        arena::RelativePtr<void> value;
        arena::RelativePtr<void> types;
        arena::RelativePtr<void> strings;
    };

    arena::ArenaAllocator* arena_;
    DocumentHeader* header_;

    friend class DocumentBuilder;

    template <typename, typename>
    friend class Map;

    template <typename>
    friend class Array;

    friend class Datatype;

public:
    using CharIterator = typename U8StringView::const_iterator;

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

    static bool is_identifier(U8StringView string) {
        return is_identifier(string.begin(), string.end());
    }

    static bool is_identifier(CharIterator start, CharIterator end);
    static void assert_identifier(U8StringView name);

    DatatypePtr parse_raw_datatype (
            CharIterator start,
            CharIterator end,
            const ParserConfiguration& cfg = ParserConfiguration{}
    );


    template <typename DT>
    DataObjectPtr<DT> set_tv(DTTViewType<DT> view)
    {
        assert_not_null();
        assert_mutable();

        auto ptr = this->new_tv<DT>(view);

        header_->value = ptr->addr_;

        return ptr;
    }


    GenericArrayPtr set_generic_array()
    {
        assert_not_null();
        assert_mutable();

        auto ptr = this->new_array();

        header_->value = ptr->array_;

        return ptr;
    }

    GenericMapPtr set_generic_map()
    {
        assert_not_null();
        assert_mutable();

        auto ptr = this->new_map();

        header_->value = ptr->map_;

        return ptr;
    }

    template <typename DT>

    DataObjectPtr<DT> new_tv(DTTViewType<DT> view);
    DatatypePtr new_datatype(U8StringView name);
    DatatypePtr new_datatype(StringValuePtr name);

protected:
    ValuePtr parse_raw_value(
            CharIterator start,
            CharIterator end,
            const ParserConfiguration& cfg = ParserConfiguration{}
    );

    GenericMapPtr new_map();
    GenericArrayPtr new_array();
    GenericArrayPtr new_array(Span<ValuePtr> span);

    TypedValuePtr new_typed_value(DatatypePtr datatype, ValuePtr constructor);

    void set_value(ValuePtr value);

private:

    void assert_not_null() const
    {
        if (MMA_UNLIKELY(header_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("HermesDocView is null");
        }
    }

    void assert_mutable();
};

inline ViewPtr<HermesDocView, true> HermesDocView::self_ptr() {
    return ViewPtr<HermesDocView>(HermesDocView(header_, ptr_holder_));
}

class HermesDoc final: public HermesDocView, public pool::enable_shared_from_this<HermesDoc> {
    arena::ArenaAllocator arena_;

    ViewPtrHolder view_ptr_holder_;

    friend class DocumentBuilder;
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

    ViewPtr<Map<Varchar, Value>> set_generic_map()
    {
        using MapT = Map<Varchar, Value>;

        auto arena_map = arena_.allocate_tagged_object<typename MapT::ArenaMap>(TypeHashV<MapT>);
        header_->value = arena_map;

        return ViewPtr<MapT>{MapT(arena_map, this, ptr_holder_)};
    }


    static pool::SharedPtr<HermesDoc> make_pooled(ObjectPools& pool = thread_local_pools());
    static pool::SharedPtr<HermesDoc> make_new(size_t initial_capacity = 4096);

    static PoolSharedPtr<HermesDoc> parse(U8StringView view) {
        return parse(view.begin(), view.end());
    }

    static PoolSharedPtr<HermesDoc> parse_datatype(U8StringView view) {
        return parse_datatype(view.begin(), view.end());
    }

    static PoolSharedPtr<HermesDoc> parse(
            CharIterator start,
            CharIterator end,
            const ParserConfiguration& cfg = ParserConfiguration{}
    );

    static PoolSharedPtr<HermesDoc> parse_datatype(
            CharIterator start,
            CharIterator end,
            const ParserConfiguration& cfg = ParserConfiguration{}
    );

protected:


    void configure_refholder(SharedPtrHolder* ref_holder) {
        view_ptr_holder_.set_owner(ref_holder);
    }
};

}}
