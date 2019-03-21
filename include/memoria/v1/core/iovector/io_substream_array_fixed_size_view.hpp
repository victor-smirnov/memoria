
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

#include <memoria/v1/core/iovector/io_substream_array_base.hpp>

#include <memoria/v1/core/types/type2type.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/strings/format.hpp>

#include <memoria/v1/core/memory/malloc.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>


#include <type_traits>
#include <cstring>

namespace memoria {
namespace v1 {
namespace io {

/*
class IOArraySubstreamFixedSizeView: public IOArraySubstream {

public:
    IOArraySubstreamFixedSizeView()
    {

    }

    virtual ~IOArraySubstreamFixedSizeView() noexcept {
    }

    uint8_t* reserve(int32_t data_size, int32_t values)
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    uint8_t* reserve(int32_t data_size, int32_t values, uint64_t* nulls_bitmap) {
        MMA1_THROW(UnsupportedOperationException());
    }

    virtual bool is_static() const {
        return false;
    }

    uint8_t* enlarge(int32_t required) final
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    virtual void reset() {
        data_size_ = 0;
        size_ = 0;
    }

    virtual void init(void* ptr) {
        MMA1_THROW(UnsupportedOperationException());
    }

protected:
    void range_check(int32_t pos, int32_t size) const
    {
        if (MMA1_UNLIKELY(pos + size > size_))
        {
            MMA1_THROW(BoundsException()) << fmt::format_ex(u"IOStreamUnalignedNative range check: pos={}, size={}, stream size={}", pos, size, size_);
        }
    }
};
*/

}}}
