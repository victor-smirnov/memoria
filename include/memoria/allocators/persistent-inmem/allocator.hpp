
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_ALLOCATORS_PERSISTENT_INMEM_ALLOCATOR_HPP
#define _MEMORIA_ALLOCATORS_PERSISTENT_INMEM_ALLOCATOR_HPP

#include <unordered_map>
#include <string>

#include <memoria/core/tools/stream.hpp>

#include <memoria/core/container/metadata_repository.hpp>

#include <memoria/core/tools/pool.hpp>
#include <memoria/core/tools/uuid.hpp>

#include <memoria/allocators/persistent-inmem/persistent_tree_node.hpp>
#include <memoria/allocators/persistent-inmem/persistent_tree.hpp>

#include <memoria/allocators/inmem/factory.hpp>

#include <malloc.h>
#include <memoria/allocators/persistent-inmem/persistent_tree_snapshot.hpp>
#include <memory>
#include <limits>
#include <unordered_map>

namespace memoria {

template <typename ValueT, typename TxnIdT>
class PersistentTreeValue {
	ValueT page_;
	TxnIdT txn_id_;
public:
	PersistentTreeValue() {}
	PersistentTreeValue(const ValueT& page, const TxnIdT& txn_id): page_(page), txn_id_(txn_id) {}

	const ValueT& page() const {return page_;}
	ValueT& page() {return page_;}

	const TxnIdT& txn_id() const {return txn_id_;}
	TxnIdT& txn_id() {return txn_id_;}
};

template <typename V, typename T>
vapi::OutputStreamHandler& operator<<(vapi::OutputStreamHandler& out, const PersistentTreeValue<V, T>& value)
{
    out << value.page();
    out << value.txn_id();
    return out;
}

template <typename V, typename T>
vapi::InputStreamHandler& operator>>(vapi::InputStreamHandler& in, PersistentTreeValue<V, T>& value)
{
	in >> value.page();
	in >> value.txn_id();
    return in;
}

template <typename V, typename T>
std::ostream& operator<<(std::ostream& out, const PersistentTreeValue<V, T>& value)
{
	out<<"PersistentTreeValue[";
    out << value.page();
    out<<", ";
    out << value.txn_id();
    out<<"]";

    return out;
}


template <typename Profile, typename PageType>
class PersistentInMemAllocatorT: public enable_shared_from_this<PersistentInMemAllocatorT<Profile, PageType>> {
public:

	static constexpr Int NodeIndexSize 	= 32;
	static constexpr Int NodeSize 		= NodeIndexSize * 8;

	using MyType 		= PersistentInMemAllocatorT<Profile, PageType>;

	using Page 			= PageType;


	using Key 			= typename PageType::ID;
	using Value 		= PageType*;

	using TxnId 		 	= UUID;
	using PTreeNodeId 	 	= UUID;

	using LeafNodeT 		= memoria::persistent_inmem::LeafNode<Key, PersistentTreeValue<PageType*, TxnId>, NodeSize, NodeIndexSize, PTreeNodeId, TxnId>;
	using LeafNodeBufferT 	= memoria::persistent_inmem::LeafNode<Key, PersistentTreeValue<typename PageType::ID, TxnId>, NodeSize, NodeIndexSize, PTreeNodeId, TxnId>;

	using BranchNodeT 		= memoria::persistent_inmem::BranchNode<Key, NodeSize, NodeIndexSize, PTreeNodeId, TxnId>;
	using BranchNodeBufferT = memoria::persistent_inmem::BranchNode<Key, NodeSize, NodeIndexSize, PTreeNodeId, TxnId, PTreeNodeId>;
	using NodeBaseT 		= typename BranchNodeT::NodeBaseT;
	using NodeBaseBufferT 	= typename BranchNodeBufferT::NodeBaseT;



	struct HistoryNode {

		enum class Status {ACTIVE, COMMITTED};

	private:
		HistoryNode* parent_;
		std::vector<HistoryNode*> children_;

		NodeBaseT* root_;

		Status status_;

		TxnId txn_id_;

	public:

		HistoryNode(HistoryNode* parent, Status status = Status::ACTIVE):
			parent_(parent),
			root_(nullptr),
			status_(status),
			txn_id_(UUID::make_random())
		{
			if (parent_) {
				parent_->children().push_back(this);
			}
		}

		HistoryNode(const TxnId& txn_id, HistoryNode* parent, Status status):
			parent_(parent),
			root_(nullptr),
			status_(status),
			txn_id_(txn_id)
		{
			if (parent_) {
				parent_->children().push_back(this);
			}
		}

		void remove_child(NodeBaseT* child)
		{
			for (size_t c = 0; c < children_.size(); c++)
			{
				if (children_[c] == child)
				{
					children_.erase(children_.begin() + c);
					break;
				}
			}
		}

		bool is_committed() const {
			return status_ == Status::COMMITTED;
		}

		bool is_active() const {
			return status_ == Status::ACTIVE;
		}

		const TxnId& txn_id() const {
			return txn_id_;
		}

		const auto& root() const {
			return root_;
		}

		auto root() {
			return root_;
		}

		void set_root(NodeBaseT* new_root) {
			root_ = new_root;
		}

		PTreeNodeId new_node_id() {
			return UUID::make_random();
		}

		const auto& parent() const {return parent_;}
		auto& parent() {return parent_;}

		auto& children() {
			return children_;
		}

		const auto& children() const {
			return children_;
		}


		Status& status() {return status_;}
		const Status& status() const {return status_;}

		void commit() {
			status_ = Status::COMMITTED;
		}
	};

	struct HistoryNodeBuffer {

		template <typename, typename>
		friend class PersistentInMemAllocatorT;

	private:
		TxnId parent_;
		std::vector<TxnId> children_;

		PTreeNodeId root_;

		typename HistoryNode::Status status_;

		TxnId txn_id_;

	public:

		HistoryNodeBuffer(){}

		const TxnId& txn_id() const {
			return txn_id_;
		}

		TxnId& txn_id() {
			return txn_id_;
		}

		const auto& root() const {
			return root_;
		}

		auto& root() {
			return root_;
		}


		const auto& parent() const {return parent_;}
		auto& parent() {return parent_;}

		std::vector<TxnId>& children() {
			return children_;
		}

		const std::vector<TxnId>& children() const {
			return children_;
		}

		auto& status() {return status_;}
		const auto& status() const {return status_;}
	};


	using PersistentTreeT 	= memoria::persistent_inmem::PersistentTree<BranchNodeT, LeafNodeT, HistoryNode>;
	using SnapshotT 	 	= memoria::persistent_inmem::Snapshot<Profile, PageType, HistoryNode, PersistentTreeT, MyType>;
	using SnapshotPtr 	= std::shared_ptr<SnapshotT>;

	using TxnMap		 		= std::unordered_map<TxnId, HistoryNode*, UUIDKeyHash, UUIDKeyEq>;

	using HistoryTreeNodeMap	= std::unordered_map<PTreeNodeId, HistoryNodeBuffer*, UUIDKeyHash, UUIDKeyEq>;
	using PersistentTreeNodeMap	= std::unordered_map<PTreeNodeId, std::pair<NodeBaseBufferT*, NodeBaseT*>, UUIDKeyHash, UUIDKeyEq>;
	using PageMap				= std::unordered_map<typename PageType::ID, PageType*, IDKeyHash, IDKeyEq>;

	template <typename, typename, typename, typename>
	friend class Snapshot;

	class AllocatorMetadata {
		TxnId master_;
		TxnId root_;
	public:
		AllocatorMetadata() {}

		TxnId& master() {return master_;}
		TxnId& root() {return root_;}

		const TxnId& master() const {return master_;}
		const TxnId& root()   const {return root_;}
	};

private:

	enum {TYPE_UNKNOWN, TYPE_HISTORY_NODE, TYPE_BRANCH_NODE, TYPE_LEAF_NODE, TYPE_DATA_PAGE};

	HistoryNode* history_tree_ = nullptr;
	HistoryNode* master_ = nullptr;

	TxnMap snapshot_map_;

	BigInt id_counter_ = 1;

	ContainerMetadataRepository*  metadata_;

	Int tag_;

public:

	PersistentInMemAllocatorT():
		metadata_(MetadataRepository<Profile>::getMetadata()),
		tag_(1)
	{
		SnapshotT::initMetadata();

		history_tree_ = new HistoryNode(nullptr, HistoryNode::Status::ACTIVE);

		auto leaf = new LeafNodeT(history_tree_->txn_id(), UUID::make_random());
		history_tree_->set_root(leaf);

		master_ = history_tree_;

		SnapshotT txn(history_tree_, this);

		txn.commit();
	}

private:

	PersistentInMemAllocatorT(Int):
		metadata_(MetadataRepository<Profile>::getMetadata()),
		tag_(2)
	{
	}

public:

	virtual ~PersistentInMemAllocatorT()
	{
		free_memory(history_tree_);
	}

	auto newId()
	{
		return typename PageType::ID(id_counter_++);
	}

	SnapshotPtr find(const TxnId& branch_id)
	{
		auto iter = snapshot_map_.find(branch_id);
		if (iter != snapshot_map_.end())
		{
			auto history_node = iter.second;
			return std::make_shared<SnapshotT>(history_node, this);
		}
		else {
			throw vapi::Exception(MA_SRC, SBuf()<<"Snapshot id "<<branch_id<<" is not known");
		}
	}

	SnapshotPtr master()
	{
		HistoryNode* node = new HistoryNode(master_);

		snapshot_map_[node->txn_id()] = node;

		return std::make_shared<SnapshotT>(node, this->shared_from_this());
	}

	void set_master(const TxnId& txn_id)
	{
		auto iter = snapshot_map_.find(txn_id);
		if (iter != snapshot_map_.end())
		{
			if (iter->second->is_committed())
			{
				master_ = iter->second;
			}
			else {
				throw vapi::Exception(MA_SRC, SBuf()<<"Snapshot "<<txn_id<<" hasn't been committed yet");
			}
		}
		else {
			throw vapi::Exception(MA_SRC, SBuf()<<"Snapshot "<<txn_id<<" is not known in this allocator");
		}
	}

	ContainerMetadataRepository* getMetadata() const {
		return metadata_;
	}

	virtual void walkContainers(vapi::ContainerWalker* walker, const char* allocator_descr = nullptr)
	{
		walker->beginAllocator("PersistentInMemAllocator", allocator_descr);

		walk_version_tree(history_tree_, [&](auto* history_tree_node, auto* txn){
			txn->walkContainers(walker);
		});

		walker->endAllocator();
	}

	virtual void store(OutputStreamHandler *output)
	{
		char signature[12] = "MEMORIA";
		for (size_t c = 7; c < sizeof(signature); c++) signature[c] = 0;

		output->write(&signature, 0, sizeof(signature));

		AllocatorMetadata meta;

		meta.master() 	= master_->txn_id();
		meta.root() 	= history_tree_->txn_id();

		write(*output, meta);

		walk_version_tree(history_tree_, [&](auto* history_tree_node, auto* txn) {
			write(*output, history_tree_node);
		});

		output->close();
	}


	static std::shared_ptr<MyType> load(InputStreamHandler *input)
	{
		MyType* allocator = new MyType(0);

		char signature[12];

		input->read(signature, sizeof(signature));

		if (!(
				signature[0] == 'M' &&
				signature[1] == 'E' &&
				signature[2] == 'M' &&
				signature[3] == 'O' &&
				signature[4] == 'R' &&
				signature[5] == 'I' &&
				signature[6] == 'A'))
		{
			throw vapi::Exception(MEMORIA_SOURCE, SBuf()<<"The stream does not start from MEMORIA signature: "<<signature);
		}

		if (!(signature[7] == 0 || signature[7] == 1))
		{
			throw vapi::BoundsException(MEMORIA_SOURCE, SBuf()<<"Endiannes filed value is out of bounds "<<signature[7]);
		}

		if (signature[8] != 0)
		{
			throw vapi::Exception(MEMORIA_SOURCE, "This is not an in-memory container");
		}

		allocator->master_ = allocator->history_tree_ = nullptr;

		allocator->snapshot_map_.clear();

		BigInt size = input->size();

		HistoryTreeNodeMap 		history_node_map;
		PersistentTreeNodeMap	ptree_node_map;
		PageMap					page_map;

		AllocatorMetadata metadata;

		*input >> metadata.master();
		*input >> metadata.root();

		while (input->pos() < size)
		{
			UByte type;
			*input >> type;

			switch (type)
			{
				case TYPE_DATA_PAGE: 	allocator->read_data_page(*input, page_map); break;
				case TYPE_LEAF_NODE: 	allocator->read_leaf_node(*input, ptree_node_map); break;
				case TYPE_BRANCH_NODE: 	allocator->read_branch_node(*input, ptree_node_map); break;
				case TYPE_HISTORY_NODE: allocator->read_history_node(*input, history_node_map); break;
				default:
					throw vapi::Exception(MA_SRC, SBuf()<<"Unknown record type: "<<(Int)type);
			}
		}

		for (auto& entry: ptree_node_map)
		{
			auto buffer = entry.second.first;
			auto node   = entry.second.second;

			if (buffer->is_leaf())
			{
				LeafNodeBufferT* leaf_buffer = static_cast<LeafNodeBufferT*>(buffer);
				LeafNodeT* leaf_node 		 = static_cast<LeafNodeT*>(node);

				for (Int c = 0; c < leaf_node->size(); c++)
				{
					const auto& descr = leaf_buffer->data(c);

					auto page_iter = page_map.find(descr.page());

					if (page_iter != page_map.end())
					{
						leaf_node->data(c) = typename LeafNodeT::Value(page_iter->second, descr.txn_id());
					}
					else {
						throw vapi::Exception(MA_SRC, SBuf()<<"Specified uuid "<<descr.page()<<" is not found in the page map");
					}
				}
			}
			else {
				BranchNodeBufferT* branch_buffer = static_cast<BranchNodeBufferT*>(buffer);
				BranchNodeT* branch_node 		   = static_cast<BranchNodeT*>(node);

				for (Int c = 0; c < branch_node->size(); c++)
				{
					const auto& node_id = branch_buffer->data(c);

					auto iter = ptree_node_map.find(node_id);

					if (iter != ptree_node_map.end())
					{
						branch_node->data(c) = iter->second.second;
					}
					else {
						throw vapi::Exception(MA_SRC, SBuf()<<"Specified uuid "<<node_id<<" is not found in the persistent tree node map");
					}
				}
			}
		}

		allocator->history_tree_ = allocator->build_history_tree(metadata.root(), nullptr, history_node_map, ptree_node_map);

		if (allocator->snapshot_map_.find(metadata.master()) != allocator->snapshot_map_.end())
		{
			allocator->master_ = allocator->snapshot_map_[metadata.master()];
		}
		else {
			throw vapi::Exception(MA_SRC, SBuf()<<"Specified master uuid "<<metadata.master()<<" is not found in the data");
		}

		return std::shared_ptr<MyType>(allocator);
	}

private:

	HistoryNode* build_history_tree(
		const TxnId& txn_id,
		HistoryNode* parent,
		const HistoryTreeNodeMap& history_map,
		const PersistentTreeNodeMap& ptree_map
	)
	{
		auto iter = history_map.find(txn_id);

		if (iter != history_map.end())
		{
			HistoryNodeBuffer* buffer = iter->second;

			HistoryNode* node = new HistoryNode(txn_id, parent, buffer->status());

			snapshot_map_[txn_id] = node;

			auto ptree_iter = ptree_map.find(buffer->root());

			if (ptree_iter != ptree_map.end())
			{
				node->set_root(ptree_iter->second.second);

				for (const auto& child_txn_id: buffer->children())
				{
					auto child = build_history_tree(child_txn_id, node, history_map, ptree_map);
					node->children().push_back(child);
				}

				return node;
			}
			else {
				throw vapi::Exception(MA_SRC, SBuf()<<"Specified node_id "<<buffer->root()<<" is not found in persistent tree data");
			}
		}
		else {
			throw vapi::Exception(MA_SRC, SBuf()<<"Specified txn_id "<<txn_id<<" is not found in history data");
		}
	}


	void free_memory(HistoryNode* node)
	{
		for (auto child: node->children())
		{
			free_memory(child);
		}

		free_memory(node->root());

		delete node;
	}


	void free_memory(const NodeBaseT* node)
	{
		const auto& txn_id = node->txn_id();
		if (node->is_leaf())
		{
			auto leaf = PersistentTreeT::to_leaf_node(node);

			for (Int c = 0; c < leaf->size(); c++)
			{
				auto& data = leaf->data(c);
				if (data.txn_id() == txn_id)
				{
					::free(data.page());
				}
			}

			delete leaf;
		}
		else {
			auto branch = PersistentTreeT::to_branch_node(node);

			for (Int c = 0; c < branch->size(); c++)
			{
				auto child = branch->data(c);
				if (child->txn_id() == txn_id)
				{
					free_memory(child);
				}
			}

			delete branch;
		}
	}


	void read_data_page(InputStreamHandler& in, PageMap& map)
	{
		Int page_data_size;
		in >> page_data_size;

		Int page_size;
		in >> page_size;

		Int ctr_hash;
		in >> ctr_hash;

		Int page_hash;
		in >> page_hash;

		unique_ptr<Byte, void (*)(void*)> page_data((Byte*)::malloc(page_data_size), ::free);

		Page* page = T2T<Page*>(::malloc(page_size));

		in.read(page_data.get(), 0, page_data_size);

		PageMetadata* pageMetadata = metadata_->getPageMetadata(ctr_hash, page_hash);
		pageMetadata->getPageOperations()->deserialize(page_data.get(), page_data_size, T2T<void*>(page));

		if (map.find(page->uuid()) == map.end())
		{
			map[page->uuid()] = page;
		}
		else {
			throw vapi::Exception(MA_SRC, SBuf()<<"Page "<<page->uuid()<<" was already registered");
		}
	}


	void read_leaf_node(InputStreamHandler& in, PersistentTreeNodeMap& map)
	{
		LeafNodeBufferT* buffer = new LeafNodeBufferT();

		buffer->read(in);

		if (map.find(buffer->node_id()) == map.end())
		{
			NodeBaseT* node = new LeafNodeT();
			node->populate_as_buffer(buffer);

			map[buffer->node_id()] = std::make_pair(buffer, node);
		}
		else {
			throw vapi::Exception(MA_SRC, SBuf()<<"PersistentTree LeafNode "<<buffer->node_id()<<" was already registered");
		}
	}


	void read_branch_node(InputStreamHandler& in, PersistentTreeNodeMap& map)
	{
		BranchNodeBufferT* buffer = new BranchNodeBufferT();

		buffer->read(in);

		if (map.find(buffer->node_id()) == map.end())
		{
			NodeBaseT* node = new BranchNodeT();
			node->populate_as_buffer(buffer);

			map[buffer->node_id()] = std::make_pair(buffer, node);
		}
		else {
			throw vapi::Exception(MA_SRC, SBuf()<<"PersistentTree BranchNode "<<buffer->node_id()<<" was already registered");
		}
	}




	void read_history_node(InputStreamHandler& in, HistoryTreeNodeMap& map)
	{
		HistoryNodeBuffer* node = new HistoryNodeBuffer();

		Int status;

		in >> status;

		node->status() = status ? HistoryNode::Status::COMMITTED : HistoryNode::Status::ACTIVE;

		in >> node->txn_id();
		in >> node->root();

		in >> node->parent();

		BigInt children;
		in >>children;

		for (BigInt c = 0; c < children; c++)
		{
			TxnId child_txn_id;
			in >> child_txn_id;

			node->children().push_back(child_txn_id);
		}

		if (map.find(node->txn_id()) == map.end())
		{
			map[node->txn_id()] = node;
		}
		else {
			throw vapi::Exception(MA_SRC, SBuf()<<"HistoryTree Node "<<node->txn_id()<<" was already registered");
		}
	}







	std::unique_ptr<LeafNodeBufferT> to_leaf_buffer(const LeafNodeT* node)
	{
		std::unique_ptr<LeafNodeBufferT> buf = std::make_unique<LeafNodeBufferT>();

		buf->populate_as_buffer(node);

		for (Int c = 0; c < node->size(); c++)
		{
			buf->data(c) = typename LeafNodeBufferT::Value(
				node->data(c).page()->uuid(),
				node->data(c).txn_id()
			);
		}

		return buf;
	}

	std::unique_ptr<BranchNodeBufferT> to_branch_buffer(const BranchNodeT* node)
	{
		std::unique_ptr<BranchNodeBufferT> buf = std::make_unique<BranchNodeBufferT>();

		buf->populate_as_buffer(node);

		for (Int c = 0; c < node->size(); c++)
		{
			buf->data(c) = node->data(c)->node_id();
		}

		return buf;
	}

	void walk_version_tree(const HistoryNode* node, std::function<void (const HistoryNode*, SnapshotT*)> fn)
	{
		SnapshotT txn(history_tree_, this->shared_from_this());
		fn(node, &txn);

		for (auto child: node->children())
		{
			if (child->is_committed())
			{
				walk_version_tree(child, fn);
			}
		}
	}

	void write(OutputStreamHandler& out, const AllocatorMetadata& meta)
	{
		out << meta.master();
		out << meta.root();
	}

	void write(OutputStreamHandler& out, const HistoryNode* history_node)
	{
		UByte type = TYPE_HISTORY_NODE;
		out << type;
		out << (Int)history_node->status();
		out << history_node->txn_id();
		out << history_node->root()->node_id();

		if (history_node->parent())
		{
			out << history_node->parent()->txn_id();
		}
		else {
			out << UUID();
		}

		out << (BigInt)history_node->children().size();

		for (auto child: history_node->children())
		{
			out << child->txn_id();
		}

		write_persistent_tree(out, history_node->root());
	}

	void write_persistent_tree(OutputStreamHandler& out, const NodeBaseT* node)
	{
		const auto& txn_id = node->txn_id();

		if (node->is_leaf())
		{
			auto leaf = PersistentTreeT::to_leaf_node(node);
			auto buf  = to_leaf_buffer(leaf);

			write(out, buf.get());

			for (Int c = 0; c < leaf->size(); c++)
			{
				if (leaf->data(c).txn_id() == txn_id)
				{
					write(out, leaf->data(c).page());
				}
			}
		}
		else {
			auto branch = PersistentTreeT::to_branch_node(node);
			auto buf    = to_branch_buffer(branch);

			write(out, buf.get());

			for (Int c = 0; c < branch->size(); c++)
			{
				auto child = branch->data(c);

				if (child->txn_id() == txn_id)
				{
					write_persistent_tree(out, child);
				}
			}
		}
	}


	void write(OutputStreamHandler& out, const BranchNodeBufferT* node)
	{
		UByte type = TYPE_BRANCH_NODE;
		out << type;
		node->write(out);
	}

	void write(OutputStreamHandler& out, const LeafNodeBufferT* node)
	{
		UByte type = TYPE_LEAF_NODE;
		out << type;
		node->write(out);
	}

	void write(OutputStreamHandler& out, const PageType* page)
	{
		UByte type = TYPE_DATA_PAGE;
		out << type;

        PageMetadata* pageMetadata = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

        auto page_size = page->page_size();
        unique_ptr<Byte, void (*)(void*)> buffer((Byte*)::malloc(page_size), ::free);

        const IPageOperations* operations = pageMetadata->getPageOperations();

        Int total_data_size = operations->serialize(page, buffer.get());

        out << total_data_size;
        out << page->page_size();
        out << page->ctr_type_hash();
        out << page->page_type_hash();

        out.write(buffer.get(), 0, total_data_size);
	}
};


template <typename Profile = DefaultProfile<>>
using PersistentInMemAllocator = class PersistentInMemAllocatorT<
		Profile,
		typename ContainerCollectionCfg<Profile>::Types::Page
>;



}



#endif
