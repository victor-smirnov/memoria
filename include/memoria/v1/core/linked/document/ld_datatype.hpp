
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

#include <memoria/v1/core/linked/document/linked_document.hpp>
#include <memoria/v1/core/linked/datatypes/traits.hpp>

#include <memoria/v1/core/linked/datatypes/datum.hpp>

namespace memoria {
namespace v1 {

class LDDocumentStorage;

template <>
struct DataTypeTraits<LDDocument>: DataTypeTraitsBase<LDDocument>
{
    using ViewType      = LDDocumentView;
    using ConstViewType = LDDocumentView;
    using AtomType      = std::remove_const_t<typename LDDocumentView::AtomType>;

    using DatumStorage  = LDDocumentStorage;

    static constexpr bool isDataType          = true;
    static constexpr bool HasTypeConstructors = false;

    static constexpr bool isSdnDeserializable = true;

    static void create_signature(SBuf& buf, const LDDocument& obj)
    {
        buf << "LDDocument";
    }

    static void create_signature(SBuf& buf)
    {
        buf << "LDDocument";
    }


    using DataSpan = Span<const AtomType>;
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

    static TypeDimensionsTuple describe_type(const LDDocument& data_type) {
        return TypeDimensionsTuple{};
    }


    static ViewType make_view(const DataDimensionsTuple& data)
    {
        return ViewType(std::get<0>(data));
    }

    static ViewType make_view(const TypeDimensionsTuple& type, const DataDimensionsTuple& data)
    {
        return ViewType(std::get<0>(data));
    }
};



template <typename Buffer>
class SparseObjectBuilder<LDDocument, Buffer> {
    Buffer* buffer_;


    using AtomType = DTTAtomType<LDDocument>;
    using ViewType = DTTViewType<LDDocument>;

    LDDocument doc_;

public:
    SparseObjectBuilder(Buffer* buffer):
        buffer_(buffer)
    {}

    SparseObjectBuilder(SparseObjectBuilder&&) = delete;
    SparseObjectBuilder(const SparseObjectBuilder&) = delete;

    LDDocumentView view() {
        return doc_;
    }

    LDDocument& doc() {
        return doc_;
    }

    void build()
    {
        doc_ = doc_.compactify();
        buffer_->append(doc_);
        doc_.clear();
    }
};

class LDDocumentStorage: public DatumStorageBase<LDDocument, typename DataTypeTraits<LDDocument>::DatumStorageSelector> {
    using SelectorTag = typename DataTypeTraits<LDDocument>::DatumStorageSelector;

    using Base = DatumStorageBase<LDDocument, SelectorTag>;
    using typename Base::ViewType;
public:
    LDDocumentStorage(ViewType view) noexcept: Base(view) {}

    virtual void destroy() noexcept;
    static LDDocumentStorage* create(ViewType view);
    virtual U8String to_sdn_string() const;
};

}}
