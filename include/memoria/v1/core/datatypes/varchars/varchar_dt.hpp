
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

#include <memoria/v1/core/tools/span.hpp>
#include <memoria/v1/core/tools/arena_buffer.hpp>
#include <memoria/v1/core/strings/u8_string.hpp>
#include <memoria/v1/core/datatypes/core.hpp>
#include <memoria/v1/core/datatypes/traits.hpp>

#include <tuple>

namespace memoria {
namespace v1 {

class VarcharStorage;
using VarcharView = U8StringView;

class LDStringView;

template <typename CharT> class LinkedString;


template <>
struct DataTypeTraits<Varchar>: DataTypeTraitsBase<Varchar>
{
    using ViewType      = VarcharView;
    using ConstViewType = VarcharView;
    using AtomType      = std::remove_const_t<typename VarcharView::value_type>;


    using LDStorageType = LinkedString<typename U8StringView::value_type>;
    using LDViewType    = LDStringView;


    using DatumStorage  = VarcharStorage;

    static constexpr bool isDataType          = true;
    static constexpr bool HasTypeConstructors = false;

    static constexpr bool isSdnDeserializable = true;

    static void create_signature(SBuf& buf, const Varchar& obj)
    {
        buf << "Varchar";
    }

    static void create_signature(SBuf& buf)
    {
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




}}
