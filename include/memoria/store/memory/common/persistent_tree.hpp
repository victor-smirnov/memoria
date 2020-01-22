
// Copyright 2016 Victor Smirnov
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

#include "persistent_tree_node.hpp"
#include "persistent_tree_iterator.hpp"


#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/tools/optional.hpp>

#include <vector>
#include <unordered_map>
#include <memory>
#include <stdlib.h>
#include <mutex>

namespace memoria {
namespace store {
namespace memory {


template <typename BranchNodeT, typename LeafNodeT_, typename RootProvider, typename BlockType>
class PersistentTree {
public:
    using LeafNodeT     = LeafNodeT_;
    using NodeId        = typename LeafNodeT::NodeId;
    using SnapshotID    = typename LeafNodeT::SnapshotID;
    using Key           = typename LeafNodeT::Key;
    using Value         = typename LeafNodeT::Value;
    using NodeBaseT     = typename BranchNodeT::NodeBaseT;

    using NodeBasePtr   = NodeBaseT*;

    using Iterator      = PersistentTreeIterator<BranchNodeT, LeafNodeT>;
    using ConstIterator = PersistentTreeConstIterator<BranchNodeT, LeafNodeT>;
    using Path          = typename Iterator::Path;

    using MutexT		= typename std::remove_reference<decltype(std::declval<RootProvider>().snapshot_mutex())>::type;
    using LockGuardT	= typename std::lock_guard<MutexT>;

private:
    RootProvider* root_provider_;

public:

    PersistentTree(RootProvider* root_provider):
        root_provider_(root_provider)
    {
        if (root_provider_->is_active())
        {
            if (root_provider_->parent())
            {
                MEMORIA_V1_ASSERT_TRUE(root_provider_->parent()->is_committed());

                auto root_provider = root_provider_->parent();

                RootProvider* target = nullptr;

                while (root_provider)
                {
                	LockGuardT guard(root_provider_->snapshot_mutex());

                    if (!root_provider->is_dropped())
                    {
                        target = root_provider;
                        break;
                    }

                    root_provider = root_provider->parent();
                }

                if (target)
                {
                    auto new_root = clone_node(target->root());
                    root_provider_->set_root(new_root);
                    root_provider_->root_id() = target->root_id();
                }
                else {
                    MMA_THROW(Exception()) << WhatCInfo("Internal error. HistoryNode.root is null for transaction.");
                }
            }
            else if (!root_provider_->root()) {
                MMA_THROW(Exception()) << WhatCInfo("Internal error. HistoryNode.root is null for transaction.");
            }
        }
    }


    NodeBasePtr root() {
        return root_provider_->root();
    }

    const NodeBasePtr root() const {
        return root_provider_->root();
    }

    const SnapshotID& snapshot_id() const {
        return root_provider_->snapshot_id();
    }


    int64_t size() const
    {
        return this->root()->metadata().size();
    }

    Value assign(const Key& key, const Value& value)
    {
        auto iter = this->locate(key);

        if (!iter.is_end() && iter.key() == key)
        {
            update_path(iter.path());

            auto tmp = iter.value();

            iter.value() = value;

            return tmp;
        }
        else {
            insert_to(iter, key, value);
            return Value();
        }
    }

    bool remove(const Key& key, bool delete_on_unref = true)
    {
        auto iter = this->locate(key);

        if (!iter.is_end())
        {
            remove_from(iter, delete_on_unref);
            return true;
        }
        else {
            return false;
        }
    }

    void remove(Iterator& iter, bool delete_on_unref = true) {
        remove_from(iter, delete_on_unref);
    }

    auto locate(const Key& key) const {
        return this->template locate<ConstIterator>(this->root(), key);
    }

    auto locate(const Key& key) {
        return this->template locate<Iterator>(this->root(), key);
    }

    auto begin() const {
        return this->template locate_begin<ConstIterator>(this->root());
    }

    auto begin() {
        return this->template locate_begin<Iterator>(this->root());
    }

    auto rbegin() const
    {
        auto iter = this->end();
        iter--;
        return iter;
    }

    auto rbegin()
    {
        auto iter = this->end();
        iter--;
        return iter;
    }


    auto end() const {
        return this->template locate_end<ConstIterator>(this->root());
    }

    auto end() {
        return this->template locate_end<Iterator>(this->root());
    }

    auto rend() const
    {
        auto iter = this->begin();
        iter--;
        return iter;
    }

    auto rend()
    {
        auto iter = this->begin();
        iter--;
        return iter;
    }

    Optional<Value> find(const Key& key) const
    {
        return this->find_value_in(this->root(), key);
    }

    static BranchNodeT* to_branch_node(NodeBaseT* node) {
        return static_cast<BranchNodeT*>(node);
    }

    static const BranchNodeT* to_branch_node(const NodeBaseT* node) {
        return static_cast<const BranchNodeT*>(node);
    }

    static LeafNodeT* to_leaf_node(NodeBaseT* node) {
        return static_cast<LeafNodeT*>(node);
    }

    static const LeafNodeT* to_leaf_node(const NodeBaseT* node) {
        return static_cast<const LeafNodeT*>(node);
    }

    void dump(std::ostream& out = std::cout) const {
        dump(root_provider_->root(), out);
    }

    void dump(const NodeBaseT* node, std::ostream& out = std::cout) const
    {
        if (node->is_leaf())
        {
            to_leaf_node(node)->dump(out);
        }
        else {
            to_branch_node(node)->dump(out);
        }
    }

    void dump_tree(std::ostream& out = std::cout)
    {
    	walk_tree([&, this](NodeBaseT* node){
    		this->dump(node, out);
    	});
    }


    void walk_tree(std::function<void (NodeBaseT*)> fn) {
        walk_tree( root(), fn);
    }

    void delete_tree(std::function<void (LeafNodeT*)> fn)
    {
        delete_tree( root(), fn);

        root_provider_->assign_root_no_ref(nullptr);
        root_provider_->root_id() = typename BlockType::BlockID{};
    }


    template <typename NodeConsumer>
    void conditional_tree_traverse(NodeConsumer& node_consumer) {
        conditional_tree_traverse(root(), node_consumer);
    }

    template <typename NodeConsumer>
    void conditional_tree_traverse(NodeBaseT* node, NodeConsumer& node_consumer)
    {
        if (node->is_leaf())
        {
            auto leaf_node = to_leaf_node(node);
            bool proceed = node_consumer.process_ptree_leaf(leaf_node);
            if(proceed)
            {
                for (int32_t c = 0; c < leaf_node->size(); c++)
                {
                    auto& child = leaf_node->data(c);
                    node_consumer.process_data_block(child.block_ptr()->raw_data());
                }
            }
        }
        else
        {
            auto branch_node = to_branch_node(node);
            bool proceed = node_consumer.process_ptree_branch(branch_node);
            if (proceed)
            {
                for (int32_t c = 0; c < branch_node->size(); c++)
                {
                    auto child = branch_node->data(c);
                    conditional_tree_traverse(child, node_consumer);
                }
            }
        }
    }


protected:

    void walk_tree(NodeBaseT* node, std::function<void (NodeBaseT*)> fn)
    {
        const auto& snapshot_id = root_provider_->snapshot_id();

        if (node->snapshot_id() == snapshot_id)
        {
            fn(node);

            if (!node->is_leaf())
            {
                auto branch_node = to_branch_node(node);
                for (int32_t c = 0; c < branch_node->size(); c++)
                {
                    auto child = branch_node->data(c);
                    walk_tree(child, fn);
                }
            }
        }
    }

    void delete_tree(NodeBaseT* node, const std::function<void (LeafNodeT*)>& fn)
    {
        if (node->unref() == 0)
        {
            if (node->is_leaf())
            {
                auto leaf_node = to_leaf_node(node);

                fn(leaf_node);

                leaf_node->del();
            }
            else {
                auto branch_node = to_branch_node(node);

                for (int32_t c = 0; c < branch_node->size(); c++)
                {
                    auto child = branch_node->data(c);

                    delete_tree(child, fn);
                }

                branch_node->del();
            }
        }
    }

    void assert_current_txn(const NodeBaseT* node)
    {
        if (node->snapshot_id() != this->snapshot_id())
        {
            throw Exception(MA_SRC, SBuf()<<"Transaction IDs do not match: "<<node->snapshot_id()<<" "<<this->snapshot_id());
        }
    }

    Optional<Value> find_value_in(const NodeBaseT* node, const Key& key) const
    {
        while (node->is_branch())
        {
            const BranchNodeT* branch_node = to_branch_node(node);
            node = branch_node->find_child(key);
        }

        const LeafNodeT* leaf_node = to_leaf_node(node);

        int32_t idx = leaf_node->find(key);

        if (idx < leaf_node->size() && leaf_node->key(idx) == key)
        {
            return Optional<Value>(leaf_node->data(idx));
        }
        else {
            return Optional<Value>{};
        }
    }




    template <typename IterT>
    IterT locate(NodeBaseT* node, const Key& key) const
    {
        IterT iter;

        iter.path().insert(0, node);

        while (node->is_branch())
        {
            const BranchNodeT* branch_node = to_branch_node(node);
            node = branch_node->find_child(key);

            iter.path().insert(0, node);
        }

        const LeafNodeT* leaf_node = to_leaf_node(node);

        int32_t idx = leaf_node->find(key);

        iter.set_idx(idx);

        return iter;
    }

    template <typename IterT>
    IterT locate_begin(NodeBaseT* node) const
    {
        IterT iter;

        iter.path().insert(0, node);

        while (node->is_branch())
        {
            BranchNodeT* branch_node = to_branch_node(node);
            node = branch_node->first_child();

            iter.path().insert(0, node);
        }

        iter.set_idx(0);

        return iter;
    }

    template <typename IterT>
    IterT locate_end(NodeBaseT* node) const
    {
        IterT iter;

        iter.path().insert(0, node);

        while (node->is_branch())
        {
            BranchNodeT* branch_node = to_branch_node(node);
            node = branch_node->last_child();

            iter.path().insert(0, node);
        }

        const LeafNodeT* leaf_node = to_leaf_node(node);
        iter.set_idx(leaf_node->size());

        return iter;
    }

    class PathLocker {
    	Path& path_;
    public:
    	PathLocker(Path& path): path_(path) {lock_path(path_);}
    	~PathLocker() {unlock_path(path_);}

    private:
        void lock_path(Path& path)
        {
        	int32_t c = path.size() - 2;

        	try {
        		for (; c >= 0; c--)
        		{
        			path[c]->lock();
        		}
        	}
        	catch (std::system_error& ex)
        	{
        		for (int32_t d = c + 1; d < path.size() - 1; d++)
        		{
        			path[d]->unlock();
        		}

                throw ex;
        	}
        }

        void unlock_path(Path& path)
        {
        	for (int32_t c = 0; c < path.size() - 1; c++)
        	{
        		path[c]->unlock();
        	}
        }
    };

    void update_path(Path& path)
    {
    	Path src = path;
    	PathLocker lk(src);

    	bool split = false;

    	for (int c = path.size() - 2; c >= 0; c--)
    	{
    		if (path[c]->references() > 1) {
    			split = true;
    			break;
    		}
    	}

    	if (split) {
    		clone_path(path);
    	}
    }

    void clone_path(Path& path, int32_t level = 0)
    {
        if (level < path.size() - 1)
        {
            NodeBaseT* node = path[level];

            BranchNodeT* parent = to_branch_node(path[level + 1]);

            clone_path(path, level + 1);

            parent = to_branch_node(path[level + 1]);

            int32_t parent_idx = parent->find_child_node(node);

            NodeBaseT* clone = clone_node(node);

            // FIXME: remove?
            parent->data(parent_idx)->unref();
            parent->data(parent_idx) = clone;

            clone->ref();

            path[level] = clone;
        }
    }



    void insert_to(Iterator& iter, const Key& key, const Value& value)
    {
        update_path(iter.path());

        if (iter.leaf()->has_space())
        {
            iter.leaf()->insert(iter.local_pos(), key, value);
        }
        else {
            Path next = iter.path();

            split_path(iter.path(), next);

            if (iter.local_pos() >= iter.leaf()->size())
            {
                iter.add_idx(-iter.leaf()->size());

                iter.path() = next;
            }

            iter.leaf()->insert(iter.local_pos(), key, value);
            update_keys_up(iter.path(), iter.local_pos(), 0);
        }

        this->root()->metadata().add_size(1);
    }

    void remove_from(Iterator& iter, bool delete_on_unref)
    {
        update_path(iter.path());

        LeafNodeT* leaf = iter.leaf();

        auto block = leaf->data(iter.local_pos()).block_ptr();

        if (block->unref() == 0 && delete_on_unref)
        {
            delete block;
        }

        leaf->remove(iter.local_pos(), iter.local_pos() + 1);

        update_keys_up(iter.path(), iter.local_pos(), 0);

        root()->metadata().add_size(-1);

        if (leaf->should_merge())
        {
            Path next = iter.path();

            if (Iterator::get_next_node(iter.path(), next, 0))
            {
                if (can_merge_paths(iter.path(), next))
                {
                    update_path(next);
                    merge_paths(iter.path(), next);
                }
            }
            else {
                Path prev = iter.path();

                if (Iterator::get_prev_node(iter.path(), prev, 0))
                {
                    if (can_merge_paths(prev, iter.path()))
                    {
                        int32_t prev_leaf_size = prev[0]->size();
                        update_path(prev);
                        merge_paths(prev, iter.path());

                        iter.path() = prev;

                        iter.add_idx(prev_leaf_size);
                    }
                }
            }
        }
    }



    void insert_child_node(BranchNodeT* node, int32_t idx, NodeBaseT* child)
    {
        node->insert(idx, child->max_key(), child);
        child->ref();
    }

    void split_path(Path& path, Path& next, int32_t level = 0)
    {
        NodeBaseT* node = path[level];

        int32_t split_at = node->size() / 2;

        NodeBaseT* right = create_node(level, this->snapshot_id());
        split_node(node, split_at, right);

        if (level < path.size() - 1)
        {
            BranchNodeT* parent = to_branch_node(path[level + 1]);
            int32_t parent_idx      = parent->find_child_node(node);

            parent->key(parent_idx) = node->max_key();
            parent->reindex();
            update_keys_up(path, parent_idx, level + 1);

            if (parent->has_space())
            {
                insert_child_node(parent, parent_idx + 1, right);

                update_keys_up(path, parent_idx + 1, level + 1);

                next[level] = right;
            }
            else {
                split_path(path, next, level + 1);

                if (parent_idx >= parent->size())
                {
                    path[level + 1] = next[level + 1];

                    int32_t next_parent_idx      = parent_idx - parent->size();
                    BranchNodeT* next_parent = to_branch_node(next[level + 1]);

                    insert_child_node(next_parent, next_parent_idx + 1, right);

                    update_keys_up(next, next_parent_idx + 1, level + 1);
                }
                else {
                    next[level + 1] = path[level + 1];

                    insert_child_node(parent, parent_idx + 1, right);

                    update_keys_up(path, parent_idx + 1, level + 1);
                }

                next[level] = right;
            }
        }
        else
        {
            BranchNodeT* new_root = create_branch_node(this->snapshot_id());
            new_root->metadata() = this->root()->metadata();

            insert_child_node(new_root, 0, node);
            insert_child_node(new_root, 1, right);

            path.insert(path.size(), new_root);
            next.insert(next.size(), new_root);

            next[level] = right;

            root_provider_->set_root(new_root);
        }
    }



    bool can_merge_paths(Path& path, Path& next, int32_t level = 0)
    {
        if (path[level]->capacity() >= next[level]->size())
        {
            BranchNodeT* path_parent = to_branch_node(path[level + 1]);
            BranchNodeT* next_parent = to_branch_node(next[level + 1]);

            if (next_parent == path_parent)
            {
                int32_t path_parent_idx = next_parent->find_child_node(path[level]);
                int32_t next_parent_idx = next_parent->find_child_node(next[level]);

                return path_parent_idx == next_parent_idx - 1;
            }
            else {
                return can_merge_paths(path, next, level + 1);
            }
        }
        else {
            return false;
        }
    }

    void update_keys_up(Path& path, int32_t insertion_point, int32_t level)
    {
        if (level < path.size() - 1)
        {
            NodeBaseT* node = path[level];

            if (insertion_point >= node->size() - 1)
            {
                NodeBaseT* parent = path[level + 1];
                int32_t parent_idx = to_branch_node(parent)->find_child_node(node);

                parent->key(parent_idx) = node->max_key();
                parent->reindex();

                update_keys_up(path, parent_idx, level + 1);
            }
        }
    }


    void merge_paths(Path& path, Path& next, int32_t level = 0)
    {
        if (path[level + 1] != next[level + 1])
        {
            merge_paths(path, next, level + 1);
        }

        NodeBaseT* node  = path[level];
        NodeBaseT* right = next[level];

        BranchNodeT* parent = to_branch_node(path[level + 1]);
        int32_t parent_idx = parent->find_child_node(node);

        merge_from(node, right);
        parent->key(parent_idx) = node->max_key();

        parent->remove(parent_idx + 1);

        update_keys_up(path, parent_idx, level + 1);

        auto root = this->root();

        if (parent == root && parent->size() == 1)
        {
            path.remove(path.size() - 1);
            next.remove(next.size() - 1);

            node->metadata() = root->metadata();

            root_provider_->set_root(node);

            node->unref();

            remove_node(root);

            MEMORIA_V1_ASSERT(node->references(), ==, 1);
        }

        next[level] = node;
    }



    void ref_children(BranchNodeT* node, const SnapshotID& snapshot_id)
    {
        for (int32_t c = 0; c < node->size(); c++)
        {
            NodeBaseT* child = node->data(c);

            child->ref();
        }
    }

    NodeBaseT* create_node(int32_t level, const SnapshotID& snapshot_id)
    {
        if (level == 0) {
            return create_leaf_node(snapshot_id);
        }
        else {
            return create_branch_node(snapshot_id);
        }
    }

    NodeBaseT* create_node(int32_t level)
    {
        if (level == 0) {
            return create_leaf_node(this->snapshot_id());
        }
        else {
            return create_branch_node(this->snapshot_id());
        }
    }

    NodeBaseT* clone_node(NodeBaseT* node, int64_t snapshot_id)
    {
        if (node->is_leaf()) {
            return clone_leaf_node(to_leaf_node(node), snapshot_id);
        }
        else {
            return clone_branch_node(to_branch_node(node), snapshot_id);
        }
    }

    NodeBaseT* clone_node(NodeBaseT* node)
    {
        if (node->is_leaf()) {
            return clone_leaf_node(to_leaf_node(node), this->snapshot_id());
        }
        else {
            return clone_branch_node(to_branch_node(node), this->snapshot_id());
        }
    }

    BranchNodeT* create_branch_node(const SnapshotID& snapshot_id)
    {
        ensure_node_budget(1);

        return new BranchNodeT(snapshot_id, root_provider_->new_node_id());
    }

    BranchNodeT* clone_branch_node(BranchNodeT* node, const SnapshotID& snapshot_id)
    {
        BranchNodeT* clone = clone_node_t(node, snapshot_id);

        ref_children(node, snapshot_id);

        return clone;
    }

    LeafNodeT* create_leaf_node(const SnapshotID& snapshot_id)
    {
        ensure_node_budget(1);

        return new LeafNodeT(snapshot_id, root_provider_->new_node_id());
    }

    LeafNodeT* clone_leaf_node(LeafNodeT* node, const SnapshotID& snapshot_id)
    {
        auto clone = clone_node_t(node, snapshot_id);

        for (int32_t c = 0; c < clone->size(); c++)
        {
            clone->data(c).block_ptr()->ref();
        }

        return clone;
    }

    template <typename NodeT>
    NodeT* clone_node_t(NodeT* node, const SnapshotID& snapshot_id)
    {
        ensure_node_budget(1);

        auto node_id = root_provider_->new_node_id();

        NodeT* new_node = new NodeT(snapshot_id, node_id);

        new_node->copy_data_from(node);

        //CopyBuffer(node, new_node, 1);

        new_node->set_snapshot_id(snapshot_id);
        new_node->set_node_id(node_id);
        new_node->clear_refs();

        return new_node;
    }

    void remove_node(NodeBaseT* node) const
    {
        if (node->is_leaf())
        {
            to_leaf_node(node)->del();
        }
        else {
            to_branch_node(node)->del();
        }
    }

    void ensure_node_budget(int64_t adjustment)
    {
    }

    void split_node(NodeBaseT* node, int32_t split_idx, NodeBaseT* to) const
    {
        if (node->is_leaf())
        {
            to_leaf_node(node)->split_to(split_idx, to_leaf_node(to));
        }
        else {
            to_branch_node(node)->split_to(split_idx, to_branch_node(to));
        }
    }

    void merge_from(NodeBaseT* node, NodeBaseT* to) const
    {
        if (node->is_leaf())
        {
            to_leaf_node(node)->merge_from(to_leaf_node(to));
        }
        else {
            to_branch_node(node)->merge_from(to_branch_node(to));
        }

        remove_node(to);
    }
};


}
}}
