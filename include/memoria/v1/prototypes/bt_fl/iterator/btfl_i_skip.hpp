
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/types/algo/for_each.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(v1::btfl::IteratorSkipName)

    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;
    using typename Base::Container;

    using typename Base::LeafDispatcher;

    template <Int StreamIdx>
    using LeafSizesSubstreamPath = typename Container::Types::template LeafSizesSubstreamPath<StreamIdx>;

    template <typename LeafPath>
    using AccumItemH = typename Container::Types::template AccumItemH<LeafPath>;

    static const Int Streams                = Container::Types::Streams;
    static const Int SearchableStreams      = Container::Types::SearchableStreams;

    using LeafPrefixRanks = typename Container::Types::LeafPrefixRanks;

public:

    CtrSizesT path() const
    {
        auto& self = this->self();
        CtrSizesT p = self.cache().data_pos();

        for (Int s = self.stream() + 1; s < Streams; s++) {
            p[s] = -1;
        }

        return p;
    }


    bool next() {
        return self().skipFw(1) > 0;
    }

    bool prev() {
    	auto distance = self().skipBw(1);

        return distance > 0;
    }

    struct SkipFwFn {
        template <Int StreamIdx, typename Itr>
        auto process(Itr&& iter, CtrSizeT n)
        {
            return iter.template skip_fw_<StreamIdx>(n);
        }
    };

    struct SkipBwFn {
        template <Int StreamIdx, typename Itr>
        auto process(Itr&& iter, CtrSizeT n)
        {
            return iter.template skip_bw_<StreamIdx>(n);
        }
    };


    CtrSizeT skipFw(CtrSizeT n)
    {
        auto& self = this->self();
        auto& cache = self.cache();

        auto stream = self.stream();

        auto pos  = cache.data_pos()[stream];
        auto size = cache.data_size()[stream];

        CtrSizeT nn;

        if (pos + n > size) {
            nn = size - pos;
        }
        else {
            nn = n;
        }

        return bt::ForEachStream<Streams - 1>::process(stream, SkipFwFn(), self, nn);
    }


    CtrSizeT skipBw(CtrSizeT n)
    {
        auto& self = this->self();
        auto& cache = self.cache();

        auto stream = self.stream();

        auto pos  = cache.data_pos()[stream];

        if (pos - n < 0) {
            n = pos;
        }

        return self.skipBw1(n);
    }

    CtrSizeT skipBw1(CtrSizeT n = 1)
    {
        auto& self = this->self();
        auto stream = self.stream();
        return bt::ForEachStream<Streams - 1>::process(stream, SkipBwFn(), self, n);
    }

    CtrSizeT skip(CtrSizeT n)
    {
        if (n > 0) {
            return skipFw(n);
        }
        else {
            return skipBw(n);
        }
    }

    CtrSizeT size() const {

    }

    CtrSizeT substream_size() const
    {

    }

    CtrSizeT pos() const {

    }


    CtrSizeT toData(CtrSizeT offset = 0)
    {

    }



    void toIndex()
    {
    }

    bool isSEnd() const
    {

    }















public:


MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(v1::btfl::IteratorSkipName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
