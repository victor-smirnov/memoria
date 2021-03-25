
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

#include <memoria/core/datatypes/core.hpp>
#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/datatypes/varchars/varchars.hpp>

#include <memoria/core/tools/lifetime_guard.hpp>
#include <memoria/core/tools/bitmap.hpp>

namespace memoria {

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

    ViewType view() const noexcept {
        return ViewType(data(), arena_.size());
    }

    GuardedView<ViewType> guarded_view() const {
        return GuardedView<ViewType>(view(), arena_.guard());
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
