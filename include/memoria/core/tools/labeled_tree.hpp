
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/core/tools/buffer.hpp>

#include <vector>
#include <tuple>

namespace memoria   {
namespace tools     {



template <typename Data, typename... Labels>
class LblTreeNode {
    typedef LblTreeNode<Data, Labels...>                                        MyType;

    std::vector<MyType> children_;
    std::tuple<Labels...> labels_;

    Data data_;

public:
    LblTreeNode() {}

    MyType& child(Int idx) {
        return children_[idx];
    }

    const MyType& child(Int idx) const {
        return children_[idx];
    }

    Int children() const
    {
        return children_.size();
    }

    MyType& addChild(Int idx)
    {
        children_.insert(children_.begin() + idx, MyType());

        return children_[idx];
    }

    MyType& appendChild()
    {
        return addChild(children());
    }

    void removeChild(Int idx)
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

}
}
