
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/core/container/container.hpp>



namespace memoria {
namespace v1 {
namespace table   {





template <typename CtrT, typename Rng>
class RandomDataInputProvider: public v1::bttl::AbstractCtrInputProvider<CtrT, CtrT::Types::Streams, CtrT::Types::LeafDataLength> {

    using Base = v1::bttl::AbstractCtrInputProvider<CtrT, CtrT::Types::Streams, CtrT::Types::LeafDataLength>;

public:

    using Position      = typename CtrT::Types::Position;
    using CtrSizeT      = typename CtrT::CtrSizeT;
    using Buffer        = typename Base::Buffer;

    using RunDescr      = bttl::RunDescr;

    using NodeBaseG     = typename Base::NodeBaseG;

    template <int32_t StreamIdx>
    using InputTuple        = typename CtrT::Types::template StreamInputTuple<StreamIdx>;

    template <int32_t StreamIdx>
    using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<StreamIdx>;

private:
    CtrSizeT keys_;
    CtrSizeT columns_;
    CtrSizeT mean_data_size_;
    CtrSizeT data_size_cnt_     = 0;
    CtrSizeT data_size_limit_   = 0;

    CtrSizeT key_ = 0;
    CtrSizeT col_ = 0;

    Position sizes_;
    Position pos_;

    int32_t level_ = 0;

    Rng rng_;
public:
    RandomDataInputProvider(CtrT& ctr, CtrSizeT keys, CtrSizeT columns, CtrSizeT mean_data_size, const Rng& rng):
        Base(ctr, Position({500, 500, 500})),
        keys_(keys), columns_(columns), mean_data_size_(mean_data_size),
        rng_(rng)
    {}


    virtual RunDescr populate(const Position& pos)
    {
        if (key_ < keys_)
        {
            if (level_ == 0)
            {
                std::get<0>(this->buffer_)[pos[0]] = InputTupleAdapter<0>::convert(v1::core::StaticVector<int64_t, 1>({1}));

                col_ = 0;

                data_size_limit_ = mean_data_size_;

                key_++;
                level_ = 1;
                return RunDescr(0, 1);
            }
            else if (level_ == 1)
            {
                std::get<1>(this->buffer_)[pos[1]] = InputTupleAdapter<1>::convert(v1::core::StaticVector<int64_t, 1>({1}));

                col_++;

                if (data_size_limit_ > 0)
                {
                    data_size_cnt_ = 0;
                    level_ = 2;
                }
                else if (col_ >= columns_)
                {
                    level_ = 0;
                }

                return RunDescr(1, 1);
            }
            else {
                CtrSizeT rest = data_size_limit_ - data_size_cnt_;

                CtrSizeT capacity   = this->capacity_[2];
                CtrSizeT ppos       = pos[2];

                CtrSizeT limit;

                if (ppos + rest > capacity) {
                    limit = capacity - ppos;
                }
                else {
                    limit = rest;
                }

                for (CtrSizeT c = ppos; c < ppos + limit; c++) {
                    std::get<2>(this->buffer_)[c] = InputTupleAdapter<2>::convert(col_ % 256);
                }

                data_size_cnt_ += limit;

                if (data_size_cnt_ >= data_size_limit_)
                {
                    if (col_ < columns_)
                    {
                        level_ = 1;
                    }
                    else {
                        level_ = 0;
                    }
                }

                return RunDescr(2, limit);
            }
        }
        else {
            return RunDescr(-1);
        }
    }

};



}
}}