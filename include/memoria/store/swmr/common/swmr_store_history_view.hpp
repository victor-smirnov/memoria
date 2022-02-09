
// Copyright 2021 Victor Smirnov
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

#include <memoria/core/tools/span.hpp>

#include <memoria/api/store/swmr_store_api.hpp>
#include <memoria/store/swmr/common/swmr_store_datatypes.hpp>


#include <atomic>
#include <memory>
#include <vector>
#include <unordered_map>

namespace memoria {

template <typename Profile> class SWMRStoreBase;

template <typename Profile>
class SWMRStoreHistoryViewImpl: public ISWMRStoreHistoryView<ApiProfile<Profile>> {
    using ApiProfileT = ApiProfile<Profile>;
    using Base = ISWMRStoreHistoryView<ApiProfileT>;

public:
    using typename Base::SnapshotID;

private:
    using SnapshotMetadataT = SWMRSnapshotMetadata<ApiProfileT>;

    struct HNode {
        SnapshotID snapshot_id;
        HNode* parent;
        std::vector<HNode*> children;
        SnapshotMetadataT metadata;
    };

    HNode* root_{};
    std::unordered_map<SnapshotID, HNode*> snapshots_;
    std::unordered_map<U8String, HNode*> branches_;

public:
    SWMRStoreHistoryViewImpl() {}

    std::vector<U8String> branch_names() override
    {
        std::vector<U8String> data;

        for (auto& br: branches_) {
            data.push_back(br.first);
        }

        return data;
    }

    Optional<SnapshotID> branch_head(U8StringView name) override
    {
        auto ii = branches_.find(name);
        if (ii != branches_.end())
        {
            return Optional<SnapshotID>{ii->second->snapshot_id};
        }

        return Optional<SnapshotID>{};
    }

    Optional<HNode*> branch_head_node(U8StringView name)
    {
        auto ii = branches_.find(name);
        if (ii != branches_.end())
        {
            return Optional<HNode*>{ii->second};
        }

        return Optional<HNode*>{};
    }



    Optional<std::vector<SnapshotID>> snapshots(U8StringView branch) override
    {
        using ResultT = Optional<std::vector<SnapshotID>>;
        auto head_opt = branch_head_node(branch);

        std::vector<SnapshotID> data;

        if (head_opt)
        {
            HNode* node =  head_opt.get();

            while (node) {
                data.push_back(node->snapshot_id);
                node = node->parent;
            }

            return ResultT{std::move(data)};
        }
        else {
            return ResultT{};
        }
    }

    Optional<std::vector<SnapshotID>> children(const SnapshotID& snapshot_id) override
    {
        using ResultT = Optional<std::vector<SnapshotID>>;

        auto ii = snapshots_.find(snapshot_id);
        if (ii != snapshots_.end())
        {
            std::vector<SnapshotID> data;

            for (HNode* chl: ii->second->children) {
                data.push_back(chl->snapshot_id);
            }

            return ResultT{std::move(data)};
        }

        return ResultT{};
    }

    Optional<SnapshotID> parent(const SnapshotID& snapshot_id) override
    {
        using ResultT = Optional<SnapshotID>;

        auto ii = snapshots_.find(snapshot_id);
        if (ii != snapshots_.end())
        {
            return ResultT{ii->second->metadata.parent_snapshot_id()};
        }

        return ResultT{};
    }

    Optional<bool> is_transient(const SnapshotID& snapshot_id) override
    {
        using ResultT = Optional<bool>;

        auto ii = snapshots_.find(snapshot_id);
        if (ii != snapshots_.end())
        {
            return ResultT{ii->second->metadata.is_transient()};
        }

        return ResultT{};
    }

    Optional<bool> is_system_snapshot(const SnapshotID& snapshot_id) override
    {
        using ResultT = Optional<bool>;

        auto ii = snapshots_.find(snapshot_id);
        if (ii != snapshots_.end())
        {
            return ResultT{ii->second->metadata.is_system_snapshot()};
        }

        return ResultT{};
    }

    void load(Span<const SnapshotID> snapshots, Span<const SnapshotMetadataT> metas)
    {
        for (size_t c = 0; c < snapshots.size(); c++)
        {
            HNode* node = new HNode{snapshots[c], nullptr, {}, metas[c]};
            snapshots_[snapshots[c]] = node;
        }
    }


    void load_branch(U8StringView name, const SnapshotID& head)
    {
        auto ii = snapshots_.find(head);
        if (ii != snapshots_.end()) {
            branches_[name] = ii->second;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("SWMRStoreHistoryView: Can't find snapshot id {} for branch '{}'", head, name).do_throw();
        }
    }

    void build_tree()
    {
        for (auto& pair: snapshots_)
        {
            SnapshotID parent_id = pair.second->metadata.parent_snapshot_id();

            if (parent_id)
            {
                auto pp = snapshots_.find(parent_id);
                if (pp != snapshots_.end()) {
                    pair.second->parent = pp->second;
                    pp->second->children.push_back(pair.second);
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("SWMRStoreHistoryView: Can't find parent for snapshot id {}", parent_id).do_throw();
                }
            }
            else {
                if (!root_) {
                    root_ = pair.second;
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR(
                        "SWMRStoreHistoryView: Multiple roots in the store history: {} {}",
                        root_->snapshot_id, pair.second->snapshot_id
                    ).do_throw();
                }
            }
        }
    }
};


}
