
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
    using typename Base::CommitID;

private:
    using CommitMetadataT = CommitMetadata<ApiProfileT>;

    struct HNode {
        CommitID commit_id;
        HNode* parent;
        std::vector<HNode*> children;
        CommitMetadataT metadata;
    };

    HNode* root_{};
    std::unordered_map<CommitID, HNode*> commits_;
    std::unordered_map<U8String, HNode*> branches_;

public:
    SWMRStoreHistoryViewImpl() noexcept {}

    std::vector<U8String> branch_names() override
    {
        std::vector<U8String> data;

        for (auto& br: branches_) {
            data.push_back(br.first);
        }

        return data;
    }

    Optional<CommitID> branch_head(U8StringView name) override
    {
        auto ii = branches_.find(name);
        if (ii != branches_.end())
        {
            return Optional<CommitID>{ii->second->commit_id};
        }

        return Optional<CommitID>{};
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



    Optional<std::vector<CommitID>> commits(U8StringView branch) override
    {
        using ResultT = Optional<std::vector<CommitID>>;
        auto head_opt = branch_head_node(branch);

        std::vector<CommitID> data;

        if (head_opt)
        {
            HNode* node =  head_opt.get();

            while (node) {
                data.push_back(node->commit_id);
                node = node->parent;
            }

            return ResultT{std::move(data)};
        }
        else {
            return ResultT{};
        }
    }

    Optional<std::vector<CommitID>> children(const CommitID& commit_id) override
    {
        using ResultT = Optional<std::vector<CommitID>>;

        auto ii = commits_.find(commit_id);
        if (ii != commits_.end())
        {
            std::vector<CommitID> data;

            for (HNode* chl: ii->second->children) {
                data.push_back(chl->commit_id);
            }

            return ResultT{std::move(data)};
        }

        return ResultT{};
    }

    Optional<CommitID> parent(const CommitID& commit_id) override
    {
        using ResultT = Optional<CommitID>;

        auto ii = commits_.find(commit_id);
        if (ii != commits_.end())
        {
            return ResultT{ii->second->metadata.parent_commit_id()};
        }

        return ResultT{};
    }

    Optional<bool> is_persistent(const CommitID& commit_id) override
    {
        using ResultT = Optional<bool>;

        auto ii = commits_.find(commit_id);
        if (ii != commits_.end())
        {
            return ResultT{ii->second->metadata.is_persistent()};
        }

        return ResultT{};
    }

    void load(Span<const CommitID> commits, Span<const CommitMetadataT> metas)
    {
        for (size_t c = 0; c < commits.size(); c++)
        {
            HNode* node = new HNode{commits[c], nullptr, {}, metas[c]};
            commits_[commits[c]] = node;
        }
    }


    void load_branch(U8StringView name, const CommitID& head)
    {
        auto ii = commits_.find(head);
        if (ii != commits_.end()) {
            branches_[name] = ii->second;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("SWMRStoreHistoryView: Can't find commit id {} for branch '{}'", head, name).do_throw();
        }
    }

    void build_tree()
    {
        for (auto& pair: commits_)
        {
            CommitID parent_id = pair.second->metadata.parent_commit_id();

            if (parent_id)
            {
                auto pp = commits_.find(parent_id);
                if (pp != commits_.end()) {
                    pair.second->parent = pp->second;
                    pp->second->children.push_back(pair.second);
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("SWMRStoreHistoryView: Can't find parent for commit id {}", parent_id).do_throw();
                }
            }
            else {
                if (!root_) {
                    root_ = pair.second;
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR(
                        "SWMRStoreHistoryView: Multiple roots in the store history: {} {}",
                        root_->commit_id, pair.second->commit_id
                    ).do_throw();
                }
            }
        }
    }
};


}
