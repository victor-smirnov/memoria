
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

#include <memoria/api/common/ctr_batch_input.hpp>

#include <memoria/core/datatypes/buffer/ssrle_buffer.hpp>

namespace memoria {

template <typename Streams>
class FLCtrBatchInput: public CtrBatchInputBase<MergeLists<Streams, TL<TL<io::IOSSRLEBufferImpl<ListSize<Streams>>>>>> {
    using Base = CtrBatchInputBase<MergeLists<Streams, TL<TL<io::IOSSRLEBufferImpl<ListSize<Streams>>>>>>;
    static constexpr size_t Symbols = ListSize<Streams>;

public:
    FLCtrBatchInput() {}

    auto& symbols() noexcept {
        return Base::template get<Symbols, 1>();
    }

    const auto& symbols() const noexcept {
        return Base::template get<Symbols, 1>();
    }

    virtual void check()
    {
        Base::check();

        uint64_t sums[Symbols]{};
        ForEach<0, Symbols>::process_fn([&](auto stream_idx){
            auto size = Base::template get<stream_idx, 0>().size();
            sums[stream_idx] = size;
        });

        uint64_t ranks[Symbols]{};
        symbols().rank_to(symbols().size(), Span<uint64_t>(ranks, Symbols));

        for (size_t c = 0; c < Symbols; c++)
        {
            if (sums[c] != ranks[c]) {
                MEMORIA_MAKE_GENERIC_ERROR(
                    "BTFL input buffer Structure mismatch: expected {}, actual {} symbols",
                    sums[c],
                    ranks[c]
                ).do_throw();
            }
        }
    }
};

}
