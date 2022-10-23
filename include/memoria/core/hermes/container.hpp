
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
#include <memoria/core/datatypes/traits.hpp>


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

class DocumentBuilder;

class ParserConfiguration {
public:
    ParserConfiguration() {}
};

class HermesCtr: public HoldingView {

protected:

    struct DocumentHeader {
        arena::RelativePtr<void> root;

        DocumentHeader* deep_copy_to(
                arena::ArenaAllocator& dst,
                void* owner_view,
                ViewPtrHolder* ptr_holder,
                DeepCopyDeduplicator& dedup) const
        {
            auto dh = dst.get_resolver_for(dst.allocate_object_untagged<DocumentHeader>());
            dedup.map(dst, this, dh.get(dst));

            if (root.is_not_null())
            {
                auto tag0 = arena::read_type_tag(root.get());
                void* new_root = get_type_reflection(tag0).deep_copy(dst, root.get(), owner_view, ptr_holder, dedup);
                dh.get(dst)->root = new_root;
            }
            else {
                dh.get(dst)->root = nullptr;
            }

            return dh.get(dst);
        }
    };

    size_t segment_size_;
    mutable arena::ArenaAllocator* arena_;
    mutable DocumentHeader* header_;

    friend class DocumentBuilder;

    template <typename, typename>
    friend class Map;

    template <typename>
    friend class Array;

    template <typename>
    friend struct memoria::DataTypeTraits;

    friend class Datatype;

public:
    using CharIterator = typename U8StringView::const_iterator;

    HermesCtr(): segment_size_(), arena_(), header_() {}


    HermesCtr(void* segment, size_t segment_size, ViewPtrHolder* ref_holder) noexcept:
        HoldingView(ref_holder),
        segment_size_(segment_size),
        header_(reinterpret_cast<DocumentHeader*>(segment))
    {}

    HermesCtr(Span<uint8_t> span, ViewPtrHolder* ref_holder) noexcept:
        HoldingView(ref_holder),
        segment_size_(span.size()),
        header_(reinterpret_cast<DocumentHeader*>(span.data()))
    {}

    virtual ~HermesCtr() noexcept = default;


    bool is_mutable() const noexcept {
        return arena_ != nullptr && arena_->is_chunked();
    }

    arena::ArenaAllocator* arena() noexcept {
        return arena_;
    }

    ValuePtr root() const noexcept
    {
        if (MMA_LIKELY(header_->root.is_not_null())) {
            return ValuePtr(Value(header_->root.get(), mutable_self(), ptr_holder_));
        }
        else {
            return ValuePtr{};
        }
    }

    U8String to_string() const
    {
        DumpFormatState fmt = DumpFormatState().simple();
        std::stringstream ss;
        stringify(ss, fmt);
        return ss.str();
    }

    U8String to_pretty_string() const
    {
        DumpFormatState fmt = DumpFormatState();
        std::stringstream ss;
        stringify(ss, fmt);
        return ss.str();
    }

    void stringify(std::ostream& out) const
    {
        DumpFormatState state;
        DumpState dump_state(*this);
        stringify(out, state, dump_state);
    }

    void stringify(std::ostream& out, DumpFormatState& format) const
    {
        DumpState dump_state(*this);
        stringify(out, format, dump_state);
    }

    void stringify(std::ostream& out, DumpFormatState& state, DumpState& dump_state) const {
        root()->stringify(out, state, dump_state);
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
    DataObjectPtr<DT> set_dataobject(DTTViewType<DT> view)
    {
        assert_not_null();
        assert_mutable();

        auto ptr = this->new_dataobject<DT>(view);

        header_->root = ptr->dt_ctr_;

        return ptr;
    }


    GenericArrayPtr set_generic_array()
    {
        assert_not_null();
        assert_mutable();

        auto ptr = this->new_array();

        header_->root = ptr->array_;

        return ptr;
    }

    GenericMapPtr set_generic_map()
    {
        assert_not_null();
        assert_mutable();

        auto ptr = this->new_map();

        header_->root = ptr->map_;

        return ptr;
    }

    ValuePtr set_hermes(U8StringView str)
    {
        assert_not_null();
        assert_mutable();

        ValuePtr vv = parse_raw_value(str.begin(), str.end());

        header_->root = vv->addr_;

        return vv;
    }

    void set_null() {
        assert_not_null();
        assert_mutable();
        header_->root = nullptr;
    }

    template <typename DT>

    DataObjectPtr<DT> new_dataobject(DTTViewType<DT> view);
    DatatypePtr new_datatype(U8StringView name);
    DatatypePtr new_datatype(StringValuePtr name);

    pool::SharedPtr<HermesCtr> self() const;

    pool::SharedPtr<HermesCtr> compactify(bool make_immutable = true) const;

    pool::SharedPtr<HermesCtr> clone(bool as_mutable = false) const;

    bool operator==(const HermesCtr& other) const noexcept {
        MEMORIA_MAKE_GENERIC_ERROR("Equals is not implemented for Hermes").do_throw();
    }


    static pool::SharedPtr<HermesCtr> make_pooled(ObjectPools& pool = thread_local_pools());
    static pool::SharedPtr<HermesCtr> make_new(size_t initial_capacity = 4096);

    static PoolSharedPtr<HermesCtr> parse(U8StringView view) {
        return parse(view.begin(), view.end());
    }

    static PoolSharedPtr<HermesCtr> parse_datatype(U8StringView view) {
        return parse_datatype(view.begin(), view.end());
    }

    static PoolSharedPtr<HermesCtr> parse(
            CharIterator start,
            CharIterator end,
            const ParserConfiguration& cfg = ParserConfiguration{}
    );

    static PoolSharedPtr<HermesCtr> parse_datatype(
            CharIterator start,
            CharIterator end,
            const ParserConfiguration& cfg = ParserConfiguration{}
    );

    static void init_hermes_doc_parser();

    template <typename DT>
    static DataObjectPtr<DT> wrap_dataobject(DTTViewType<DT> view);

protected:
    ValuePtr parse_raw_value(
            CharIterator start,
            CharIterator end,
            const ParserConfiguration& cfg = ParserConfiguration{}
    );

    HermesCtr* mutable_self() const {
        return const_cast<HermesCtr*>(this);
    }

    GenericMapPtr new_map();
    GenericArrayPtr new_array();
    GenericArrayPtr new_array(Span<ValuePtr> span);

    TypedValuePtr new_typed_value(DatatypePtr datatype, ValuePtr constructor);

    void set_value(ValuePtr value);

    void deep_copy_from(const DocumentHeader* header, DeepCopyDeduplicator& dedup);

    ValuePtr do_import_value(ValuePtr value);

    Span<uint8_t> span() const
    {
        size_t ss_size;
        if (arena_)
        {
            if (!arena_->is_chunked()) {
                ss_size = arena_->head().size;
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("HermesCtr is multi-chunked!").do_throw();
            }
        }
        else {
            ss_size = segment_size_;
        }

        return Span<uint8_t>(reinterpret_cast<uint8_t*>(header_), ss_size);
    }



private:

    void assert_not_null() const
    {
        if (MMA_UNLIKELY(header_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("HermesCtr is null").do_throw();
        }
    }

    void assert_mutable();
};

}

struct Hermes{};

class HermesDocumentStorage;

template <>
struct DataTypeTraits<Hermes>: DataTypeTraitsBase<Hermes>
{
    using ViewType      = hermes::HermesCtr;
    using ConstViewType = hermes::HermesCtr;
    using AtomType      = uint8_t;

    using DatumStorage  = HermesDocumentStorage;

    using SharedPtrT    = PoolSharedPtr<ViewType>;
    using ConstSharedPtrT = PoolSharedPtr<ViewType>;

    using SpanT      = DTViewSpan<ViewType, SharedPtrT>;
    using ConstSpanT = DTConstViewSpan<ViewType, ConstSharedPtrT>;

    static constexpr bool isDataType          = true;
    static constexpr bool HasTypeConstructors = false;

    static constexpr bool isSdnDeserializable = true;

    static void create_signature(SBuf& buf, const LinkedData& obj)
    {
        buf << "Hermes";
    }

    static void create_signature(SBuf& buf)
    {
        buf << "Hermes";
    }


    using DataSpan = Span<AtomType>;
    using SpanList = TL<DataSpan>;
    using SpanTuple = AsTuple<SpanList>;

    using DataDimensionsList  = TL<DataSpan>;
    using DataDimensionsTuple = AsTuple<DataDimensionsList>;

    using TypeDimensionsList  = TL<>;
    using TypeDimensionsTuple = AsTuple<TypeDimensionsList>;

    static DataDimensionsTuple describe_data(ViewType view) {
        return std::make_tuple(view.span());
    }

    static DataDimensionsTuple describe_data(const ViewType* view) {
        return std::make_tuple(view->span());
    }


    static TypeDimensionsTuple describe_type(ViewType view) {
        return std::make_tuple();
    }

    static TypeDimensionsTuple describe_type(const LinkedData& data_type) {
        return TypeDimensionsTuple{};
    }


    static ViewType make_view(const DataDimensionsTuple& data, ViewPtrHolder* holder)
    {
        return ViewType(std::get<0>(data), holder);
    }

    static ViewType make_view(const TypeDimensionsTuple& type, const DataDimensionsTuple& data, ViewPtrHolder* holder)
    {
        return ViewType(std::get<0>(data), holder);
    }
};



template <typename Buffer>
class SparseObjectBuilder<Hermes, Buffer> {
    Buffer* buffer_;


    using AtomType = DTTAtomType<Hermes>;
    using ViewType = DTTViewType<Hermes>;

    using ViewPtr = PoolSharedPtr<ViewType>;

    PoolSharedPtr<ViewType> doc_;

public:
    SparseObjectBuilder(Buffer* buffer):
        buffer_(buffer)
    {
      doc_ = hermes::HermesCtr::make_new();
    }

    SparseObjectBuilder(SparseObjectBuilder&&) = delete;
    SparseObjectBuilder(const SparseObjectBuilder&) = delete;

    ViewPtr view() {
        return doc_;
    }

    ViewPtr& doc() {
        return doc_;
    }

    void build()
    {
        doc_ = doc_->compactify();
        buffer_->append(*doc_);
        //doc_->clear();
    }
};

class HermesDocumentStorage: public DatumStorageBase<Hermes, typename DataTypeTraits<Hermes>::DatumStorageSelector> {
    using SelectorTag = typename DataTypeTraits<Hermes>::DatumStorageSelector;

    using Base = DatumStorageBase<Hermes, SelectorTag>;
    using typename Base::ViewType;
public:
    HermesDocumentStorage(ViewType view) noexcept: Base(view) {}

    virtual void destroy() noexcept;
    static HermesDocumentStorage* create(ViewType view);
    virtual U8String to_sdn_string() const;
};

}