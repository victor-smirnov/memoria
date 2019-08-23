
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

namespace memoria {
namespace v1 {

template <typename CtrT, typename NodeType_>
class BranchNodeSO: public NodeCommonSO<CtrT, NodeType_> {
    using Base = NodeCommonSO<CtrT, NodeType_>;

    using Base::node_;
    using Base::ctr_;

public:
    using Base::size;

    BranchNodeSO(): Base() {}
    BranchNodeSO(CtrT* ctr, NodeType_* node):
        Base(ctr, node)
    {}

    template <typename V>
    std::vector<V> values_as_vector(int32_t start, int32_t end) const {
        return node_->template values_as_vector<V>(start, end);
    }

    template <typename V>
    std::vector<V> values_as_vector() const
    {
        return node_->template values_as_vector<V>();
    }

    auto& value(int32_t idx) {
        return node_->value(idx);
    }

    const auto& value(int32_t idx) const {
        return node_->value(idx);
    }

    template <typename... Args>
    void forAllValues(Args&&... args) const
    {
        node_->forAllValues(std::forward<Args>(args)...);
    }

    void layout(uint64_t active_streams) {
        node_->layout(active_streams);
    }

    template <typename BranchNodeEntry>
    void max(BranchNodeEntry& entry) const {
        node_->max(entry);
    }

    template <typename BranchNodeEntry>
    OpStatus updateUp(int32_t idx, const BranchNodeEntry& keys) {
        return node_->updateUp(idx, keys);
    }

    auto sizes() const {
        return node_->sizes();
    }

    template <typename BranchNodeEntry, typename Value>
    OpStatus insert(int32_t idx, const BranchNodeEntry& keys, const Value& value)
    {
        return node_->insert(idx, keys, value);
    }

    int32_t capacity() const
    {
        return node_->capacity();
    }

    int32_t capacity(uint64_t active_streams) const
    {
        return node_->capacity(active_streams);
    }

    OpStatus removeSpace(int32_t room_start, int32_t room_end) {
        return node_->removeSpace(room_start, room_end);
    }

    OpStatus removeSpaceAcc(int32_t room_start, int32_t room_end)
    {
        return node_->removeSpace(room_start, room_end);
    }


    int32_t size() const {
        return node_->size();
    }

    template <typename OtherNodeT>
    OpStatus splitTo(OtherNodeT&& other, int32_t split_idx) {
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
};

}
}
