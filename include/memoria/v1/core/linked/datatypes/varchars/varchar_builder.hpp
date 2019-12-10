
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

#include <memoria/v1/core/linked/datatypes/core.hpp>
#include <memoria/v1/core/linked/datatypes/traits.hpp>
#include <memoria/v1/core/linked/datatypes/varchars/varchars.hpp>

#include <memoria/v1/core/tools/bitmap.hpp>

namespace memoria {
namespace v1 {

template <typename Buffer>
class SparseObjectBuilder<Varchar, Buffer> {
    Buffer* buffer_;

    using AtomType = DTTAtomType<Varchar>;
    using ViewType = DTTViewType<Varchar>;

    ArenaBuffer<char> arena_;

public:
    SparseObjectBuilder(Buffer* buffer):
        buffer_(buffer)
    {}

    SparseObjectBuilder(SparseObjectBuilder&&) = delete;
    SparseObjectBuilder(const SparseObjectBuilder&) = delete;

    void append(ViewType view)
    {
        arena_.append_values(view.data(), view.size());
    }

    ViewType view() const {
        return ViewType(data(), arena_.size());
    }

    void reset() {
        arena_.reset();
    }

    void build()
    {
        buffer_->append(view());
        arena_.clear();
    }

    AtomType* data() {
        return arena_.data();
    }

    const AtomType* data() const {
        return arena_.data();
    }

    bool is_empty() const {
        return arena_.size() == 0;
    }
};

}
}
