
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
#include <memoria/core/datatypes/traits.hpp>

#include <memoria/core/arena/arena.hpp>
#include <memoria/core/arena/relative_ptr.hpp>

#include <memoria/core/memory/object_pool.hpp>

#include <memoria/core/hermes/object.hpp>
#include <memoria/core/hermes/datatype.hpp>
#include <memoria/core/hermes/array/object_array.hpp>
#include <memoria/core/hermes/map/object_map.hpp>
#include <memoria/core/hermes/map/typed_map.hpp>
#include <memoria/core/hermes/typed_value.hpp>
#include <memoria/core/hermes/parameter.hpp>

#include <memoria/core/hermes/traits.hpp>

namespace memoria {
namespace hermes {

class HermesCtrBuilder;

class ParserConfiguration {
public:
    ParserConfiguration() {}
};

class StaticHermesCtrImpl;
struct CommonInstance;

namespace detail {

template <typename, bool>
struct DTSizeDispatcher;

template <bool SmallV>
struct WrappingImportHelper;

}

template <size_t Size>
class SizedHermesCtrImpl;



template <typename T>
struct ArrayMaker;

class HermesCtr: public HoldingView<HermesCtr> {
    using Base = HoldingView<HermesCtr>;

protected:

    struct DocumentHeader {
        arena::RelativePtr<void> root;

        DocumentHeader* deep_copy_to(
                arena::ArenaAllocator& dst,
                LWMemHolder* ptr_holder,
                DeepCopyDeduplicator& dedup) const
        {
            auto dh = dst.get_resolver_for(dst.allocate_object_untagged<DocumentHeader>());
            dedup.map(dst, this, dh.get(dst));

            if (root.is_not_null())
            {
                auto tag0 = arena::read_type_tag(root.get());
                void* new_root = get_type_reflection(tag0).deep_copy(dst, ptr_holder, root.get(), dedup);
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
    using Base::mem_holder_;

    friend class HermesCtrBuilder;
    friend struct CommonInstance;
    friend class ObjectView;

    template <typename, typename>
    friend class MapView;

    template <typename>
    friend class ArrayView;

    template <typename>
    friend struct memoria::DataTypeTraits;

    template <typename, bool>
    friend struct detail::DTSizeDispatcher;

    template <bool>
    friend struct detail::WrappingImportHelper;

    friend class DatatypeView;

    template <typename, CtrMakers>
    friend struct CtrMakeHelper;

    template <typename>
    friend struct ArrowMaker;

public:
    using CharIterator = typename U8StringView::const_iterator;

    HermesCtr(): segment_size_(), arena_(), header_() {}


    HermesCtr(void* segment, size_t segment_size, LWMemHolder* ref_holder) noexcept:
        Base(ref_holder),
        segment_size_(segment_size),
        header_(reinterpret_cast<DocumentHeader*>(segment))
    {}

    HermesCtr(Span<uint8_t> span, LWMemHolder* ref_holder) noexcept:
        Base(ref_holder),
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

    Object root() const noexcept
    {
        if (MMA_LIKELY(header_->root.is_not_null())) {
            return Object(ObjectView(mem_holder_, header_->root.get()));
        }
        else {
            return Object{};
        }
    }

    U8String to_string(const StringifyCfg& cfg = StringifyCfg()) const
    {
        DumpFormatState fmt(cfg);
        std::stringstream ss;
        stringify(ss, fmt);
        return ss.str();
    }

    U8String to_pretty_string() const
    {
        return to_string(StringifyCfg::pretty());
    }

    void stringify(std::ostream& out, const StringifyCfg& cfg) const
    {
        DumpFormatState state(cfg);
        stringify(out, state);
    }


    void stringify(std::ostream& out, DumpFormatState& state) const {
        root().stringify(out, state);
    }

    static bool is_identifier(U8StringView string) {
        return is_identifier(string.begin(), string.end());
    }

    static bool is_identifier(CharIterator start, CharIterator end);
    static void assert_identifier(U8StringView name);

    Datatype parse_raw_datatype (
            CharIterator start,
            CharIterator end,
            const ParserConfiguration& cfg = ParserConfiguration{}
    );


    template <typename DT>
    Object set_dataobject(DTTViewType<DT> view)
    {
        assert_not_null();
        assert_mutable();

        auto ptr = this->new_dataobject<DT>(view);
        header_->root = ptr.addr();

        return ptr;
    }


    Object set_hermes(U8StringView str)
    {
        assert_not_null();
        assert_mutable();

        Object vv = parse_document(str.begin(), str.end())->root();
        auto vv1 = this->do_import_value(vv);
        header_->root = vv1.storage_.addr;

        return vv1;
    }

    void set_null() {
        assert_not_null();
        assert_mutable();
        header_->root = nullptr;
    }

    void set_root(Object value);

    template <typename DT>
    Object new_dataobject(DTTViewType<DT> view);

    template <typename DT>
    Object new_embeddable_dataobject(DTTViewType<DT> view);

    Datatype new_datatype(U8StringView name);

    pool::SharedPtr<HermesCtr> self() const;

    pool::SharedPtr<HermesCtr> compactify(bool make_immutable = true) const;

    pool::SharedPtr<HermesCtr> clone(bool as_mutable = false) const;

    bool operator==(const HermesCtr& other) const noexcept {
        MEMORIA_MAKE_GENERIC_ERROR("Equals is not implemented for Hermes").do_throw();
    }

    Parameter new_parameter(U8StringView name);


    static pool::SharedPtr<HermesCtr> make_pooled(ObjectPools& pool = thread_local_pools());
    static pool::SharedPtr<HermesCtr> make_new(size_t initial_capacity = 4096);

    static pool::SharedPtr<HermesCtr> from_span(Span<const uint8_t> data);

    static PoolSharedPtr<HermesCtr> parse_document(U8StringView view) {
        return parse_document(view.begin(), view.end());
    }

    static PoolSharedPtr<HermesCtr> parse_datatype(U8StringView view) {
        //println("Parse Datatype: {}", view);
        return parse_datatype(view.begin(), view.end());
    }

    static PoolSharedPtr<HermesCtr> parse_document(
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
    static Object wrap_dataobject(DTTViewType<DT> view);


    TypedValue new_typed_value(Datatype datatype, Object constructor);

    Object parse_raw_value(
            CharIterator start,
            CharIterator end,
            const ParserConfiguration& cfg = ParserConfiguration{}
    );

    template <typename DT>
    Object new_from_string(U8StringView str);

    static PoolSharedPtr<HermesCtr> parse_hermes_path(U8StringView text);

    // FIXME: nees make(Object) variant but unsure about
    // what it should do with its arguments. Creating a copy
    // of an arg (with convertion to the target type) --
    // is one option.

    template <typename T, typename std::enable_if_t<!HermesObject<T>::Value, int> = 0>
    auto make(T&& view);

    template <typename T, typename... CtrArg>
    auto make_t(CtrArg&&... args);

    template <typename DT>
    Object make_dataobject(const DTViewArg<DT>& view);

    template <typename T, std::enable_if_t<std::is_same_v<T, ObjectArray>, int> = 0>
    Array<T> make_array(uint64_t capacity = 6);

    template <typename T, std::enable_if_t<!std::is_same_v<T, ObjectArray>, int> = 0>
    Array<T> make_array(uint64_t capacity = 6);

    template <typename T>
    auto make_array(Span<const T> span);

    template <typename T>
    auto make_array(const std::vector<T>& vv) {
        return make_array(Span<const T>(vv.data(), vv.size()));
    }

    template <typename DT, typename T>
    auto make_array_t(Span<const T> span);

    template <typename DT, typename T>
    auto make_array_t(const std::vector<T>& vv) {
        return make_array_t<DT>(Span<const T>(vv.data(), vv.size()));
    }

    template <typename Key, typename Value>
    Map<Key, Value> make_map(uint64_t capacity = 6);


    ObjectMap make_object_map(size_t capacity = 8) {
        return make_map<Varchar, Object>(capacity);
    }

    TinyObjectMap make_tiny_map(size_t capacity = 4) {
        return make_map<UTinyInt, Object>();
    }

    ObjectArray make_object_array(uint64_t capacity = 4);

    template <typename DT>
    Array<DT> new_typed_array();

    template <typename KeyDT, typename ValueDT>
    Map<KeyDT, ValueDT> new_typed_map();


    Parameter make_parameter(const U8StringView& name);

    Datatype make_datatype(const U8StringView& name);
    Datatype make_datatype(const DTView<Varchar>& name);

    TypedValue make_typed_value(const Datatype& datatype, const Object& constructor);

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

protected:




    template <typename DT>
    static Object wrap_dataobject__full(DTTViewType<DT> view);

    template <typename DT>
    static Object wrap_primitive(DTTViewType<DT> view);

    template <typename DT>
    static Object wrap_primitive(DTTViewType<DT> view, HermesCtr* ctr);

    HermesCtr* mutable_self() const {
        return const_cast<HermesCtr*>(this);
    }

    void deep_copy_from(const DocumentHeader* header, DeepCopyDeduplicator& dedup);

    Object do_import_value(Object value);
    Object do_import_embeddable(Object value);

    template <typename ViewT>
    Object import_object(const Own<ViewT, OwningKind::WRAPPING>& object);

    template <typename ViewT, OwningKind OK>
    Object import_object(const Own<ViewT, OK>& object);

    Object import_object(const Object& object);

    Object import_small_object(const Object& object);




    void init_from(arena::ArenaAllocator& arena);

    static PoolSharedPtr<StaticHermesCtrImpl> get_ctr_instance(size_t size);

    static PoolSharedPtr<HermesCtr> common_instance();

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
    using AtomType      = uint8_t;

    using View2Type = pool::SharedPtr<hermes::HermesCtr>;

    static constexpr bool isDataType          = true;
    static constexpr bool HasTypeConstructors = false;

    static constexpr bool isSdnDeserializable = true;

    static void create_signature(SBuf& buf) {
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

    static ViewType make_view(const DataDimensionsTuple& data, LWMemHolder* holder)
    {
        return ViewType(std::get<0>(data), holder);
    }

    static ViewType make_view(const TypeDimensionsTuple& type, const DataDimensionsTuple& data, LWMemHolder* holder)
    {
        return ViewType(std::get<0>(data), holder);
    }
};


inline PoolSharedPtr<hermes::HermesCtr> operator "" _hdoc(const char* s, std::size_t n) {
    return hermes::HermesCtr::parse_document(s, s + n);
}

}
