
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

#include <memoria/core/datatypes/buffer/buffer_common.hpp>

#include <memoria/core/types/mp11.hpp>
#include <memoria/core/types/algo.hpp>

#include <memoria/core/iovector/io_substream_base.hpp>

namespace memoria {

template <typename DataTypeT, typename... TypeParts, typename... DataParts >
class DataTypeBuffer<
        DataTypeT,
        SparseObjectAdapterDescriptor<
            false,
            TL<TypeParts...>,
            TL<DataParts...>
        >
>: public io::IOSubstream
{
    using TypeDimensionsTuple = AsTuple<TL<TypeParts...>>;
    using DataDimensionsTuple = typename DataTypeTraits<DataTypeT>::DataDimensionsTuple;

    using ViewType = DTTViewType<DataTypeT>;

    using SizeT = size_t;

    template <typename T>
    using DimensionTypeToBuffer = detail::DataTypeBufferDimension<T, SizeT>;

    using DataBuffersTuple = boost::mp11::mp_transform<DimensionTypeToBuffer, DataDimensionsTuple>;

    template <typename T>
    using DimensionTypeToSizeT = SizeT;

    using DataSizesTuple = boost::mp11::mp_transform<DimensionTypeToSizeT, DataDimensionsTuple>;

    ArenaBuffer<ViewType, SizeT> views_;
    DataBuffersTuple data_buffers_;

    using BuilderT = SparseObjectBuilder<DataTypeT, DataTypeBuffer>;

    TypeDimensionsTuple type_data_;
    BuilderT builder_;

    mutable detail::LifetimeGuardShared* lg_shared_{};

    using SOAdapter = DataTypeTraits<DataTypeT>;

public:
    template <typename, typename>
    friend class SparseObjectBuilder;

    using DataType = DataTypeT;
    using Builder  = BuilderT;

    DataTypeBuffer(DataType data_type = DataType()):
        type_data_(SOAdapter::describe_type(data_type)),
        builder_(this)
    {
        install_invalidation_listener();
    }

    DataTypeBuffer(const TypeDimensionsTuple& type_data):
        type_data_(type_data),
        builder_(this)
    {
        install_invalidation_listener();
    }

    ~DataTypeBuffer() noexcept {
        invalidate_guard();
    }

    LifetimeGuard buffer_guard() const noexcept {
        if (!lg_shared_) {
            lg_shared_ = new detail::DefaultLifetimeGuardSharedImpl([&]{
               lg_shared_ = nullptr;
            });
        }
        return LifetimeGuard(lg_shared_);
    }


    LifetimeGuard data_guard() const noexcept {
        return views_.guard();
    }

    GuardedView<ViewType> get_guarded(size_t idx) noexcept {
        return views_.get_guarded(idx);
    }

    GuardedView<const ViewType> get_guarded(size_t idx) const noexcept {
        return views_.get_guarded(idx);
    }


    virtual void reindex() {}

    virtual U8String describe() const {
        return U8String("DataTypeBuffer<") + TypeNameFactory<DataType>::name().to_u8() + ">";
    }

    virtual const std::type_info& substream_type() const {
        return typeid(DataTypeBuffer);
    }

    void sort() noexcept {
        views_.sort();
    }

    Builder& builder() {
        return builder_;
    }


    void clear()
    {
        views_.clear();
        for_eatch_data_buffer([&](auto idx) {
            std::get<idx>(data_buffers_).clear();
        });
    }

    void reset() {
        views_.reset();
        for_eatch_data_buffer([&](auto idx) {
            std::get<idx>(data_buffers_).reset();
        });
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

    template <typename ValueType>
    bool append(const std::vector<ValueType>& data)
    {
        check_builder_is_empty();

        bool resized = false;
        for (const ValueType& value: data) {
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

    template <int32_t Dimension>
    size_t data_length(size_t start, size_t size) const
    {
        return std::get<Dimension>(data_buffers_).data_length(start, size);
    }

    DataSizesTuple data_lengths(size_t start, psize_t size) const
    {
        DataSizesTuple tuple{};

        for_eatch_data_buffer([&](auto idx){
            std::get<idx>(tuple) = std::get<idx>(data_buffers_).data_length(start, size);
        });

        return tuple;
    }


    template <int32_t Dimension>
    const auto* data(size_t start) const {
        return std::get<Dimension>(data_buffers_).data(start);
    }

    template <int32_t Dimension>
    const auto* offsets(size_t start) const {
        return std::get<Dimension>(data_buffers_).offsets(start);
    }



    void read_to(size_t start, size_t size, ArenaBuffer<ViewType>& buffer) const {
        buffer.append_values(views_.span(start, size));
    }

    const ViewType& get(size_t idx) const {
        return views_[idx];
    }

protected:

    DataBuffersTuple& buffers() {
        return data_buffers_;
    }

    const DataBuffersTuple& buffers() const {
        return data_buffers_;
    }

    void add_view(const DataDimensionsTuple& dimensions)
    {
        views_.append_value(SOAdapter::make_view(type_data_, dimensions));

        for_eatch_data_buffer([&](auto idx){
            std::get<idx>(data_buffers_).append_top(
                std::get<idx>(dimensions)
            );
        });
    }

    template <size_t Idx>
    bool reserve(size_t size)
    {
        return std::get<Idx>(data_buffers_).buffer().ensure(size);
    }

private:

    bool emplace_back_nockeck(const ViewType& view)
    {
        DataDimensionsTuple data = SOAdapter::describe_data(view);

        bool refresh_views{};

        for_eatch_data_buffer([&](auto idx){
            refresh_views = std::get<idx>(data_buffers_).emplace_back_nockeck_tool(
                std::get<idx>(data)
            ) || refresh_views;
        });

        bool views_resized = views_.emplace_back(SOAdapter::make_view(type_data_, data));

        if (views_resized || refresh_views)
        {
            this->refresh_views();
            return true;
        }

        return false;
    }


    void refresh_views()
    {
        for_eatch_data_buffer([&](auto idx){
            std::get<idx>(data_buffers_).reset_iterator();
        });

        SizeT views_size = views_.size();
        views_.clear();

        for (SizeT c = 0; c < views_size; c++)
        {
            DataDimensionsTuple tuple;
            for_eatch_data_buffer([&](auto idx){
                std::get<idx>(data_buffers_).next(std::get<idx>(tuple));
            });

            views_.append_value(SOAdapter::make_view(type_data_, tuple));
        }
    }

    void check_builder_is_empty() const {
//        if (!builder_.is_empty()) {
//            MMA_THROW(RuntimeException()) << WhatCInfo("Builder is not empty");
//        }
    }

    template <typename Fn, typename... Args>
    static void for_eatch_data_buffer(Fn&& fn, Args&&... args) {
        ForEach<0, std::tuple_size<DataBuffersTuple>::value>::process_fn(
            std::forward<Fn>(fn),
            std::forward<Args>(args)...
        );
    }

    void install_invalidation_listener() noexcept {
        auto listener = [&]{
            refresh_views();
        };

        for_eatch_data_buffer([&](auto idx) {
            std::get<idx>(data_buffers_).set_invalidation_listener(listener);
        });
    }

    void invalidate_guard() const noexcept {
        if (lg_shared_) {
            lg_shared_->invalidate();
            lg_shared_ = nullptr;
        }
    }
};


}
