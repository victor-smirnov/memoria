
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

#include <memoria/api/common/ctr_api.hpp>

#include <memoria/core/types/algo/for_each.hpp>

#include <memoria/core/hermes/hermes.hpp>
#include <memoria/core/datatypes/dt_span.hpp>

#include <tuple>

namespace memoria {

namespace detail {

template <size_t Idx>
struct BISubstreamHelper {
    template <typename Tuple>
    static auto& get(Tuple& tuple) noexcept {
        return std::get<Idx - 1>(tuple);
    }
};

template <typename T>
struct BIStreamSize {
    T value;
    T size() const noexcept {
        return value;
    }
};

template <>
struct BISubstreamHelper<0> {
    template <typename Tuple>
    static auto get(Tuple& tuple) noexcept
    {
        auto size = std::get<0>(tuple).size();
        return BIStreamSize<decltype(size)>{size};
    }
};

}

template <typename StreamsList>
class CtrBatchInputBase {
public:
    using Streams = AsTuple<StreamsList>;
protected:
    Streams streams_;
public:
    CtrBatchInputBase() {}
    virtual ~CtrBatchInputBase() noexcept = default;

    Streams& streams() {return streams_;}
    const Streams& streams() const {return streams_;}

    template <size_t Stream, size_t Substream>
    decltype(auto) get() noexcept {
        return detail::BISubstreamHelper<Substream>::get(std::get<Stream>(streams_));
    }

    template <size_t Stream, size_t Substream>
    decltype(auto) get() const noexcept {
        return detail::BISubstreamHelper<Substream>::get(std::get<Stream>(streams_));
    }

    virtual void check() const
    {
        for_each_stream([&](auto stream_idx){
            auto size0 = std::get<0>(std::get<stream_idx>(streams_)).size();

            for_each_substream1<stream_idx>([&](auto, auto substream_idx){
                const auto& substream = std::get<substream_idx>(std::get<stream_idx>(streams_));

                if (substream.size() != size0) {
                    MEMORIA_MAKE_GENERIC_ERROR(
                        "Substream size check failure: expected: {}, actual: {}, stream: {}, substream {}",
                        size0,
                        substream.size(),
                        stream_idx,
                        substream_idx
                    ).do_throw();
                }
            });

        });
    }

    virtual void clear()
    {
        for_each_stream([&](auto stream_idx){
            for_each_substream<stream_idx>([&](auto substream_idx){
                auto& substream = std::get<substream_idx>(std::get<stream_idx>(streams_));
                substream.clear();
            });
        });
    }

    virtual void reset()
    {
        for_each_stream([&](auto stream_idx){
            for_each_substream<stream_idx>([&](auto substream_idx){
                auto& substream = std::get<substream_idx>(std::get<stream_idx>(streams_));
                substream.reset();
            });
        });
    }


    virtual void reindex()
    {
        for_each_stream([&](auto stream_idx){
            for_each_substream<stream_idx>([&](auto substream_idx){
                auto& substream = std::get<substream_idx>(std::get<stream_idx>(streams_));
                substream.reindex();
            });
        });
    }

    void reset_state() {
        clear();
    }


    template <typename Fn, typename... Args>
    void for_each_stream(Fn&& fn, Args&&... args) {
        ForEach<0, std::tuple_size_v<Streams>>::process_fn(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    void for_each_stream(Fn&& fn, Args&&... args) const {
        ForEach<0, std::tuple_size_v<Streams>>::process_fn(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <size_t Idx, typename Fn, typename... Args>
    void for_each_substream(Fn&& fn, Args&&... args) {
        ForEach<
            0,
            std::tuple_size_v<std::tuple_element_t<Idx, Streams>>
        >::process_fn(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <size_t Idx, typename Fn, typename... Args>
    void for_each_substream1(Fn&& fn, Args&&... args) {
        ForEach<
            1,
            std::tuple_size_v<std::tuple_element_t<Idx, Streams>>
        >::process_fn(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <size_t Idx, typename Fn, typename... Args>
    void for_each_substream(Fn&& fn, Args&&... args) const {
        ForEach<
            0,
            std::tuple_size_v<std::tuple_element_t<Idx, Streams>>
        >::process_fn(std::forward<Fn>(fn), ForEachIdx<Idx>{}, std::forward<Args>(args)...);
    }

    template <size_t Idx, typename Fn, typename... Args>
    void for_each_substream1(Fn&& fn, Args&&... args) const {
        ForEach<
            1,
            std::tuple_size_v<std::tuple_element_t<Idx, Streams>>
        >::process_fn(std::forward<Fn>(fn), ForEachIdx<Idx>{}, std::forward<Args>(args)...);
    }
};

template <typename Streams>
void check_ctr_batch_input(const CtrBatchInputBase<Streams>& input) {
    input.check();
}

template <typename T>
void check_ctr_batch_input(const T& input) {}


template <typename Streams>
void clear_ctr_batch_input(CtrBatchInputBase<Streams>& input) {
    input.clear();
}

template <typename Streams>
void reset_ctr_batch_input(CtrBatchInputBase<Streams>& input) {
    input.reset();
}

template <typename Streams>
void reindex_ctr_batch_input(CtrBatchInputBase<Streams>& input) {
    input.reindex();
}

template <size_t StreamIdx, size_t SubstreamIdx, typename Streams>
decltype(auto) get_ctr_batch_input_substream(const CtrBatchInputBase<Streams>& input) {
    return input.template get<StreamIdx, SubstreamIdx>();
}


template <typename BufferT>
using CtrBatchInputFn = std::function<bool (BufferT&)>;

template <typename DT, typename Category = typename DataTypeTraits<DT>::DatumSelector>
class HermesDTBuffer;

template <typename DT>
class HermesDTBuffer<DT, FixedSizeDataTypeTag> {
    hermes::HermesCtr ctr_;
    hermes::Array<DT> array_;

    using DataLengths = std::tuple<size_t>;

public:
    HermesDTBuffer():
        ctr_(hermes::HermesCtr::make_pooled())
    {
        array_ = ctr_.make_array<DT>();
    }

    auto get(size_t c) const {
        return array_.get(c);
    }

    uint64_t size() const {
        return array_.size();
    }

    void clear() {
        ctr_ = hermes::HermesCtr::make_pooled();
        array_ = ctr_.make_array<DT>();
    }

    void reset() {
        clear();
    }

    void reindex() {}

    DataLengths data_lengths(size_t start, size_t size) const {
        return size * sizeof(DTSpanStorage<DT>);
    }


    void append(const DTTViewType<DT>& value) {
        array_ = array_.push_back(value);
    }

    template <typename T>
    void append(Span<const T> value) {
        for (auto& vv: value) {
            array_ = array_.push_back(vv);
        }
    }

    template <typename T>
    void append(const std::vector<T>& value) {
        for (auto& vv: value) {
            array_ = array_.push_back(vv);
        }
    }

    template <typename T, typename S>
    void append(OSpan<T, S> value) {
        for (auto& vv: value) {
            array_ = array_.push_back(vv.value_t());
        }
    }
};


template <typename DT>
class HermesDTBuffer<DT, EmptyType> {
    hermes::HermesCtr ctr_;
    hermes::ObjectArray array_;

    using DataLengths = std::tuple<size_t>;
public:
    HermesDTBuffer():
        ctr_(hermes::HermesCtr::make_pooled())
    {
        array_ = ctr_.make_object_array();
    }

    auto get(size_t c) const {
        return array_.get(c).convert_to<DT>().template cast_to<DT>();
    }

    uint64_t size() const {
        return array_.size();
    }

    void clear() {
        ctr_ = hermes::HermesCtr::make_pooled();
        array_ = ctr_.make_object_array();
    }

    void reset() {
        clear();
    }

    void reindex() {}

    DataLengths data_lengths(size_t start, size_t size) const
    {
        size_t total{};
        for (size_t c = start; c < start + size; c++) {
            auto obj = array_.get(c);
            auto view = obj.convert_to<DT>().template cast_to<DT>();
            auto descr = DataTypeTraits<DT>::describe_data(view);
            total += std::get<0>(descr).size();
        }

        return total;
    }

    void append(const DTTViewType<DT>& value) {
        array_ = array_.push_back(value);
    }

    template <typename T>
    void append(Span<const T> value) {
        for (auto& vv: value) {
            array_ = array_.push_back(vv);
        }
    }

    template <typename T>
    void append(const std::vector<T>& value) {
        for (auto& vv: value) {
            array_ = array_.push_back(vv);
        }
    }

    template <typename T, typename S>
    void append(OSpan<T, S> value) {
        for (auto& vv: value) {
            array_ = array_.push_back(vv);
        }
    }
};

template <typename DT>
void clear_ctr_batch_input(HermesDTBuffer<DT>& input) {
    input.clear();
}


template <typename DT>
void reindex_ctr_batch_input(HermesDTBuffer<DT>& input) {
    input.reindex();
}

}
