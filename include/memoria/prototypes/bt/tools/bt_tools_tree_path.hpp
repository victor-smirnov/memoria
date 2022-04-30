
// Copyright 2020 Victor Smirnov
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

#include <memoria/core/tools/result.hpp>

#include <vector>

namespace memoria {

template <typename NodeT>
class TreePath {
    std::vector<NodeT> path_;
    size_t size_;
public:
    TreePath() : path_(), size_() {}

    TreePath(size_t size) : path_(size), size_(size) {}

    TreePath(const TreePath& other) :
        path_(other.path_),
        size_(other.size_)
    {}

    TreePath(const TreePath& other, size_t level) :
        path_(other.path_.size()),
        size_(other.size_)
    {
        for (size_t ll = level; ll < size_; ll++) {
            path_[ll] = other.path_[ll];
        }
    }

    TreePath(TreePath&& other) :
        path_(std::move(other.path_)),
        size_(other.size_)
    {}

    TreePath& operator=(const TreePath& other)  {
        path_ = other.path_;
        size_ = other.size_;
        return *this;
    }

    TreePath& operator=(TreePath&& other)  {
        path_ = std::move(other.path_);
        size_ = other.size_;
        return *this;
    }

    void resize(size_t size)
    {
        if (size > size_) {
            path_.resize(size);
            size_ = size;
        }
        else {
            size_ = size;
            for (size_t c = size; c < path_.size(); c++) {
                path_[c] = NodeT{};
            }
        }
    }

    NodeT& leaf()  {
        return path_[0];
    }

    const NodeT& leaf() const  {
        return path_[0];
    }

    NodeT& root()  {
        return path_[size_ - 1];
    }

    const NodeT& root() const  {
        return path_[size_ - 1];
    }

    NodeT& operator[](size_t idx)
    {
        if (MMA_UNLIKELY(idx >= size_)) {
            terminate("Invalid tree path access");
        }

        return path_[idx];
    }

    const NodeT& operator[](size_t idx) const
    {
        if (MMA_UNLIKELY(idx >= size_)) {
            terminate("Invalid tree path access");
        }

        return path_[idx];
    }

    void set(size_t idx, const NodeT& node) {
        if (MMA_LIKELY(idx >= 0 && idx <= size_)) {
            path_[idx] = node;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Range check in TreePath: {} {}", idx, size()).do_throw();
        }
    }

    size_t size() const  {
        return size_;
    }

    void add_root(NodeT node)
    {
        if (path_.size() > size_)
        {
            path_[size_] = node;
            size_++;
        }
        else {
            path_.push_back(node);
            size_ = path_.size();
        }
    }

    void remove_root()
    {
        if (size_ > 0) {
            path_[size_ - 1] = NodeT{};
            size_--;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("TreePath is empty").do_throw();
        }
    }

    void clear() {
        for (NodeT& node: path_) {
            node = NodeT{};
        }
        size_ = 0;
    }

    static TreePath build(const NodeT& top_node, size_t height)
    {
        TreePath path(height);
        path.set(height - 1, top_node);
        return path;
    }

    void reset_state() noexcept {
        clear();
    }
};

}
