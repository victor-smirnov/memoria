
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

#include <vector>

namespace memoria {

template <typename NodeT>
class TreePath {
    std::vector<NodeT> path_;
public:
    TreePath() noexcept: path_() {}

    TreePath(size_t size) noexcept: path_(size) {}

    TreePath(const TreePath& other) noexcept:
        path_(other.path_)
    {}

    TreePath(const TreePath& other, size_t level) noexcept:
        path_(other.path_.size())
    {
        for (size_t ll = level; ll < path_.size(); ll++) {
            path_[ll] = other.path_[ll];
        }
    }

    TreePath(TreePath&& other) noexcept:
        path_(std::move(other.path_))
    {}

    TreePath& operator=(const TreePath& other) noexcept {
        path_ = other.path_;
        return *this;
    }

    TreePath& operator=(TreePath&& other) noexcept {
        path_ = std::move(other.path_);
        return *this;
    }

    void resize(size_t size) noexcept {
        path_.resize(size);
    }

    NodeT& leaf() noexcept {
        return path_[0];
    }

    const NodeT& leaf() const noexcept {
        return path_[0];
    }

    NodeT& root() noexcept {
        return path_[path_.size() - 1];
    }

    const NodeT& root() const noexcept {
        return path_[path_.size() - 1];
    }

    NodeT& operator[](size_t idx) noexcept
    {
        if (idx >= path_.size()) {
            terminate("Invalid tree path access");
        }

        return path_[idx];
    }

    const NodeT& operator[](size_t idx) const noexcept
    {
        if (idx >= path_.size()) {
            terminate("Invalid tree path access");
        }

        return path_[idx];
    }

    void set(size_t idx, const NodeT& node) noexcept {
        path_[idx] = node;
    }

    size_t size() const noexcept {
        return path_.size();
    }

    void add_root(NodeT node) noexcept {
        path_.push_back(node);
    }

    void remove_root() noexcept {
        path_.erase(path_.begin() + size() - 1);
    }

    void clear() noexcept {
        path_.clear();
    }

    static TreePath build(const NodeT& top_node, size_t height)
    {
        TreePath path(height);
        path.set(height - 1, top_node);
        return path;
    }
};

}
