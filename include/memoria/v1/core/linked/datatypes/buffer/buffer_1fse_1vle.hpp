
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

#include <memoria/v1/core/linked/datatypes/buffer/buffer_common.hpp>

namespace memoria {
namespace v1 {

template <typename DataTypeT, typename TypePart, typename AtomTypeT>
class DataTypeBuffer<
        DataTypeT,
        SparseObjectAdapterDescriptor<
            false,
            TL<TypePart>,
            TL<Span<AtomTypeT>>
        >
> {
    using AtomType = std::decay_t<AtomTypeT>;

    using ViewType  = DTTViewType<DataTypeT>;

    ArenaBuffer<ViewType> views_;
    ArenaBuffer<AtomType> data_;

    using DataSpan = Span<const AtomType>;

    using Builder = SparseObjectBuilder<DataTypeT, DataTypeBuffer>;

    Builder builder_;

    using SOAdapter = DataTypeTraits<DataTypeT>;

public:
    template <typename, typename>
    friend class SparseObjectBuilder;

    using DataType = DataTypeT;

    DataTypeBuffer(DataType data_type = DataType()):
        builder_(this)
    {}

    Builder& builder() {
        return builder_;
    }

    void clear() {
        views_.clear();
        data_.clear();
    }

    void reset() {
        views_.reset();
        data_.reset();
    }

    size_t size() const {
        return views_.size();
    }

    bool emplace_back(const ViewType& value)
    {
        check_builder_is_empty();
        return emplace_back_nockeck(value);
    }

    bool append(const ViewType& value) {
        return emplace_back(value);
    }

    template <typename ValueType>
    bool append(Span<const ValueType> span)
    {
        check_builder_is_empty();

        bool resized = false;
        for (const ValueType& value: span) {
            resized = emplace_back_nockeck(value) || resized;
        }
        return resized;
    }

    const ViewType& operator[](size_t idx) const {
        return views_[idx];
    }

    Span<const ViewType> span() const {
        return views_.span();
    }

    Span<const ViewType> span(size_t from) const
    {
        return views_.span(from);
    }

    Span<const ViewType> span(size_t from, size_t length) const
    {
        return views_.span(from, length);
    }

    AtomType* data() {
        return data_.data();
    }

    const AtomType* data() const {
        return data_.data();
    }

protected:
    void add_view(size_t offset, size_t size)
    {
        views_.append_value(make_view(data_.data() + offset, size));
        data_.add_size(size);
    }

    bool reserve(size_t size)
    {
        return data_.ensure(size);
    }

private:
    bool emplace_back_nockeck(const ViewType& value)
    {
        auto span = SOAdapter::describe_data(value);
        size_t size = std::get<0>(span).size();

        bool data_resized = data_.ensure(size);

        auto ptr  = data_.top();
        bool resized = views_.emplace_back(
                SOAdapter::make_view(std::make_tuple(DataSpan(ptr, size)))
        );

        if (data_.append_values(std::get<0>(span)) || data_resized)
        {
            refresh_views();
        }

        return resized;
    }


    static ViewType make_view(const AtomType* data, size_t size) {
        return SOAdapter::make_view(std::make_tuple(DataSpan(data, size)));
    }

    void refresh_views()
    {
        size_t offset{};
        AtomType* data = data_.data();
        for (size_t c = 0; c < views_.size(); c++)
        {
            auto span_tpl = SOAdapter::describe_data(views_[c]);
            auto& span = std::get<0>(span_tpl);

            views_[c] = make_view(data + offset, span.size());
            offset += span.size();
        }
    }

    void check_builder_is_empty() const {
        if (!builder_.is_empty()) {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Builder is not empty");
        }
    }
};


}}
