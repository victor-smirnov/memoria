
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

namespace memoria {
namespace v1 {

template <typename SizeT, SizeT SplitSize>
class FixedSizeSpliterator {
    SizeT size_;
    SizeT start_;
    SizeT end_;

public:
    constexpr FixedSizeSpliterator(SizeT size):
        size_(size),
        start_()
    {
        end_ = MMA1_LIKELY(size_ > SplitSize) ? SplitSize : size_;
    }

    constexpr bool has_more() const {
        return start_ < end_;
    }

    constexpr SizeT split_start() const {
        return start_;
    }

    constexpr SizeT split_end() const {
        return end_;
    }

    constexpr SizeT split_size() const {
        return end_ - start_;
    }

    constexpr void next_split()
    {
        start_ += SplitSize;
        end_   += SplitSize;

        if (MMA1_UNLIKELY(end_ > size_))
        {
            end_ = size_ - start_;
        }
    }
};

}}
