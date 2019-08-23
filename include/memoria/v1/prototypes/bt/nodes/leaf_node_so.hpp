
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

#include <memoria/v1/prototypes/bt/nodes/node_common_so.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

namespace memoria {
namespace v1 {

template <typename CtrT, typename NodeType_>
class LeafNodeSO: public NodeCommonSO<CtrT, NodeType_> {
    using Base = NodeCommonSO<CtrT, NodeType_>;

    using Base::node_;
    using Base::ctr_;


public:
    using typename Base::Position;

    LeafNodeSO(): Base() {}
    LeafNodeSO(CtrT* ctr, NodeType_* node):
        Base(ctr, node)
    {}

    static std::unique_ptr<io::IOVector> create_iovector()
    {
        return NodeType_::create_iovector();
    }

    std::unique_ptr<io::IOVector> create_iovector_view()
    {
        return node_->create_iovector_view();
    }

    void configure_iovector_view(io::IOVector& io_vector)
    {
        node_->configure_iovector_view(io_vector);
    }

    void layout(const Position& sizes) {
        node_->layout(sizes);
    }

    template <typename BranchNodeEntry>
    void max(BranchNodeEntry& entry) const {
        node_->max(entry);
    }

    auto sizes() const {
        return node_->sizes();
    }



    OpStatus removeSpace(int32_t room_start, int32_t room_end) {
        return node_->removeSpace(room_start, room_end);
    }

    OpStatus removeSpace(const Position& room_start, const Position& room_end) {
        return node_->removeSpace(room_start, room_end);
    }

    template <typename OtherNodeT>
    OpStatus splitTo(OtherNodeT&& other, const Position& split_idx) {
        return node_->splitTo(std::forward<OtherNodeT>(other), split_idx);
    }

    template <typename OtherNodeT>
    OpStatus mergeWith(OtherNodeT&& other) {
        return node_->mergeWith(std::forward<OtherNodeT>(other));
    }

    template <typename OtherNodeT>
    bool canBeMergedWith(OtherNodeT&& other) const {
        return node_->canBeMergedWith(std::forward<OtherNodeT>(other));
    }

    int32_t single_stream_capacity(int32_t max_hops) const {
        return node_->single_stream_capacity(max_hops);
    }
};

}
}
