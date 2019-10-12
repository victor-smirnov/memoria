
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

#include <memoria/v1/api/datatypes/core.hpp>
#include <memoria/v1/api/datatypes/traits.hpp>
#include <memoria/v1/api/datatypes/varchars/varchars.hpp>

#include <memoria/v1/core/tools/bitmap.hpp>

namespace memoria {
namespace v1 {

template <typename Buffer>
class SparseObjectBuilder<Varchar, Buffer> {
    Buffer* buffer_;
    size_t offset_;
    size_t size_;
    bool refresh_views_{};

    using AtomType = DTTAtomType<Varchar>;
    using ViewType = DTTViewType<Varchar>;

public:
    SparseObjectBuilder(Buffer* buffer):
        buffer_(buffer), offset_(buffer->size()), size_()
    {}

    SparseObjectBuilder(SparseObjectBuilder&&) = delete;
    SparseObjectBuilder(const SparseObjectBuilder&) = delete;

    void append(ViewType view)
    {
        refresh_views_ = buffer().ensure(view.size()) || refresh_views_;
        MemCpyBuffer(view.data(), data() + size_, view.size());
        size_ += view.size();
    }

    ViewType view() const {
        return ViewType(data(), size_);
    }

    void build()
    {
        buffer_->add_view(std::make_tuple(Span<const AtomType>(data(), size_)));
        if (refresh_views_) {
            buffer_->refresh_views();
        }

        offset_ = buffer().size();
        size_ = 0;
        refresh_views_ = false;
    }

    AtomType* data() {
        return buffer().data() + offset_;
    }

    const AtomType* data() const {
        return buffer().data() + offset_;
    }

    bool is_empty() const {
        return size_ == 0;
    }

private:
    auto& buffer() {
        return std::get<0>(buffer_->buffers());
    }

    const auto& buffer() const {
        return std::get<0>(buffer_->buffers());
    }
};

}
}
