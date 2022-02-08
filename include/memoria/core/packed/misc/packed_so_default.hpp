
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

#include <memoria/core/types.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/profiles/common/block_operations.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/core/packed/tools/packed_tools.hpp>


#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/optional.hpp>

#include <algorithm>

namespace memoria {

template <typename PkdStruct>
class PackedDefaultSO {

    PkdStruct* data_;

    using MyType = PackedDefaultSO;

public:

    PackedDefaultSO():
        data_()
    {}

    PackedDefaultSO(PkdStruct* data):
        data_(data)
    {}

    void setup() {
        data_ = nullptr;
    }

    void setup(PkdStruct* data)
    {
        data_ = data;
    }

    operator bool() const {
        return data_ != nullptr;
    }

    const PkdStruct* data() const {return data_;}
    PkdStruct* data() {return data_;}

    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        return data_->generateDataEvents(handler);
    }

    void check() const {
        return data_->check();
    }
};


}
