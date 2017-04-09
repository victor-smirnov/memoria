
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

#include <dumbo/v1/tools/memory.hpp>

#include <memoria/v1/core/types/types.hpp>

#include <dumbo/v1/allocators/inmem/persistent_tree_node.hpp>
#include <dumbo/v1/allocators/inmem/persistent_tree_iterator.hpp>
#include <dumbo/v1/allocators/inmem/persistent_tree_snapshot.hpp>

#include <memoria/v1/core/exceptions/memoria.hpp>

#include <memoria/v1/core/tools/optional.hpp>

#include <vector>
#include <unordered_map>
#include <memory>
#include <malloc.h>
#include <mutex>

namespace dumbo {
namespace v1 {
namespace inmem {

using namespace memoria::v1;

template <typename BranchNodeT, typename LeafNodeT_, typename RootProvider, typename PageType>
class PersistentTree {
public:
    using LeafNodeT     = LeafNodeT_;
    using NodeId        = typename LeafNodeT::NodeId;
    using TxnId         = typename LeafNodeT::TxnId;
    using Key           = typename LeafNodeT::Key;
    using Value         = typename LeafNodeT::Value;
    using NodeBaseT     = typename BranchNodeT::NodeBaseT;

    using NodeBasePtr   = NodeBaseT*;

    using Iterator      = PersistentTreeIterator<BranchNodeT, LeafNodeT>;
    using ConstIterator = PersistentTreeConstIterator<BranchNodeT, LeafNodeT>;
    using Path          = typename Iterator::Path;

private:
    RootProvider* root_provider_;
    Int owner_cpu_id_;

public:

    PersistentTree(RootProvider* root_provider, Int owner_cpu_id):
        root_provider_(root_provider),
		owner_cpu_id_(owner_cpu_id)
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
                    throw Exception(MA_SRC, "Internal error. HistoryNode.root is null for transaction.");
                }
            }
            else if (!root_provider_->root()) {
                throw Exception(MA_SRC, "Internal error. HistoryNode.root is null for transaction.");
            }
        }
    }

    Int owner_cpu_id() const {return owner_cpu_id_;}


    NodeBasePtr root() {
        return root_provider_->root();
    }

    const NodeBasePtr root() const {
        return root_provider_->root();
    }

    const TxnId& txn_id() const {
        return root_provider_->txn_id();
    }


    BigInt size() const
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
        root_provider_->root_id() = typename PageType::ID();
    }


protected:

    void walk_tree(NodeBaseT* node, std::function<void (NodeBaseT*)> fn)
    {
        const auto& txn_id = root_provider_->txn_id();

        if (node->txn_id() == txn_id)
        {
            fn(node);

            if (!node->is_leaf())
            {
                auto branch_node = to_branch_node(node);
                for (Int c = 0; c < branch_node->size(); c++)
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

                leaf_node->del(owner_cpu_id());
            }
            else {
                auto branch_node = to_branch_node(node);

                for (Int c = 0; c < branch_node->size(); c++)
                {
                    auto child = branch_node->data(c);

                    delete_tree(child, fn);
                }

                branch_node->del(owner_cpu_id());
            }
        }
    }

    void assert_current_txn(const NodeBaseT* node)
    {
        if (node->txn_id() != this->txn_id())
        {
            throw Exception(MA_SRC, SBuf()<<"Transaction IDs do not match: "<<node->txn_id()<<" "<<this->txn_id());
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

        Int idx = leaf_node->find(key);

        if (idx < leaf_node->size() && leaf_node->key(idx) == key)
        {
            return Optional<Value>(leaf_node->data(idx));
        }
        else {
            return Optional<Value>();
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

        Int idx = leaf_node->find(key);

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

        void lock_path(Path& path)
        {
        	Int c = path.size() - 2;

        	try {
        		for (; c >= 0; c--)
        		{
        			path[c]->lock();
        		}
        	}
        	catch (std::system_error& ex)
        	{
        		for (Int d = c + 1; d < path.size() - 1; d++)
        		{
        			path[d]->unlock();
        		}
        	}
        }

        void unlock_path(Path& path)
        {
        	for (Int c = 0; c < path.size() - 1; c++)
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

    void clone_path(Path& path, Int level = 0)
    {
        if (level < path.size() - 1)
        {
            NodeBaseT* node = path[level];

            BranchNodeT* parent = to_branch_node(path[level + 1]);

            clone_path(path, level + 1);

            parent = to_branch_node(path[level + 1]);

            Int parent_idx = parent->find_child_node(node);

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
            iter.leaf()->insert(iter.idx(), key, value);
        }
        else {
            Path next = iter.path();

            split_path(iter.path(), next);

            if (iter.idx() >= iter.leaf()->size())
            {
                iter.add_idx(-iter.leaf()->size());

                iter.path() = next;
            }

            iter.leaf()->insert(iter.idx(), key, value);
            update_keys_up(iter.path(), iter.idx(), 0);
        }

        this->root()->metadata().add_size(1);
    }

    void remove_from(Iterator& iter, bool delete_on_unref)
    {
        update_path(iter.path());

        LeafNodeT* leaf = iter.leaf();

        auto page = leaf->data(iter.idx()).page_ptr();

        if (page->unref() == 0 && delete_on_unref)
        {
            delete page;
        }

        leaf->remove(iter.idx(), iter.idx() + 1);

        update_keys_up(iter.path(), iter.idx(), 0);

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
                        Int prev_leaf_size = prev[0]->size();
                        update_path(prev);
                        merge_paths(prev, iter.path());

                        iter.path() = prev;

                        iter.add_idx(prev_leaf_size);
                    }
                }
            }
        }
    }



    void insert_child_node(BranchNodeT* node, Int idx, NodeBaseT* child)
    {
        node->insert(idx, child->max_key(), child);
        child->ref();
    }

    void split_path(Path& path, Path& next, Int level = 0)
    {
        NodeBaseT* node = path[level];

        Int split_at = node->size() / 2;

        NodeBaseT* right = create_node(level, this->txn_id());
        split_node(node, split_at, right);

        if (level < path.size() - 1)
        {
            BranchNodeT* parent = to_branch_node(path[level + 1]);
            Int parent_idx      = parent->find_child_node(node);

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

                    Int next_parent_idx      = parent_idx - parent->size();
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
            BranchNodeT* new_root = create_branch_node(this->txn_id());
            new_root->metadata() = this->root()->metadata();

            insert_child_node(new_root, 0, node);
            insert_child_node(new_root, 1, right);

            path.insert(path.size(), new_root);
            next.insert(next.size(), new_root);

            next[level] = right;

            root_provider_->set_root(new_root);
        }
    }



    bool can_merge_paths(Path& path, Path& next, Int level = 0)
    {
        if (path[level]->capacity() >= next[level]->size())
        {
            BranchNodeT* path_parent = to_branch_node(path[level + 1]);
            BranchNodeT* next_parent = to_branch_node(next[level + 1]);

            if (next_parent == path_parent)
            {
                Int path_parent_idx = next_parent->find_child_node(path[level]);
                Int next_parent_idx = next_parent->find_child_node(next[level]);

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

    void update_keys_up(Path& path, Int insertion_point, Int level)
    {
        if (level < path.size() - 1)
        {
            NodeBaseT* node = path[level];

            if (insertion_point >= node->size() - 1)
            {
                NodeBaseT* parent = path[level + 1];
                Int parent_idx = to_branch_node(parent)->find_child_node(node);

                parent->key(parent_idx) = node->max_key();
                parent->reindex();

                update_keys_up(path, parent_idx, level + 1);
            }
        }
    }


    void merge_paths(Path& path, Path& next, Int level = 0)
    {
        if (path[level + 1] != next[level + 1])
        {
            merge_paths(path, next, level + 1);
        }

        NodeBaseT* node  = path[level];
        NodeBaseT* right = next[level];

        BranchNodeT* parent = to_branch_node(path[level + 1]);
        Int parent_idx = parent->find_child_node(node);

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



    void ref_children(BranchNodeT* node, const TxnId& txn_id)
    {
        for (Int c = 0; c < node->size(); c++)
        {
            NodeBaseT* child = node->data(c);

            child->ref();
        }
    }

    NodeBaseT* create_node(Int level, const TxnId& txn_id)
    {
        if (level == 0) {
            return create_leaf_node(txn_id);
        }
        else {
            return create_branch_node(txn_id);
        }
    }

    NodeBaseT* create_node(Int level)
    {
        if (level == 0) {
            return create_leaf_node(this->txn_id());
        }
        else {
            return create_branch_node(this->txn_id());
        }
    }

    NodeBaseT* clone_node(NodeBaseT* node, BigInt txn_id)
    {
        if (node->is_leaf()) {
            return clone_leaf_node(to_leaf_node(node), txn_id);
        }
        else {
            return clone_branch_node(to_branch_node(node), txn_id);
        }
    }

    NodeBaseT* clone_node(NodeBaseT* node)
    {
        if (node->is_leaf()) {
            return clone_leaf_node(to_leaf_node(node), this->txn_id());
        }
        else {
            return clone_branch_node(to_branch_node(node), this->txn_id());
        }
    }

    BranchNodeT* create_branch_node(const TxnId& txn_id)
    {
        ensure_node_budget(1);

        return new BranchNodeT(txn_id, root_provider_->new_node_id(), owner_cpu_id());
    }

    BranchNodeT* clone_branch_node(BranchNodeT* node, const TxnId& txn_id)
    {
        BranchNodeT* clone = clone_node_t(node, txn_id);

        ref_children(node, txn_id);

        return clone;
    }

    LeafNodeT* create_leaf_node(const TxnId& txn_id)
    {
        ensure_node_budget(1);

        return new LeafNodeT(txn_id, root_provider_->new_node_id(), owner_cpu_id());
    }

    LeafNodeT* clone_leaf_node(LeafNodeT* node, const TxnId& txn_id)
    {
        auto clone = clone_node_t(node, txn_id);

        for (Int c = 0; c < clone->size(); c++)
        {
            clone->data(c).page_ptr()->ref();
        }

        return clone;
    }

    template <typename NodeT>
    NodeT* clone_node_t(NodeT* node, const TxnId& txn_id)
    {
        ensure_node_budget(1);

        auto node_id = root_provider_->new_node_id();

        NodeT* new_node = new NodeT(txn_id, node_id, owner_cpu_id());

        CopyBuffer(node, new_node, 1);

        new_node->set_txn_id(txn_id);
        new_node->set_node_id(node_id);
        new_node->clear_refs();

        return new_node;
    }

    void remove_node(NodeBaseT* node) const
    {
        if (node->is_leaf())
        {
            to_leaf_node(node)->del(owner_cpu_id());
        }
        else {
            to_branch_node(node)->del(owner_cpu_id());
        }
    }

    void ensure_node_budget(BigInt adjustment)
    {
    }

    void split_node(NodeBaseT* node, Int split_idx, NodeBaseT* to) const
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
