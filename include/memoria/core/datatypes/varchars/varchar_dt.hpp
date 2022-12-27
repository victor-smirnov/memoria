
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

#include <memoria/core/types.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/arena_buffer.hpp>
#include <memoria/core/strings/u8_string.hpp>
#include <memoria/core/strings/u8_string_view.hpp>
#include <memoria/core/datatypes/core.hpp>
#include <memoria/core/datatypes/traits.hpp>

#include <tuple>

namespace memoria {

class VarcharStorage;
using VarcharView = U8StringView;


template <typename CharT> class LinkedString;


template <>
struct DataTypeTraits<Varchar>: DataTypeTraitsBase<Varchar>
{
    using ViewType      = VarcharView;
    using ConstViewType = VarcharView;
    using AtomType      = std::remove_const_t<typename VarcharView::value_type>;

    using View2Type     = Own<U8StringOView, OwningKind::HOLDING>;


    using DatumStorage  = VarcharStorage;

    using SharedPtrT = Own<ViewType>;
    using ConstSharedPtrT = DTConstSharedPtr<ViewType>;

    using SpanT      = DTViewSpan<ViewType, SharedPtrT>;
    using ConstSpanT = DTConstViewSpan<ViewType, ConstSharedPtrT>;

    using SpanStorageT = U8StringView;
    using OSpanT       = OSpan<View2Type, SpanStorageT>;

    static constexpr bool isDataType          = true;
    static constexpr bool HasTypeConstructors = false;

    static constexpr bool isSdnDeserializable = true;

    static void create_signature(SBuf& buf, const Varchar& obj) {
        buf << "Varchar";
    }

    static void create_signature(SBuf& buf) {
        buf << "Varchar";
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

template <typename Selector>
struct DatatypeComparator<Varchar, Selector> {
    static int32_t compare(const DTTViewType<Varchar>& left, const DTTViewType<Varchar>& right) {
        return left.compare(right);
    }
};

template <typename Selector>
struct DatatypeEqualityComparator<Varchar, Selector> {
    static bool equals(const DTTViewType<Varchar>& left, const DTTViewType<Varchar>& right) {
        return left == right;
    }
};

template <typename Selector>
struct CrossDatatypeEqualityComparator<Varchar, Boolean, Selector> {
    static bool equals(const DTTViewType<Varchar>& left, const DTTViewType<Boolean>& right) noexcept {
        bool is_true = left == "true";
        return right == is_true;
    }
};

template <typename Selector>
struct CrossDatatypeEqualityComparator<Boolean, Varchar, Selector> {
    static bool equals(const DTTViewType<Boolean>& left, const DTTViewType<Varchar>& right) noexcept {
        bool is_true = right == "true";
        return left == is_true;
    }
};

}
