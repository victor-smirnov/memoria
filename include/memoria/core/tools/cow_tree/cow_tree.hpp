
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef INCLUDE_MEMORIA_CORE_TOOLS_COW_TREE_COW_TREE_HPP_
#define INCLUDE_MEMORIA_CORE_TOOLS_COW_TREE_COW_TREE_HPP_

#include <memoria/core/types/types.hpp>

#include <memoria/core/tools/optional.hpp>
#include <memoria/core/tools/cow_tree/cow_tree_node.hpp>
#include <memoria/core/tools/cow_tree/cow_tree_iterator.hpp>

#include <vector>

/**
 * Serializable Copy-on-write B+Tree (Without leaf links).
 */

namespace memoria 	{
namespace cow 		{
namespace tree 		{

template <typename Key, typename Value>
class CoWTree {

	static constexpr Int NodeIndexSize 	= 32;
	static constexpr Int NodeSize 		= NodeIndexSize * 8;


	using LeafNodeT 	= LeafNode<Key, Value, NodeSize, NodeIndexSize>;
	using BranchNodeT 	= BranchNode<Key, NodeSize, NodeIndexSize>;
	using NodeBaseT 	= typename BranchNodeT::NodeBaseT;

	using Iterator		= CoWTreeIterator<NodeBaseT, BranchNodeT, LeafNodeT>;
	using Path			= typename Iterator::Path;

	NodeBaseT* current_root_;

	BigInt txn_id_counter_  = 0;
	BigInt node_id_counter_ = 0;

	BigInt current_txn_id_;

	std::vector<NodeBaseT*> roots_;

public:
	CoWTree() {
		current_root_ = create_leaf_node();
	}

	BigInt size() const {
		return current_root_->metadata().size();
	}

	void assign(const Key& key, const Value& value)
	{
		Iterator iter = this->locate(current_root_, key);

		if (!iter.is_end() && iter.key() == key)
		{
			iter.value() = value;
		}
		else {
			this->insert_to(iter, key, value);
		}
	}

	bool remove(const Key& key)
	{
		Iterator iter = this->locate(current_root_, key);

		if (!iter.is_end())
		{
			this->remove_from(iter);
			return true;
		}
		else {
			return false;
		}
	}

	Iterator locate(const Key& key) const {
		return this->locate(current_root_, key);
	}

	Iterator begin() const {
		return this->locate_begin(current_root_);
	}

	Iterator rbegin() const
	{
		auto iter = this->end();

		iter--;

		return iter;
	}

	Iterator end() const {
		return this->locate_end(current_root_);
	}

	Iterator rend() const
	{
		auto iter = this->begin();
		iter--;

		return iter;
	}

	Optional<Value>
	find(BigInt txn, const Key& key) const
	{
		return this->find_value_in(current_root_, key);
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

protected:

	Optional<Value>
	find_value_in(const NodeBaseT* node, const Key& key) const
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

	Iterator
	locate(NodeBaseT* node, const Key& key) const
	{
		Iterator iter;

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


	Iterator
	locate_begin(NodeBaseT* node) const
	{
		Iterator iter;

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

	Iterator
	locate_end(NodeBaseT* node) const
	{
		Iterator iter;

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

	void insert_to(Iterator& iter, const Key& key, const Value& value)
	{
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

		current_root_->metadata().add_size(1);
	}

	void remove_from(Iterator& iter)
	{
		LeafNodeT* leaf = iter.leaf();

		leaf->remove(iter.idx(), iter.idx() + 1);

		update_keys_up(iter.path(), iter.idx(), 0);

		current_root_->metadata().add_size(-1);

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
						Int prev_leaf_size = prev[0]->size();
						merge_paths(prev, iter.path());

						iter.path() = prev;

						iter.add_idx(prev_leaf_size);
					}
				}
			}
		}
	}

	void split_path(Path& path, Path& next, Int level = 0)
	{
		NodeBaseT* node = path[level];

		Int split_at = node->size() / 2;

		NodeBaseT* right = create_node(level);
		split_node(node, split_at, right);

		if (level < path.size() - 1)
		{
			BranchNodeT* parent = to_branch_node(path[level + 1]);
			Int parent_idx = parent->find_child_node(node);

			parent->key(parent_idx) = node->max_key();
			parent->reindex();
			update_keys_up(path, parent_idx, level + 1);

			if (parent->has_space())
			{
				parent->insert(parent_idx + 1, right->max_key(), right);
				update_keys_up(path, parent_idx + 1, level + 1);

				next[level] = right;
			}
			else {
				split_path(path, next, level + 1);

				if (parent_idx >= parent->size())
				{
					path[level + 1] = next[level + 1];

					Int next_parent_idx	= parent_idx - parent->size();
					BranchNodeT* next_parent = to_branch_node(next[level + 1]);

					next_parent->insert(next_parent_idx + 1, right->max_key(), right);
					update_keys_up(next, next_parent_idx + 1, level + 1);
				}
				else {
					next[level + 1] = path[level + 1];

					parent->insert(parent_idx + 1, right->max_key(), right);
					update_keys_up(path, parent_idx + 1, level + 1);
				}

				next[level] = right;
			}
		}
		else
		{
			BranchNodeT* new_root = create_branch_node();
			new_root->metadata() = current_root_->metadata();

			new_root->insert(0, node->max_key(), node);
			new_root->insert(1, node->max_key(), right);

			path.insert(path.size(), new_root);
			next.insert(path.size(), new_root);

			next[level] = right;

			this->current_root_ = new_root;
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

		parent->remove(parent_idx + 1, parent_idx + 2);

		update_keys_up(path, parent_idx, level + 1);

		if (parent == current_root_ && parent->size() == 1)
		{
			path.remove(path.size() - 1);
			next.remove(path.size() - 1);

			node->metadata() = current_root_->metadata();

			current_root_ = node;
		}

		next[level] = node;
	}


	BigInt new_txn_id() {
		return ++txn_id_counter_;
	}

	NodeBaseT* create_node(Int level)
	{
		if (level == 0) {
			return create_leaf_node();
		}
		else {
			return create_branch_node();
		}
	}

	BranchNodeT* create_branch_node()
	{
		return new BranchNodeT(current_txn_id_, ++node_id_counter_);
	}

	LeafNodeT* create_leaf_node()
	{
		return new LeafNodeT(current_txn_id_, ++node_id_counter_);
	}

	void remove_node(NodeBaseT* node) const
	{
		delete node;
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

private:
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
}
}

#endif /* INCLUDE_MEMORIA_CORE_TOOLS_COW_TREE_HPP_ */
