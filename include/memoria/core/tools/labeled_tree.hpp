
// Copyright 2013 Victor Smirnov
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

#include <memoria/core/tools/buffer.hpp>

#include <vector>
#include <tuple>

namespace memoria {
namespace tools {



template <typename Data, typename... Labels>
class LblTreeNode {
    typedef LblTreeNode<Data, Labels...>                                        MyType;

    std::vector<MyType> children_;
    std::tuple<Labels...> labels_;

    Data data_;

public:
    LblTreeNode() {}

    MyType& child(int32_t idx) {
        return children_[idx];
    }

    const MyType& child(int32_t idx) const {
        return children_[idx];
    }

    int32_t children() const
    {
        return children_.size();
    }

    MyType& addChild(int32_t idx)
    {
        children_.insert(children_.begin() + idx, MyType());

        return children_[idx];
    }

    MyType& appendChild()
    {
        return addChild(children());
    }

    void removeChild(int32_t idx)
    {
        children_.erase(children_.begin() + idx);
    }

    std::tuple<Labels...>& labels() {
        return labels_;
    }

    const std::tuple<Labels...>& labels() const {
        return labels_;
    }

    Data& data() {
        return data_;
    }

    const Data& data() const {
        return data_;
    }
};

template <typename Data, typename... Labels>
std::ostream& operator<<(std::ostream& out, const LblTreeNode<Data, Labels...>& node) {
    out<<"LblTreeNode["<<node.labels()<<"]";
    return out;
}

}}
