
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/core/config.hpp>

#include <memoria/v1/core/tools/optional.hpp>
#include <memoria/v1/core/tools/cow_tree/cow_tree_node.hpp>
#include <memoria/v1/core/tools/cow_tree/cow_tree_iterator.hpp>
#include <memoria/v1/core/tools/cow_tree/cow_tree_txn.hpp>

#include <memoria/v1/core/tools/md5.hpp>

#include <mutex>

/**
 * MRSW Copy-on-write B+Tree (Without leaf links).
 */

namespace memoria {
namespace v1 {
namespace cow       {
namespace tree      {

class InvalidUpdateException: public std::exception {
public:
    virtual const char* what() const noexcept {
        return "Invalid snapshot update";
    }
};



template <typename Key, typename Value>
class CoWTree {

    static constexpr int32_t NodeIndexSize  = 32;
    static constexpr int32_t NodeSize       = NodeIndexSize * 8;

    using MyType        = CoWTree<Key, Value>;

    using LeafNodeT     = LeafNode<Key, Value, NodeSize, NodeIndexSize>;
    using BranchNodeT   = BranchNode<Key, NodeSize, NodeIndexSize>;
    using NodeBaseT     = typename BranchNodeT::NodeBaseT;

    using TxnDataT      = TxnData<NodeBaseT, MyType>;
    using TxnLogT       = TxnLog<NodeBaseT>;
    using SnapshotT     = typename TxnLogT::SnapshotT;
    using TransactionT  = Transaction<TxnDataT>;


    using Iterator          = CoWTreeIterator<BranchNodeT, LeafNodeT>;
    using ConstIterator     = CoWTreeConstIterator<BranchNodeT, LeafNodeT>;
    using Path              = typename Iterator::Path;

    using MutexT            = std::recursive_mutex;
    using LockT             = std::lock_guard<MutexT>;

    TxnDataT* txn_data_;

    int64_t txn_id_counter_  = 0;
    int64_t node_id_counter_ = 0;

    MutexT mutex_;

    TxnLogT txn_log_;

    template <typename, typename> friend class TxnData;

    int64_t node_budget_max_     = 100;
    int64_t node_budget_         = 100;


public:
    CoWTree(): txn_data_(nullptr), mutex_(), txn_log_()
    {
    }

    std::recursive_mutex& mutex() {
        return mutex_;
    }

    SnapshotT snapshot()
    {
        return txn_log_.snapshot();
    }

    TransactionT transaction()
    {
        mutex_.lock();

        if (txn_data_)
        {
            mutex_.unlock();

            throw CoWTreeException("There is another active transaction for this CoWTree");
        }

        if (txn_log_.size() > 0)
        {
            txn_data_ = clone_last_tx();
        }
        else {
            txn_data_ = create_new_tx();
        }

        return TransactionT(txn_data_);
    }

    int64_t size(const TransactionT& txn) const
    {
        return txn.root()->metadata().size();
    }

    int64_t size(const SnapshotT& txn) const
    {
        return txn.root()->metadata().size();
    }

    void assign(TransactionT& txn, const Key& key, const Value& value)
    {
        check(txn);

        auto iter = this->template locate<Iterator>(txn.root(), key);

        if (!iter.is_end() && iter.key() == key)
        {
            iter.value() = value;
        }
        else {
            this->insert_to(iter, key, value);
        }
    }

    bool remove(TransactionT& txn, const Key& key)
    {
        check(txn);

        auto iter = this->template locate<Iterator>(txn.root(), key);

        if (!iter.is_end())
        {
            this->remove_from(iter);
            return true;
        }
        else {
            return false;
        }
    }

    auto locate(TransactionT& txn, const Key& key) const {
        check(txn);
        return this->template locate<Iterator>(txn.root(), key);
    }

    auto locate(SnapshotT& txn, const Key& key) const {
        check(txn);
        return this->template locate<ConstIterator>(txn.root(), key);
    }

    auto begin(SnapshotT& txn) const {
        check(txn);
        return this->template locate_begin<ConstIterator>(txn.root());
    }

    auto begin(TransactionT& txn) const {
        check(txn);
        return this->template locate_begin<Iterator>(txn.root());
    }

    template <typename TxnT>
    auto rbegin(TxnT& txn) const
    {
        auto iter = this->end(txn);
        iter--;
        return iter;
    }



    auto end(TransactionT& txn) const {
        check(txn);
        return this->template locate_end<Iterator>(txn.root());
    }

    auto end(SnapshotT& txn) const {
        check(txn);
        return this->template locate_end<ConstIterator>(txn.root());
    }

    template <typename TxnT>
    auto rend(TxnT& txn) const
    {
        auto iter = this->begin(txn);
        iter--;
        return iter;
    }

    Optional<Value> find(TransactionT& txn, const Key& key) const
    {
        check(txn);
        return this->find_value_in(txn.root(), key);
    }

    Optional<Value> find(SnapshotT& txn, const Key& key) const
    {
        check(txn);
        return this->find_value_in(txn.root(), key);
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

    void dump_log(std::ostream& out = std::cout)
    {
        txn_log_.dump(out);
    }

    int64_t cleanup_snapshots()
    {
        LockT lock(mutex_);
        return do_cleanup_snapshots();
    }

    void check_log()
    {
        LockT lock(mutex_);
        auto& events = txn_log_.events_;

        for (size_t c = 0; c < events.size(); c++)
        {
            auto event = events[c];
            auto hash = snapshot_hash(event->root());

            cout<<"c: "<<c<<" "<<event->root()->txn_id()<<" "<<event->root()->node_id()<<" "<<hex<<event->md5_sum()<<" "<<hash<<dec<<" "<<(hash == event->md5_sum())<<endl;
        }
    }

protected:

    void check(TransactionT& txn) const
    {
        if (txn.get() != txn_data_)
        {
            throw CoWTreeException("Invalid transaction");
        }
    }

    void check(SnapshotT& txn) const
    {
        if (txn.get() == nullptr)
        {
            throw CoWTreeException("Invalid snapshot");
        }
    }

    uint64_t snapshot_hash(const NodeBaseT* root)
    {
        uint64_t sum = 0;

        snapshot_hash(root, sum, root->txn_id());

        return sum;
    }

    void snapshot_hash(const NodeBaseT* node, uint64_t& sum, int64_t txn_id)
    {
        if (node->txn_id() == txn_id)
        {
            if (node->is_leaf())
            {
                const LeafNodeT* leaf = to_leaf_node(node);
                auto hash = leaf->hash();

                sum ^= hash;
            }
            else {
                const BranchNodeT* branch = to_branch_node(node);

                auto hash = branch->hash();

                sum ^= hash;

                for (int32_t c = 0; c < branch->size(); c++)
                {
                    snapshot_hash(branch->data(c), sum, txn_id);
                }
            }
        }
    }

    void assert_current_txn(const NodeBaseT* node)
    {
        if (node->txn_id() != txn_data_->txn_id())
        {
            throw InvalidUpdateException();
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

    void update_path(Path& path, int32_t level = 0)
    {
        if (level < path.size() - 1)
        {
            NodeBaseT* node = path[level];

            if (node->txn_id() < txn_data_->txn_id())
            {
                BranchNodeT* parent = to_branch_node(path[level + 1]);
                if (parent->txn_id() < txn_data_->txn_id())
                {
                    update_path(path, level + 1);

                    parent = to_branch_node(path[level + 1]);
                }

                int32_t parent_idx = parent->find_child_node(node);

                NodeBaseT* clone = clone_node(node);

                parent->data(parent_idx)->unref();
                parent->data(parent_idx) = clone;

                clone->ref();

                path[level] = clone;
            }
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

        txn_data_->root()->metadata().add_size(1);
    }

    void remove_from(Iterator& iter)
    {
        update_path(iter.path());

        LeafNodeT* leaf = iter.leaf();

        leaf->remove(iter.idx(), iter.idx() + 1);

        update_keys_up(iter.path(), iter.idx(), 0);

        txn_data_->root()->metadata().add_size(-1);

        if (leaf->should_merge())
        {
            Path next = iter.path();

            if (Iterator::get_next_node(iter.path(), next, 0))
            {
                if (can_merge_paths(iter.path(), next))
                {
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
                        merge_paths(prev, iter.path());

                        iter.path() = prev;

                        iter.add_idx(prev_leaf_size);
                    }
                }
            }
        }
    }

    int64_t remove_snapshot(NodeBaseT* node)
    {
        int64_t removed = 0;

        if (node->is_branch())
        {
            BranchNodeT* branch = to_branch_node(node);

            for (int32_t c = 0; c < branch->size(); c++)
            {
                NodeBaseT* child = branch->data(c);
                if (child->refs() <= 1)
                {
                    removed += remove_snapshot(child);

                    if (node_budget_ < node_budget_max_)
                    {
                        node_budget_++;
                    }
                }
                else {
                    child->unref();
                }
            }

            remove_node(branch);
            removed++;
        }
        else {
            remove_node(node);
            removed++;
        }

        return removed;
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

        NodeBaseT* right = create_node(level, txn_data_->txn_id());
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
            BranchNodeT* new_root = create_branch_node(txn_data_->txn_id());
            new_root->metadata() = txn_data_->root()->metadata();

            insert_child_node(new_root, 0, node);
            insert_child_node(new_root, 1, right);

            path.insert(path.size(), new_root);
            next.insert(path.size(), new_root);

            next[level] = right;

            txn_data_->set_root(new_root);
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

        if (parent == txn_data_->root() && parent->size() == 1)
        {
            path.remove(path.size() - 1);
            next.remove(path.size() - 1);

            node->metadata() = txn_data_->root()->metadata();

            txn_data_->set_root(node);
            node->unref();
        }

        next[level] = node;
    }


    int64_t new_txn_id() {
        return ++txn_id_counter_;
    }

    void ref_children(BranchNodeT* node, int64_t txn_id)
    {
        for (int32_t c = 0; c < node->size(); c++)
        {
            NodeBaseT* child = node->data(c);

            if (child->txn_id() < txn_id)
            {
                child->ref();
            }
        }
    }

    NodeBaseT* create_node(int32_t level, int64_t txn_id)
    {
        if (level == 0) {
            return create_leaf_node(txn_id);
        }
        else {
            return create_branch_node(txn_id);
        }
    }

    NodeBaseT* create_node(int32_t level)
    {
        if (level == 0) {
            return create_leaf_node(txn_data_->txn_id());
        }
        else {
            return create_branch_node(txn_data_->txn_id());
        }
    }

    NodeBaseT* clone_node(NodeBaseT* node, int64_t txn_id)
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
            return clone_leaf_node(to_leaf_node(node), txn_data_->txn_id());
        }
        else {
            return clone_branch_node(to_branch_node(node), txn_data_->txn_id());
        }
    }

    BranchNodeT* create_branch_node(int64_t txn_id)
    {
        ensure_node_budget(1);

        return new BranchNodeT(txn_id, ++node_id_counter_);
    }

    BranchNodeT* clone_branch_node(BranchNodeT* node, int64_t txn_id)
    {
        BranchNodeT* clone = clone_node_t(node, txn_id);

        ref_children(node, txn_id);

        return clone;
    }

    LeafNodeT* create_leaf_node(int64_t txn_id)
    {
        ensure_node_budget(1);

        return new LeafNodeT(txn_id, ++node_id_counter_);
    }

    LeafNodeT* clone_leaf_node(LeafNodeT* node, int64_t txn_id)
    {
        return clone_node_t(node, txn_id);
    }

    template <typename NodeT>
    NodeT* clone_node_t(NodeT* node, int64_t txn_id)
    {
        ensure_node_budget(1);

        int64_t node_id = ++node_id_counter_;

        NodeT* new_node = new NodeT(txn_id, node_id);

        CopyBuffer(node, new_node, 1);

        new_node->set_txn_id(txn_id);
        new_node->set_node_id(node_id);
        new_node->clear_refs();

        return new_node;
    }

    void remove_node(NodeBaseT* node) const
    {
        if (node->is_leaf()) {
            delete to_leaf_node(node);
        }
        else {
            delete to_branch_node(node);
        }

    }

    void ensure_node_budget(int64_t adjustment)
    {
        if (node_budget_ == 0)
        {
            do_cleanup_snapshots();
        }

        if (node_budget_ >= adjustment) {
            node_budget_ -= adjustment;
        }
        else {
            node_budget_ = 0;
        }
    }

    int64_t do_cleanup_snapshots()
    {
        int64_t removed = 0;

        auto& events = txn_log_.events_;

        if (events.size() > 0)
        {
            for (auto c = events.size() - 1; c > 0; c--)
            {
                auto* event = events[c];

                {
                    typename TxnLogT::LockT lock(txn_log_.mutex());

                    if (event->refs() == 0)
                    {
                        event->lock();
                    }
                }

                if (event->locked())
                {
//                  auto txn_id = event->root()->txn_id();
//                  auto removed =
                    remove_snapshot(event->root());
//                  cout<<"Removed snapshot: "<<txn_id<<" nodes: "<<removed<<endl;

                    typename TxnLogT::LockT lock(txn_log_.mutex());

                    events.erase(events.begin() + c);

                    removed++;
                }
            }
        }

        return removed;
    }

private:

    TxnDataT* clone_last_tx()
    {
        int64_t current_txn_id = new_txn_id();

        NodeBaseT* root = txn_log_.events_.front()->root();

        NodeBaseT* new_root = clone_node(root, current_txn_id);

        return new TxnDataT(new_root, this);
    }

    TxnDataT* create_new_tx()
    {
        int64_t txn_id = new_txn_id();
        return new TxnDataT(create_leaf_node(txn_id), this);
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

    void commit_txn(TxnDataT* data)
    {
//      auto hash = snapshot_hash(data->root());
        txn_log_.new_entry(data->root(), 0);
        txn_data_ = nullptr;

        mutex_.unlock();
    }

    void rollback_txn(TxnDataT* data)
    {
        remove_snapshot(data->root());
        txn_data_ = nullptr;

        mutex_.unlock();
    }
};


}
}
}}