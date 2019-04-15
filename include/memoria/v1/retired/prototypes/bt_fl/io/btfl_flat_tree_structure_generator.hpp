
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/v1/core/packed/sseq/packed_rle_searchable_seq.hpp>

#include <memoria/v1/retired/prototypes/bt/layouts/bt_input.hpp>
#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>

#include <memoria/v1/retired/prototypes/bt_fl/io/btfl_data_input.hpp>
#include <memoria/v1/retired/prototypes/bt_fl/io/btfl_structure_input.hpp>
#include <memoria/v1/retired/prototypes/bt_fl/io/btfl_rank_dictionary.hpp>

#include <memory>

namespace memoria {
namespace v1 {
namespace btfl {
namespace io {


namespace {

    template <int32_t Size, int32_t Idx = 0>
    struct StreamSizeAdapter {
        template <typename T, typename... Args>
        static auto process(int32_t stream, T&& object, Args&&... args)
        {
            if (stream == Idx)
            {
                return object.stream_size(bt::StreamTag<Idx>(), std::forward<Args>(args)...);
            }
            else {
                return StreamSizeAdapter<Size, Idx+1>::process(stream, std::forward<T>(object), std::forward<Args>(args)...);
            }
        }
    };


    template <int32_t Size>
    struct StreamSizeAdapter<Size, Size> {
        template <typename T, typename... Args>
        static auto process(int32_t stream, T&& object, Args&&... args)
        {
            if (stream == Size)
            {
                return object.stream_size(bt::StreamTag<Size>(), std::forward<Args>(args)...);
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Invalid stream number: {}", stream));
            }
        }
    };
}






template <typename MyType, int32_t Streams>
class FlatTreeStructureGeneratorBase {

public:
    using CtrSizeT = int64_t;

    using CtrSizesT = core::StaticVector<CtrSizeT, Streams>;

private:

    CtrSizesT counts_;
    CtrSizesT current_limits_;
    CtrSizesT totals_;

    int32_t level_;

    template <int32_t, int32_t> friend struct StreamSizeAdapter;

public:
    FlatTreeStructureGeneratorBase(int32_t level = 0):
        level_(level)
    {}

    auto query()
    {
        if (counts_[level_] < current_limits_[level_])
        {
            if (level_ < Streams - 1)
            {
                level_++;

                counts_[level_] = 0;
                current_limits_[level_] = StreamSizeAdapter<Streams - 1>::process(level_, *this, counts_);

                return btfl::io::RunDescr(level_ - 1, 1);
            }
            else {
                return btfl::io::RunDescr(level_, current_limits_[level_] - counts_[level_]);
            }
        }
        else if (level_ > 0)
        {
            level_--;
            return this->query();
        }
        else {
            return btfl::io::RunDescr(-1);
        }
    }

    const auto& consumed() const {
        return totals_;
    }

    auto& consumed() {
        return totals_;
    }

    const auto& counts() const {
        return counts_;
    }

    auto& counts() {
        return counts_;
    }

    void commit(int32_t level, CtrSizeT len)
    {
        counts_[level] += len;
    }

    const auto& current_limits() const {
        return current_limits_;
    }

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<MyType*>(this);}

    void init() {
        current_limits_[level_] = StreamSizeAdapter<Streams - 1>::process(level_, *this, counts_);
    }

private:

    auto stream_size(bt::StreamTag<0>, const CtrSizesT& pos)
    {
        return self().prepare(bt::StreamTag<0>());
    }

    template <int32_t StreamIdx>
    auto stream_size(bt::StreamTag<StreamIdx>, const CtrSizesT& pos)
    {
        return self().prepare(bt::StreamTag<StreamIdx>(), pos);
    }
};


}}}}
