
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

#include <unordered_map>
#include <iostream>

namespace memoria {
namespace v1 {

using LDDValueTag = uint16_t;
struct LDDInvalidCastException: RuntimeException {};

struct LDDocumentHeader {
    uint32_t version;
};


namespace ld_ {
    using LDArena     = LinkedArena<LDDocumentHeader, uint32_t>;
    using LDArenaView = LinkedArenaView<LDDocumentHeader, uint32_t>;
//    using LDArenaAddressMapping = LDArenaView::AddressMapping;

    using LDDPtrHolder = LDArenaView::PtrHolderT;

    template <typename T>
    using LDPtr = typename LDArenaView::template PtrT<T>;

    template <typename T>
    using LDGenericPtr = typename LDArenaView::template GenericPtrT<T>;

    class LDArenaAddressMapping {
        typename LDArenaView::AddressMapping mapping_;
    public:
        LDArenaAddressMapping() {}

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
    };
}


class LDDArray;

class LDDValue;
class LDDMap;
class LDTypeDeclaration;
class LDTypeName;
class LDDataTypeParam;
class LDDataTypeCtrArg;
class LDDocument;
class LDDocumentView;
class LDString;
class LDIdentifier;
class LDDTypedValue;
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



    static inline void ldd_set_tag(LDArenaView* arena, PtrHolder ptr, LDDValueTag tag)
    {
        *T2T<LDDValueTag*>(arena->data() + ptr - sizeof(tag)) = tag;
    }

    static inline LDDValueTag ldd_get_tag(const LDArenaView* arena, PtrHolder ptr) {
        return *T2T<const LDDValueTag*>(arena->data() + ptr - sizeof(LDDValueTag));
    }


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
        if (tag != LDDValueTraits<Type>::ValueTag) {
            MMA1_THROW(LDDInvalidCastException());
        }
    }

    template <typename Type>
    void ldd_assert_tag(LDDValueTag tag)
    {
        if (tag != LDDValueTraits<Type>::ValueTag) {
            MMA1_THROW(LDDInvalidCastException());
        }
    }
}





template <>
struct LDDValueTraits<LDString> {
    static constexpr LDDValueTag ValueTag = 1;
};

template <>
struct LDDValueTraits<LDInteger> {
    static constexpr LDDValueTag ValueTag = 2;
};

template <>
struct LDDValueTraits<LDDouble> {
    static constexpr LDDValueTag ValueTag = 3;
};

template <>
struct LDDValueTraits<LDDMap> {
    static constexpr LDDValueTag ValueTag = 5;
};

template <>
struct LDDValueTraits<LDDArray> {
    static constexpr LDDValueTag ValueTag = 6;
};

template <>
struct LDDValueTraits<LDTypeDeclaration> {
    static constexpr LDDValueTag ValueTag = 7;
};

template <>
struct LDDValueTraits<LDIdentifier> {
    static constexpr LDDValueTag ValueTag = LDDValueTraits<LDString>::ValueTag;
};

template <>
struct LDDValueTraits<LDDTypedValue> {
    static constexpr LDDValueTag ValueTag = 9;
};

template <>
struct LDDValueTraits<LDBoolean> {
    static constexpr LDDValueTag ValueTag = 10;
};




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

}}
