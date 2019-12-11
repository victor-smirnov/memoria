
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


#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/strings/u8_string.hpp>

#include <memoria/v1/core/linked/common/arena.hpp>
#include <memoria/v1/core/linked/common/linked_string.hpp>
#include <memoria/v1/core/linked/common/linked_dyn_vector.hpp>
#include <memoria/v1/core/linked/common/linked_vector.hpp>
#include <memoria/v1/core/linked/common/linked_map.hpp>
#include <memoria/v1/core/linked/common/linked_set.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/strings/format.hpp>

#include <memoria/v1/core/types/typehash.hpp>

#include <memoria/v1/core/datatypes/traits.hpp>

#include <unordered_map>
#include <iostream>

namespace memoria {
namespace v1 {

using LDDValueTag = uint64_t;
struct LDDInvalidCastException: RuntimeException {};

template <typename T>
constexpr size_t ld_tag_size()
{
    uint64_t code = TypeHash<T>::Value;
    if (code < 250)
    {
        return 1;
    }
    else {
        return 8;
    }
}

template <typename T>
constexpr LDDValueTag ld_tag_value()
{
    uint64_t code = TypeHash<T>::Value;
    if (code < 250)
    {
        return code;
    }
    else {
        return (code & 0xFFFFFFFFFFFFFF) | (static_cast<uint64_t>(250) << 56);
    }
}

struct LDDocumentHeader {
    uint64_t version;
};


namespace ld_ {
    using LDArena     = LinkedArena<LDDocumentHeader, uint64_t>;
    using LDArenaView = LinkedArenaView<LDDocumentHeader, uint64_t>;

    using LDDPtrHolder = LDArenaView::PtrHolderT;

    template <typename T>
    using LDPtr = typename LDArenaView::template PtrT<T>;

    template <typename T>
    using LDGenericPtr = typename LDArenaView::template GenericPtrT<T>;

    enum class LDDCopyingType {COMPACTION, EXPORT, IMPORT};
}

using LDPtrHolder = ld_::LDDPtrHolder;


class LDDArrayView;

class LDDValueView;
class LDDMapView;
class LDTypeDeclarationView;
class LDTypeName;
class LDDataTypeParam;
class LDDataTypeCtrArg;
class LDDocument;
class LDDocumentView;
class LDStringView;
class LDIdentifierView;
class LDDTypedValueView;
class LDDocumentBuilder;

using LDBoolean = bool;
using LDInteger = int64_t;
using LDDouble  = double;



template <typename T> struct LDDValueTraits;

namespace ld_ {

    using LDIntegerStorage = LDInteger;
    using LDDoubleStorage  = LDDouble;
    using LDBooleanStorage = uint8_t;

    struct GenericValue {};

    using PtrHolder = LDArenaView::PtrHolderT;

    using ValueMap = LinkedMap<
        LDPtr<U8LinkedString>,
        LDGenericPtr<GenericValue>,
        LDArenaView,
        LinkedPtrHashFn,
        LinkedStringPtrEqualToFn
    >;

    using StringSet = LinkedSet<
        LDPtr<U8LinkedString>,
        LDArenaView,
        LinkedPtrHashFn,
        LinkedStringPtrEqualToFn
    >;

    using MapState = typename ValueMap::State;

    using Array = LinkedDynVector<LDGenericPtr<GenericValue>, LDArenaView>;
    using ArrayState = typename Array::State;

    struct TypeDeclState;

    using TypeDeclPtr = LDPtr<TypeDeclState>;

    struct TypeDeclState {
        LDPtr<U8LinkedString> name;
        LDPtr<LinkedVector<TypeDeclPtr>> type_params;
        LDPtr<LinkedVector<PtrHolder>> ctr_args;
        LDDPtrHolder raw_data;
    };

    struct TypedValueState {
        LDPtr<TypeDeclState> type_decl;
        PtrHolder value_ptr;
    };

    using TypeDeclsMap = LinkedMap<
        LDPtr<U8LinkedString>,
        LDPtr<TypeDeclState>,
        LDArenaView,
        LinkedPtrHashFn,
        LinkedStringPtrEqualToFn
    >;

    struct DocumentState {
        PtrHolder value;
        LDPtr<TypeDeclsMap::State> type_directory;
        LDPtr<ld_::StringSet::State> strings;
    };



    static inline void ldd_set_tag(LDArenaView* arena, PtrHolder ptr, LDDValueTag tag) noexcept
    {
        if (tag < SHORT_TYPEHASH_LENGTH_BASE)
        {
            *T2T<uint8_t*>(arena->data() + ptr - sizeof(uint8_t)) = (uint8_t)tag;
        }
        else {
            tag &= 0xFFFFFFFFFFFFFF;
            tag |= ((LDDValueTag)250) << 56;
            *T2T<LDDValueTag*>(arena->data() + ptr - sizeof(LDDValueTag)) = tag;
        }
    }

    static inline LDDValueTag ldd_get_tag(const LDArenaView* arena, PtrHolder ptr) noexcept
    {
        uint8_t short_value = *T2T<const uint8_t*>(arena->data() + ptr - sizeof(uint8_t));
        if (short_value < SHORT_TYPEHASH_LENGTH_BASE)
        {
            return short_value;
        }
        else {
            LDDValueTag long_value = *T2T<const LDDValueTag*>(arena->data() + ptr - sizeof(LDDValueTag));
            return long_value & 0xFFFFFFFFFFFFFF;
        }
    }



    class LDArenaAddressMapping {

        struct TypeNameData {
            U8StringView name;
            bool imported;
        };


        using AddressMapping  = typename LDArenaView::AddressMapping;
        using TypeNameMapping = std::unordered_map<LDDPtrHolder, TypeNameData>;
        using TypeDataMapping = std::unordered_map<U8String, LDDPtrHolder>;

        AddressMapping mapping_;
        LDDCopyingType copying_type_;
        TypeNameMapping type_names_;
        TypeDataMapping types_by_data_;

    public:
        LDArenaAddressMapping(): copying_type_(LDDCopyingType::COMPACTION) {}
        LDArenaAddressMapping(const LDDocumentView& src);
        LDArenaAddressMapping(const LDDocumentView& src, const LDDocumentView& dst);

        TypeNameMapping& type_name_mapping() {
            return type_names_;
        }

        LDDCopyingType copying_type() const {
            return copying_type_;
        }

        bool is_compaction() const {
            return copying_type_ == LDDCopyingType::COMPACTION;
        }

        bool is_export() const {
            return copying_type_ == LDDCopyingType::EXPORT;
        }

        bool is_import() const {
            return copying_type_ == LDDCopyingType::IMPORT;
        }

        Optional<LDDPtrHolder> resolve(LDDPtrHolder ptr) const
        {
            auto ii = mapping_.find(ptr);
            if (ii != mapping_.end()) {
                return ii->second;
            }

            return Optional<LDDPtrHolder>{};
        }

        void map_ptrs(LDDPtrHolder source_arena_ptr, LDDPtrHolder target_arena_ptr)
        {
            mapping_[source_arena_ptr] = target_arena_ptr;
        }

        Optional<TypeNameData> is_src_named_type(LDDPtrHolder type_ptr) const
        {
            auto ii = type_names_.find(type_ptr);
            if (ii != type_names_.end()) {
                return ii->second;
            }

            return Optional<TypeNameData>{};
        }

        Optional<LDDPtrHolder> is_dst_named_type(U8StringView type_ptr) const
        {
            auto ii = types_by_data_.find(type_ptr);
            if (ii != types_by_data_.end()) {
                return ii->second;
            }

            return Optional<LDDPtrHolder>{};
        }

        void finish_src_named_type(LDDPtrHolder holder) {
            type_names_[holder].imported = true;
        }

        void finish_dst_named_type(const U8String& data, LDDPtrHolder holder) {
            types_by_data_[data] = holder;
        }
    };


    template <typename Base>
    class DeepCopyHelper: public Base {
    protected:
        LDArenaAddressMapping& mapping_;
    public:

        template <typename... Args>
        DeepCopyHelper(LDArenaAddressMapping& mapping, Args&&... args):
            Base(std::forward<Args>(args)...),
            mapping_(mapping)
        {}


        template <
                template <typename, typename, typename> class PtrT,
                typename ElementT,
                typename HolderT,
                typename Arena
        >
        PtrT<ElementT, HolderT, Arena> deep_copy(
                LDArenaView* dst,
                const LDArenaView* src,
                PtrT<ElementT, HolderT, Arena> element
        )
        {
            auto dst_ptr = mapping_.resolve(element.get());

            if (!dst_ptr)
            {
                 auto new_ptr = this->do_deep_copy(dst, src, element, mapping_);
                 mapping_.map_ptrs(element.get(), new_ptr.get());
                 return new_ptr;
            }

            return dst_ptr.get();
        }
    };


    template <typename Type, typename Arena>
    void ldd_assert_tag(const Arena* arena, LDDPtrHolder ptr)
    {
        LDDValueTag tag = ldd_get_tag(arena, ptr);
        if (tag != ld_tag_value<Type>()) {
            MMA1_THROW(LDDInvalidCastException());
        }
    }

    template <typename Type>
    void ldd_assert_tag(LDDValueTag tag)
    {
        if (tag != ld_tag_value<Type>()) {
            MMA1_THROW(LDDInvalidCastException());
        }
    }

    static inline void assert_different_docs(const LDDocumentView* one, const LDDocumentView* two)
    {
        if (one == two) {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Performing operation on the same documents");
        }
    }
}

template <> struct TypeHash<LDStringView>:  UInt64Value<1> {};
template <> struct TypeHash<LDDMapView>:    UInt64Value<2> {};
template <> struct TypeHash<LDDArrayView>:  UInt64Value<3> {};
template <> struct TypeHash<LDTypeDeclarationView>: UInt64Value<4> {};
template <> struct TypeHash<LDDTypedValueView>:     UInt64Value<5> {};
template <> struct TypeHash<LDBoolean>:         UInt64Value<6> {};

class LDDumpFormatState {
    const char* space_;

    const char* nl_start_;
    const char* nl_middle_;
    const char* nl_end_;

    size_t indent_size_;
    size_t current_indent_;

public:
    LDDumpFormatState(
            const char* space,
            const char* nl_start,
            const char* nl_middle,
            const char* nl_end,
            size_t indent_size
    ):
        space_(space),
        nl_start_(nl_start),
        nl_middle_(nl_middle),
        nl_end_(nl_end),
        indent_size_(indent_size),
        current_indent_(0)
    {}

    LDDumpFormatState(): LDDumpFormatState(" ", "\n", "\n", "\n", 2) {}

    static LDDumpFormatState no_indent() {
        return LDDumpFormatState("", "", "", "", 0);
    }

    LDDumpFormatState simple() {
        return LDDumpFormatState("", "", " ", "", 0);
    }

    const char* space() const {return space_;}

    const char* nl_start() const {return nl_start_;}
    const char* nl_middle() const {return nl_middle_;}
    const char* nl_end() const {return nl_end_;}

    size_t indent_size() const {return indent_size_;}
    size_t current_indent() const {return current_indent_;}

    void push() {
        current_indent_ += indent_size_;
    }

    void pop() {
        current_indent_ -= indent_size_;
    }

    void make_indent(std::ostream& out) const {
        for (size_t c = 0; c < current_indent_; c++) {
            out << space_;
        }
    }
};

class LDDumpState {
    std::unordered_map<ld_::LDDPtrHolder, U8StringView> type_mapping_;
public:
    LDDumpState(const LDDocumentView& doc);

    Optional<U8StringView> resolve_type_id(ld_::LDDPtrHolder ptr) const
    {
        auto ii = type_mapping_.find(ptr);
        if (ii != type_mapping_.end()) {
            return ii->second;
        }

        return Optional<U8StringView>{};
    }
};

class SDNStringEscaper {
    ArenaBuffer<U8StringView::value_type> buffer_;
public:

    bool has_quotes(U8StringView str) const noexcept
    {
        for (auto& ch: str) {
            if (ch == '\'') return true;
        }

        return false;
    }

    U8StringView escape_quotes(const U8StringView& str)
    {
        if (!has_quotes(str)) {
            return str;
        }
        else {
            buffer_.clear();

            for (auto& ch: str)
            {
                if (MMA1_UNLIKELY(ch == '\''))
                {
                    buffer_.append_value('\\');
                }

                buffer_.append_value(ch);
            }

            buffer_.append_value(0);

            return U8StringView(buffer_.data(), buffer_.size() - 1);
        }
    }

    void reset()
    {
        if (buffer_.size() <= 1024*16) {
            buffer_.clear();
        }
        else {
            buffer_.reset();
        }
    }

    static SDNStringEscaper& current();
};


template <>
struct DataTypeTraits<LDStringView>: DataTypeTraitsBase<LDStringView> {
    using ViewType      = U8StringView;
    using ConstViewType = ViewType;
    using AtomType      = std::remove_const_t<typename ViewType::value_type>;
    using LDStorageType = U8LinkedString;

    using LDViewType    = LDStringView;

    //using DatumStorage  = VarcharStorage;

    static constexpr bool isDataType          = true;
    static constexpr bool HasTypeConstructors = false;

    static constexpr bool isSdnDeserializable = true;

    static void create_signature(SBuf& buf, const LDStringView& obj)
    {
        buf << "LDStringView";
    }

    static void create_signature(SBuf& buf)
    {
        buf << "LDStringView";
    }


    using DataSpan = Span<const AtomType>;
    using SpanList = TL<DataSpan>;
    using SpanTuple = AsTuple<SpanList>;

    using DataDimensionsList  = TL<DataSpan>;
    using DataDimensionsTuple = AsTuple<DataDimensionsList>;

    using TypeDimensionsList  = TL<>;
    using TypeDimensionsTuple = AsTuple<TypeDimensionsList>;

    static DataDimensionsTuple describe_data(ViewType view) {
        return std::make_tuple(DataSpan(view.data(), view.size()));
    }

    static DataDimensionsTuple describe_data(const ViewType* view) {
        return std::make_tuple(DataSpan(view->data(), view->size()));
    }


    static TypeDimensionsTuple describe_type(ViewType view) {
        return std::make_tuple();
    }

    static TypeDimensionsTuple describe_type(const Varchar& data_type) {
        return TypeDimensionsTuple{};
    }


    static ViewType make_view(const DataDimensionsTuple& data)
    {
        return ViewType(std::get<0>(data).data(), std::get<0>(data).size());
    }

    static ViewType make_view(const TypeDimensionsTuple& type, const DataDimensionsTuple& data)
    {
        return ViewType(std::get<0>(data).data(), std::get<0>(data).size());
    }
};

template <typename T>
constexpr const T* make_null_v() {
    return nullptr;
}


template <typename T, typename DocT, typename... Args>
DTTLDViewType<T> make_ld_view(const T*, const DocT* doc, Args&&... args)
{
    return DTTLDViewType<T>(doc, std::forward<Args>(args)...);
}



template <typename T, typename Arena, typename... Args>
auto ld_allocate_and_construct(const T*, Arena* arena, Args&&... args)
{
    using LDStorageType = DTTLDStorageType<T>;

    return allocate_tagged<LDStorageType>(
        ld_tag_size<T>(), arena, std::forward<Args>(args)...
    );
}

//template <typename T, typename Arena, typename... Args>
//auto ld_create_storage(const T* dt_tag, Arena* arena, Args&&... args)
//{
//    using LDViewType = DTTLDViewType<T>;

//    auto value_ptr = ld_allocate_and_construct<T>(
//        dt_tag, arena, std::forward<Args>(args)...
//    );

//    ld_::ldd_set_tag(value_ptr.get(), ld_tag_value<T>());
//    return LDViewType{this, value_ptr, ld_tag_value<T>()};
//}




}}
