
// Copyright 2017 Victor Smirnov
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
#include <memoria/core/container/container.hpp>


#include <memoria/core/tools/any_id.hpp>

#include <memoria/core/container/store.hpp>

#include <memoria/core/iovector/io_vector.hpp>

#include "ctr_api.hpp"

#include <limits>


namespace memoria {

template <typename Profile>
struct BTSSIterator {
    virtual ~BTSSIterator() noexcept = default;

    virtual const io::IOVector& iovector_view() const = 0;
    virtual int32_t iovector_pos() const    = 0;

    virtual bool is_end() const             = 0;
    virtual bool next_leaf()                = 0;
    virtual bool next_entry()               = 0;

    virtual void dump(std::ostream& out = std::cout, const char* header = nullptr) const = 0;
    virtual void dumpPath(std::ostream& out = std::cout, const char* header = nullptr) const = 0;

    virtual AnyID leaf_id() const = 0;
};

}
