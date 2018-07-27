
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

#include <memoria/v1/core/tools/pool.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/stream.hpp>
#include <memoria/v1/core/tools/pair.hpp>
#include <memoria/v1/core/tools/latch.hpp>
#include <memoria/v1/core/tools/memory.hpp>
#include <memoria/v1/core/memory/malloc.hpp>

#include <memoria/v1/core/tools/pair.hpp>

#include <memoria/v1/allocators/inmem/common/container_collection_cfg.hpp>


#include "persistent_tree_node.hpp"
#include "persistent_tree.hpp"
#include "snapshot_base.hpp"


#include <stdlib.h>
#include <memory>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>



namespace memoria {
namespace v1 {

namespace persistent_inmem {
namespace _ {

	template <typename PageT>
	struct PagePtr {
		using RefCntT = int64_t;
	private:
		PageT* page_;
		std::atomic<RefCntT> refs_;

		// Currently for debug purposes
		static std::atomic<int64_t> page_cnt_;
	public:
		PagePtr(PageT* page, int64_t refs): page_(page), refs_(refs) {
			//page_cnt_++;
			//cout << "Create page: " << page_cnt_ << endl;
		}

		~PagePtr() {
			//if (--page_cnt_ == 0) {
			//	cout << "All Pages removed" << endl;
			//}
			//cout << "Remove page: " << page_cnt_ << endl;

            free_system(page_);
		}

		PageT* raw_data() {return page_;}
		const PageT* raw_data() const {return page_;}

		PageT* operator->() {return page_;}
		const PageT* operator->() const {return page_;}

		void clear() {
			page_ = nullptr;
		}

		void ref() {
			refs_++;
		}

		RefCntT references() const {return refs_;}

		RefCntT unref() {
			return --refs_;
		}
	};

	template <typename PageT>
	std::atomic<int64_t> PagePtr<PageT>::page_cnt_(0);


	template <typename ValueT, typename TxnIdT>
	class PersistentTreeValue {
		ValueT page_;
		TxnIdT txn_id_;
	public:
		using Value = ValueT;

		PersistentTreeValue(): page_(), txn_id_() {}
		PersistentTreeValue(const ValueT& page, const TxnIdT& txn_id): page_(page), txn_id_(txn_id) {}

		const ValueT& page_ptr() const {return page_;}
		ValueT& page_ptr() {return page_;}

		const TxnIdT& txn_id() const {return txn_id_;}
		TxnIdT& txn_id() {return txn_id_;}
	};

}

}

template <typename V, typename T>
OutputStreamHandler& operator<<(OutputStreamHandler& out, const persistent_inmem::_::PersistentTreeValue<V, T>& value)
{
    out << value.page_ptr();
    out << value.txn_id();
    return out;
}

template <typename V, typename T>
InputStreamHandler& operator>>(InputStreamHandler& in, persistent_inmem::_::PersistentTreeValue<V, T>& value)
{
    in >> value.page_ptr();
    in >> value.txn_id();
    return in;
}

template <typename V, typename T>
std::ostream& operator<<(std::ostream& out, const persistent_inmem::_::PersistentTreeValue<V, T>& value)
{
    out << "PersistentTreeValue[";
    out << value.page_ptr()->raw_data();
    out << ", ";
    out << value.page_ptr()->references();
    out << ", ";
    out << value.txn_id();
    out << "]";

    return out;
}    

namespace persistent_inmem {

template <typename Profile, typename MyType>
class InMemAllocatorBase: public EnableSharedFromThis<MyType> {
public:
    using PageType = ProfilePageType<Profile>;

    static constexpr int32_t NodeIndexSize  = 32;
    static constexpr int32_t NodeSize       = NodeIndexSize * 32;

    using Page          = PageType;
    using RCPagePtr		= persistent_inmem::_::PagePtr<Page>;

    using Key           = typename PageType::ID;
    using Value         = PageType*;

    using TxnId             = UUID;
    using PTreeNodeId       = UUID;

    using LeafNodeT         = persistent_inmem::LeafNode<Key, persistent_inmem::_::PersistentTreeValue<RCPagePtr*, TxnId>, NodeSize, NodeIndexSize, PTreeNodeId, TxnId>;
    using LeafNodeBufferT   = persistent_inmem::LeafNode<Key, persistent_inmem::_::PersistentTreeValue<typename PageType::ID, TxnId>, NodeSize, NodeIndexSize, PTreeNodeId, TxnId>;

    using BranchNodeT       = persistent_inmem::BranchNode<Key, NodeSize, NodeIndexSize, PTreeNodeId, TxnId>;
    using BranchNodeBufferT = persistent_inmem::BranchNode<Key, NodeSize, NodeIndexSize, PTreeNodeId, TxnId, PTreeNodeId>;
    using NodeBaseT         = typename BranchNodeT::NodeBaseT;
    using NodeBaseBufferT   = typename BranchNodeBufferT::NodeBaseT;



    struct HistoryNode {

    	using Status 		= SnapshotStatus;
        using HMutexT 		= typename MyType::SnapshotMutexT;
        using HLockGuardT 	= typename MyType::SnapshotLockGuardT;

    private:
        MyType* allocator_;

        HistoryNode* parent_;
        U16String metadata_;

        std::vector<HistoryNode*> children_;

        NodeBaseT* root_;

        typename PageType::ID root_id_;

        Status status_;

        TxnId txn_id_;

        int64_t references_ = 0;

        mutable HMutexT mutex_;

    public:

        HistoryNode(MyType* allocator, Status status = Status::ACTIVE):
        	allocator_(allocator),
            parent_(nullptr),
            root_(nullptr),
            root_id_(),
            status_(status),
            txn_id_(UUID::make_random())
        {
            if (parent_) {
                parent_->children().push_back(this);
            }
        }

        HistoryNode(HistoryNode* parent, Status status = Status::ACTIVE):
        	allocator_(parent->allocator_),
			parent_(parent),
			root_(nullptr),
			root_id_(),
			status_(status),
			txn_id_(UUID::make_random())
        {
        	if (parent_) {
        		parent_->children().push_back(this);
        	}
        }

        HistoryNode(MyType* allocator, const TxnId& txn_id, HistoryNode* parent, Status status):
        	allocator_(allocator),
            parent_(parent),
            root_(nullptr),
            root_id_(),
            status_(status),
            txn_id_(txn_id)
        {
            if (parent_) {
                parent_->children().push_back(this);
            }
        }


        auto* allocator() {
        	return allocator_;
        }

        const auto* callocator() const {
        	return allocator_;
        }

        auto& allocator_mutex() {return allocator_->mutex_;}
        const auto& allocator_mutex() const {return allocator_->mutex_;}

        auto& store_mutex() {return allocator_->store_mutex_;}
        const auto& store_mutex() const {return allocator_->store_mutex_;}

        auto& snapshot_mutex() {return mutex_;}
        const auto& snapshot_mutex() const {return mutex_;}

        void remove_child(HistoryNode* child)
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

        void remove_from_parent() {
            if (parent_) {
                parent_->remove_child(this);
            }
        }

        bool is_committed() const
        {
            return status_ == Status::COMMITTED;
        }

        bool is_active() const
        {
            return status_ == Status::ACTIVE;
        }

        bool is_dropped() const
        {
            return status_ == Status::DROPPED;
        }

        bool is_data_locked() const
        {
        	return status_ == Status::DATA_LOCKED;
        }

        const TxnId& txn_id() const {return txn_id_;}

        const auto& root() const {
        	return root_;
        }

        auto& root_id() {return root_id_;}
        const auto& root_id() const {return root_id_;}

        void set_root(NodeBaseT* new_root)
        {
            if (root_) {
                root_->unref();
            }

            root_ = new_root;

            if (root_) {
                root_->ref();
            }
        }

        void assign_root_no_ref(NodeBaseT* new_root) {
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



        const Status& status() const {
        	return status_;
        }

        void set_status(Status& status) {
        	status_ = status;
        }


        void set_metadata(U16StringRef metadata) {
        	metadata_ = metadata;
        }

        const auto& metadata() const {
        	HLockGuardT lock(mutex_);
        	return metadata_;
        }

        void commit() {
            status_ = Status::COMMITTED;
        }

        void mark_to_clear() {
            status_ = Status::DROPPED;
        }

        void lock_data() {
        	status_ = Status::DATA_LOCKED;
        }


        auto references()  const
        {
        	return references_;
        }

        auto ref() {
            return ++references_;
        }

        auto unref() {
            return --references_;
        }
    };

    struct HistoryNodeBuffer {

        template <typename, typename>
        friend class PersistentInMemAllocatorT;

        template <typename, typename>
        friend class InMemAllocatorBase;
        
        template <typename, typename, typename>
        friend class SnapshotBase;
        
    private:
        TxnId parent_;

        U16String metadata_;

        std::vector<TxnId> children_;

        PTreeNodeId root_;
        typename PageType::ID root_id_;

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

        const auto& root() const {return root_;}

        auto& root() {return root_;}

        auto& root_id() {return root_id_;}
        const auto& root_id() const {return root_id_;}

        const auto& parent() const {return parent_;}
        auto& parent() {return parent_;}

        auto& children() {
            return children_;
        }

        const auto& children() const {
            return children_;
        }

        auto& status() {return status_;}
        const auto& status() const {return status_;}

        auto& metadata() {return metadata_;}
        const auto& metadata() const {return metadata_;}
    };


    using PersistentTreeT       = typename persistent_inmem::PersistentTree<BranchNodeT, LeafNodeT, HistoryNode, PageType>;
    using TxnMap                = std::unordered_map<TxnId, HistoryNode*>;

    using HistoryTreeNodeMap    = std::unordered_map<PTreeNodeId, HistoryNodeBuffer*>;
    using PersistentTreeNodeMap = std::unordered_map<PTreeNodeId, std::pair<NodeBaseBufferT*, NodeBaseT*>>;
    using PageMap               = std::unordered_map<typename PageType::ID, RCPagePtr*>;
    using RCPageSet             = std::unordered_set<RCPagePtr*>;
    using BranchMap             = std::unordered_map<U16String, HistoryNode*>;
    using ReverseBranchMap      = std::unordered_map<const HistoryNode*, U16String>;

    template <typename, typename>
    friend class ThreadSnapshot;
    
    template <typename, typename>
    friend class Snapshot;

    template <typename, typename, typename>
    friend class SnapshotBase;
    
protected:
 
    class AllocatorMetadata {
        TxnId master_;
        TxnId root_;

        std::unordered_map<U16String, TxnId> named_branches_;

    public:
        AllocatorMetadata() {}

        TxnId& master() {return master_;}
        TxnId& root() {return root_;}

        const TxnId& master() const {return master_;}
        const TxnId& root()   const {return root_;}

        auto& named_branches() {return named_branches_;}
        const auto& named_branches() const {return named_branches_;}
    };

    class Checksum {
        int64_t records_;
    public:
        int64_t& records() {return records_;}
        const int64_t& records() const {return records_;}
    };

    enum {TYPE_UNKNOWN, TYPE_METADATA, TYPE_HISTORY_NODE, TYPE_BRANCH_NODE, TYPE_LEAF_NODE, TYPE_DATA_PAGE, TYPE_CHECKSUM};

    Logger logger_;

    HistoryNode* history_tree_  = nullptr;
    HistoryNode* master_ 		= nullptr;

    TxnMap snapshot_map_;

    BranchMap named_branches_;

    ContainerMetadataRepository* metadata_;

    int64_t records_ = 0;

    PairPtr pair_;

    ReverseBranchMap snapshot_labels_metadata_;

public:
    InMemAllocatorBase():
        logger_("PersistentInMemAllocator"),
        metadata_(MetadataRepository<Profile>::getMetadata())
    {


        master_ = history_tree_ = new HistoryNode(&self(), HistoryNode::Status::ACTIVE);

        snapshot_map_[history_tree_->txn_id()] = history_tree_;

        auto leaf = new LeafNodeT(history_tree_->txn_id(), UUID::make_random());
        history_tree_->set_root(leaf);
    }

protected:
    InMemAllocatorBase(int32_t):
        metadata_(MetadataRepository<Profile>::getMetadata())
    {}



public:

    virtual ~InMemAllocatorBase()
    {
        
    }

    auto get_root_snapshot_uuid() const {
        return history_tree_->txn_id();
    }


    PairPtr& pair() {
        return pair_;
    }

    const PairPtr& pair() const {
        return pair_;
    }

    // return true in case of errors
    bool check() {
        return false;
    }

    const Logger& logger() const {
        return logger_;
    }

    Logger& logger() {
        return logger_;
    }



    static auto create()
    {
        return alloc_make_shared<MyType>();
    }

    auto newId()
    {
        return PageType::ID::make_random();
    }




    static AllocSharedPtr<MyType> load(InputStreamHandler *input)
    {
        auto alloc_ptr = MakeLocalShared<MyType>(0);

        MyType* allocator = alloc_ptr.get();

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
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"The stream does not start from MEMORIA signature: {}", signature));
        }

        if (!(signature[7] == 0 || signature[7] == 1))
        {
            MMA1_THROW(BoundsException()) << WhatInfo(fmt::format8(u"Endiannes filed value is out of bounds {}", signature[7]));
        }

        if (signature[8] != 0)
        {
            MMA1_THROW(Exception()) << WhatCInfo("This is not an in-memory container");
        }

        allocator->master_ = allocator->history_tree_ = nullptr;

        allocator->snapshot_map_.clear();

        HistoryTreeNodeMap      history_node_map;
        PersistentTreeNodeMap   ptree_node_map;
        PageMap                 page_map;

        AllocatorMetadata metadata;

        Checksum checksum;

        bool proceed = true;

        while (proceed)
        {
            uint8_t type;
            *input >> type;

            switch (type)
            {
                case TYPE_METADATA:     allocator->read_metadata(*input, metadata); break;
                case TYPE_DATA_PAGE:    allocator->read_data_page(*input, page_map); break;
                case TYPE_LEAF_NODE:    allocator->read_leaf_node(*input, ptree_node_map); break;
                case TYPE_BRANCH_NODE:  allocator->read_branch_node(*input, ptree_node_map); break;
                case TYPE_HISTORY_NODE: allocator->read_history_node(*input, history_node_map); break;
                case TYPE_CHECKSUM:     allocator->read_checksum(*input, checksum); proceed = false; break;
                default:
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Unknown record type: {}", (int32_t)type));
            }

            allocator->records_++;
        }

        if (allocator->records_ != checksum.records())
        {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Invalid records checksum: actual={}, expected={}", allocator->records_, checksum.records()));
        }

        for (auto& entry: ptree_node_map)
        {
            auto buffer = entry.second.first;
            auto node   = entry.second.second;

            if (buffer->is_leaf())
            {
                LeafNodeBufferT* leaf_buffer = static_cast<LeafNodeBufferT*>(buffer);
                LeafNodeT* leaf_node         = static_cast<LeafNodeT*>(node);

                for (int32_t c = 0; c < leaf_node->size(); c++)
                {
                    const auto& descr = leaf_buffer->data(c);

                    auto page_iter = page_map.find(descr.page_ptr());

                    if (page_iter != page_map.end())
                    {
                        leaf_node->data(c) = typename LeafNodeT::Value(page_iter->second, descr.txn_id());
                    }
                    else {
                        MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Specified uuid {} is not found in the page map", descr.page_ptr()));
                    }
                }
            }
            else {
                BranchNodeBufferT* branch_buffer = static_cast<BranchNodeBufferT*>(buffer);
                BranchNodeT* branch_node         = static_cast<BranchNodeT*>(node);

                for (int32_t c = 0; c < branch_node->size(); c++)
                {
                    const auto& node_id = branch_buffer->data(c);

                    auto iter = ptree_node_map.find(node_id);

                    if (iter != ptree_node_map.end())
                    {
                        branch_node->data(c) = iter->second.second;
                    }
                    else {
                        MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Specified uuid {} is not found in the persistent tree node map", node_id));
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
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Specified master uuid {} is not found in the data", metadata.master()));
        }

        for (auto& entry: metadata.named_branches())
        {
            auto iter = allocator->snapshot_map_.find(entry.second);

            if (iter != allocator->snapshot_map_.end())
            {
                allocator->named_branches_[entry.first] = iter->second;
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Specified snapshot uuid {} is not found", entry.first));
            }
        }

        // Delete temporary buffers

        for (auto& entry: ptree_node_map)
        {
            auto buffer = entry.second.first;

            if (buffer->is_leaf())
            {
                static_cast<LeafNodeBufferT*>(buffer)->del();
            }
            else {
                static_cast<BranchNodeBufferT*>(buffer)->del();
            }
        }

        for (auto& entry: history_node_map)
        {
            auto node = entry.second;
            delete node;
        }

        allocator->do_delete_dropped();
        allocator->pack();

        return alloc_ptr;
    }

protected:
    virtual void walk_version_tree(HistoryNode* node, std::function<void (HistoryNode*)> fn) = 0;
    
    
    const auto& snapshot_labels_metadata() const {
    	return snapshot_labels_metadata_;
    }

    auto& snapshot_labels_metadata() {
    	return snapshot_labels_metadata_;
    }

    const char16_t* get_labels_for(const HistoryNode* node) const
    {
    	auto labels = snapshot_labels_metadata_.find(node);
    	if (labels != snapshot_labels_metadata_.end())
    	{
            return labels->second.data();
    	}
    	else {
    		return nullptr;
    	}
    }

    void build_snapshot_labels_metadata()
    {
    	snapshot_labels_metadata_.clear();

    	walk_version_tree(history_tree_, [&, this](const HistoryNode* node) {
            std::vector<U16String> labels;

    		if (node == history_tree_) {
                labels.emplace_back(u"Root");
    		}

    		if (node == master_)
    		{
                labels.emplace_back(u"Master Head");
    		}


    		for (const auto& e: named_branches_)
    		{
    			if (e.second == node)
    			{
    				labels.emplace_back(e.first);
    			}
    		}

    		if (labels.size() > 0)
    		{
    			std::stringstream labels_str;

    			bool first = true;
    			for (auto& s: labels)
    			{
    				if (!first)
    				{
    					labels_str << ", ";
    				}
    				else {
    					first = true;
    				}

    				labels_str << s;
    			}

                snapshot_labels_metadata_[node] = U8String(labels_str.str()).to_u16();
    		}
    	});
    }


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

            HistoryNode* node = new HistoryNode(&self(), txn_id, parent, buffer->status());

            node->root_id() = buffer->root_id();
            node->set_metadata(buffer->metadata());

            snapshot_map_[txn_id] = node;

            if (!buffer->root().is_null())
            {
                auto ptree_iter = ptree_map.find(buffer->root());

                if (ptree_iter != ptree_map.end())
                {
                    node->assign_root_no_ref(ptree_iter->second.second);

                    for (const auto& child_txn_id: buffer->children())
                    {
                        build_history_tree(child_txn_id, node, history_map, ptree_map);
                    }

                    return node;
                }
                else {
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Specified node_id {} is not found in persistent tree data", buffer->root()));
                }
            }
            else {
                for (const auto& child_txn_id: buffer->children())
                {
                    build_history_tree(child_txn_id, node, history_map, ptree_map);
                }

                return node;
            }
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Specified txn_id {} is not found in history data", txn_id));
        }
    }


    //virtual void free_memory(HistoryNode* node) = 0;


    /**
     * Deletes recursively all data quickly. Must not be used to delete data of a selected snapshot.
     */

    void free_memory(const NodeBaseT* node)
    {
        const auto& txn_id = node->txn_id();
        if (node->is_leaf())
        {
            auto leaf = PersistentTreeT::to_leaf_node(node);

            for (int32_t c = 0; c < leaf->size(); c++)
            {
                auto& data = leaf->data(c);
                if (data.txn_id() == txn_id)
                {
                    free_system(data.page_ptr());
                }
            }

            leaf->del();
        }
        else {
            auto branch = PersistentTreeT::to_branch_node(node);

            for (int32_t c = 0; c < branch->size(); c++)
            {
                auto child = branch->data(c);
                if (child->txn_id() == txn_id)
                {
                    free_memory(child);
                }
            }

            branch->del();
        }
    }

    void read_metadata(InputStreamHandler& in, AllocatorMetadata& metadata)
    {
        in >> metadata.master();
        in >> metadata.root();

        int64_t size;

        in >>size;

        for (int64_t c = 0; c < size; c++)
        {
            U16String name;
            in >> name;

            TxnId value;
            in >> value;

            metadata.named_branches()[name] = value;
        }
    }

    void read_checksum(InputStreamHandler& in, Checksum& checksum)
    {
        in >> checksum.records();
    }

    void read_data_page(InputStreamHandler& in, PageMap& map)
    {
    	typename RCPagePtr::RefCntT references;

    	in >> references;

    	int32_t page_data_size;
        in >> page_data_size;

        int32_t page_size;
        in >> page_size;

        uint64_t ctr_hash;
        in >> ctr_hash;

        uint64_t page_hash;
        in >> page_hash;

        auto page_data = allocate_system<int8_t>(page_data_size);
        Page* page = allocate_system<Page>(page_size).release();

        in.read(page_data.get(), 0, page_data_size);

        auto pageMetadata = metadata_->getPageMetadata(ctr_hash, page_hash);
        pageMetadata->getPageOperations()->deserialize(page_data.get(), page_data_size, T2T<void*>(page));

        if (map.find(page->uuid()) == map.end())
        {
            map[page->uuid()] = new RCPagePtr(page, references);
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Page {} was already registered", page->uuid()));
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
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"PersistentTree LeafNode {} has been already registered", buffer->node_id()));
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
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"PersistentTree BranchNode {} has been already registered", buffer->node_id()));
        }
    }




    void read_history_node(InputStreamHandler& in, HistoryTreeNodeMap& map)
    {
        HistoryNodeBuffer* node = new HistoryNodeBuffer();

        int32_t status;

        in >> status;

        node->status() = static_cast<typename HistoryNode::Status>(status);

        in >> node->txn_id();
        in >> node->root();
        in >> node->root_id();

        in >> node->parent();

        in >> node->metadata();

        int64_t children;
        in >>children;

        for (int64_t c = 0; c < children; c++)
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
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"HistoryTree Node {} has been already registered", node->txn_id()));
        }
    }




//    void update_leaf_references(LeafNodeT* node)
//    {
//        for (int32_t c = 0; c < node->size(); c++)
//        {
//        	node->data(c)->page_ptr()->raw_data()->references() = node->data(c)->page_ptr()->references();
//        }
//    }



    std::unique_ptr<LeafNodeBufferT> to_leaf_buffer(const LeafNodeT* node)
    {
        std::unique_ptr<LeafNodeBufferT> buf = std::make_unique<LeafNodeBufferT>();

        buf->populate_as_buffer(node);

        for (int32_t c = 0; c < node->size(); c++)
        {
        	buf->data(c) = typename LeafNodeBufferT::Value(
                node->data(c).page_ptr()->raw_data()->uuid(),
                node->data(c).txn_id()
            );
        }

        return buf;
    }

    std::unique_ptr<BranchNodeBufferT> to_branch_buffer(const BranchNodeT* node)
    {
        std::unique_ptr<BranchNodeBufferT> buf = std::make_unique<BranchNodeBufferT>();

        buf->populate_as_buffer(node);

        for (int32_t c = 0; c < node->size(); c++)
        {
            buf->data(c) = node->data(c)->node_id();
        }

        return buf;
    }

//  LeafNodeBufferT* to_leaf_buffer(const LeafNodeT* node)
//  {
//      LeafNodeBufferT* buf = new LeafNodeBufferT();
//
//      buf->populate_as_buffer(node);
//
//      for (int32_t c = 0; c < node->size(); c++)
//      {
//          buf->data(c) = typename LeafNodeBufferT::Value(
//                  node->data(c).page()->uuid(),
//                  node->data(c).txn_id()
//          );
//      }
//
//      return buf;
//  }
//
//  BranchNodeBufferT* to_branch_buffer(const BranchNodeT* node)
//  {
//      BranchNodeBufferT* buf = new BranchNodeBufferT();
//
//      buf->populate_as_buffer(node);
//
//      for (int32_t c = 0; c < node->size(); c++)
//      {
//          buf->data(c) = node->data(c)->node_id();
//      }
//
//      return buf;
//  }



    void write_metadata(OutputStreamHandler& out)
    {
        uint8_t type = TYPE_METADATA;
        out << type;

        out << master_->txn_id();
        out << history_tree_->txn_id();

        out << (int64_t) named_branches_.size();

        for (auto& entry: named_branches_)
        {
            out << entry.first;
            out << entry.second->txn_id();
        }

        records_++;
    }

    void write(OutputStreamHandler& out, const Checksum& checksum)
    {
    	uint8_t type = TYPE_CHECKSUM;
        out << type;
        out << checksum.records() + 1;
    }

    void write_history_node(OutputStreamHandler& out, const HistoryNode* history_node, RCPageSet& stored_pages)
    {
    	uint8_t type = TYPE_HISTORY_NODE;
        out << type;
        out << (int32_t)history_node->status();
        out << history_node->txn_id();

        if (history_node->root())
        {
            out << history_node->root()->node_id();
        }
        else {
            out << PTreeNodeId();
        }

        out << history_node->root_id();

        if (history_node->parent())
        {
            out << history_node->parent()->txn_id();
        }
        else {
            out << TxnId();
        }

        out << history_node->metadata();

        out << (int64_t)history_node->children().size();

        for (auto child: history_node->children())
        {
            out << child->txn_id();
        }

        records_++;

        if (history_node->root())
        {
            write_persistent_tree(out, history_node->root(), stored_pages);
        }
    }

    void write_persistent_tree(OutputStreamHandler& out, const NodeBaseT* node, RCPageSet& stored_pages)
    {
        const auto& txn_id = node->txn_id();

        if (node->is_leaf())
        {
            auto leaf = PersistentTreeT::to_leaf_node(node);
            auto buf  = to_leaf_buffer(leaf);

            write(out, buf.get());

            for (int32_t c = 0; c < leaf->size(); c++)
            {
                const auto& data = leaf->data(c);

                if (stored_pages.count(data.page_ptr()) == 0)
                {
                	write(out, data.page_ptr());
                	stored_pages.insert(data.page_ptr());
                }
            }
        }
        else {
            auto branch = PersistentTreeT::to_branch_node(node);
            auto buf    = to_branch_buffer(branch);

            write(out, buf.get());

            for (int32_t c = 0; c < branch->size(); c++)
            {
                auto child = branch->data(c);

                if (child->txn_id() == txn_id || child->references() == 1)
                {
                    write_persistent_tree(out, child, stored_pages);
                }
            }
        }
    }


    void write(OutputStreamHandler& out, const BranchNodeBufferT* node)
    {
        uint8_t type = TYPE_BRANCH_NODE;
        out << type;
        node->write(out);

        records_++;
    }

    void write(OutputStreamHandler& out, const LeafNodeBufferT* node)
    {
        uint8_t type = TYPE_LEAF_NODE;
        out << type;

        node->write(out);

        records_++;
    }

    void write(OutputStreamHandler& out, const RCPagePtr* page_ptr)
    {
    	auto page = page_ptr->raw_data();

        uint8_t type = TYPE_DATA_PAGE;
        out << type;

        out << page_ptr->references();

        auto pageMetadata = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

        auto page_size = page->page_size();

        auto buffer = allocate_system<uint8_t>(page_size);

        const auto operations = pageMetadata->getPageOperations();

        int32_t total_data_size = operations->serialize(page, buffer.get());

        out << total_data_size;
        out << page->page_size();
        out << page->ctr_type_hash();
        out << page->page_type_hash();

        out.write(buffer.get(), 0, total_data_size);

        records_++;
    }


    void dump(const Page* page, std::ostream& out = std::cout)
    {
        TextPageDumper dumper(out);

        auto meta = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

        meta->getPageOperations()->generateDataEvents(page, DataEventsParams(), &dumper);
    }

    virtual void forget_snapshot(HistoryNode* history_node) = 0;
    

    auto get_named_branch_nodeset() const
    {
    	std::unordered_set<HistoryNode*> set;

    	for (const auto& branch: named_branches_)
    	{
    		set.insert(branch.second);
    	}

    	return set;
    }

    void do_pack(HistoryNode* node)
    {
        std::unordered_set<HistoryNode*> branches = get_named_branch_nodeset();
    	do_pack(history_tree_, 0, branches);

    	for (const auto& branch: named_branches_)
    	{
    		auto node = branch.second;
    		if (node->root() == nullptr && node->references() == 0)
    		{
    			do_remove_history_node(node);
    		}
    	}
    }
    
    virtual void do_pack(HistoryNode* node, int32_t depth, const std::unordered_set<HistoryNode*>& branches) = 0;

    bool do_remove_history_node(HistoryNode* node)
    {
        if (node->children().size() == 0)
        {
            node->remove_from_parent();
            delete node;

            return true;
        }
        else if (node->children().size() == 1)
        {
        	auto parent = node->parent();
            auto child  = node->children()[0];

            node->remove_from_parent();

            delete node;

            parent->children().push_back(child);
            child->parent() = parent;

            return true;
        }

        return false;
    }

    void adjust_named_references(HistoryNode* node)
    {
        HistoryNode* parent = node->parent();
        while (parent && parent->status() != HistoryNode::Status::COMMITTED)
        {
            parent = parent->parent();
        }

        if (parent)
        {
            std::unordered_set<U16String> branch_set_;
            for (auto& pair: named_branches_)
            {
                if (pair.second == node)
                {
                    branch_set_.insert(pair.first);
                }
            }

            for(auto& branch_name: branch_set_)
            {
                named_branches_[branch_name] = parent;
            }

            if (master_ == node)
            {
                master_ = parent;
            }
        }
    }


    MyType& self() {return *static_cast<MyType*>(this);}
    const MyType& self() const {return *static_cast<const MyType*>(this);}
};

}
/*
template <typename Profile = DefaultProfile<>>
using PersistentInMemAllocator = class persistent_inmem_thread::ThreadInMemAllocatorImpl<Profile>;



template <typename Profile>
ThreadInMemAllocator<Profile>::ThreadInMemAllocator() {}

template <typename Profile>
ThreadInMemAllocator<Profile>::ThreadInMemAllocator(AllocSharedPtr<PImpl> impl): pimpl_(impl) {}

template <typename Profile>
ThreadInMemAllocator<Profile>::ThreadInMemAllocator(ThreadInMemAllocator&& other): pimpl_(std::move(other.pimpl_)) {}

template <typename Profile>
ThreadInMemAllocator<Profile>::ThreadInMemAllocator(const ThreadInMemAllocator& other): pimpl_(other.pimpl_) {}

template <typename Profile>
ThreadInMemAllocator<Profile>& ThreadInMemAllocator<Profile>::operator=(const ThreadInMemAllocator& other) 
{
    pimpl_ = other.pimpl_;
    return *this;
}

template <typename Profile>
ThreadInMemAllocator<Profile>& ThreadInMemAllocator<Profile>::operator=(ThreadInMemAllocator&& other) 
{
    pimpl_ = std::move(other.pimpl_);
    return *this;
}

template <typename Profile>
ThreadInMemAllocator<Profile>::~ThreadInMemAllocator() {}


template <typename Profile>
bool ThreadInMemAllocator<Profile>::operator==(const ThreadInMemAllocator& other) const
{
    return pimpl_ == other.pimpl_;
}


template <typename Profile>
ThreadInMemAllocator<Profile>::operator bool() const 
{
    return pimpl_ != nullptr;
}



template <typename Profile>
ThreadInMemAllocator<Profile> ThreadInMemAllocator<Profile>::load(InputStreamHandler* input_stream) 
{
    return ThreadInMemAllocator<Profile>(PImpl::load(input_stream));
}

template <typename Profile>
ThreadInMemAllocator<Profile> ThreadInMemAllocator<Profile>::load(boost::filesystem::path file_name) 
{
    return ThreadInMemAllocator<Profile>(PImpl::load(file_name.string().c_str()));
}

template <typename Profile>
ThreadInMemAllocator<Profile> ThreadInMemAllocator<Profile>::create() 
{
    return ThreadInMemAllocator<Profile>(PImpl::create());
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::store(boost::filesystem::path file_name) 
{
    pimpl_->store(file_name.string().c_str());
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::store(OutputStreamHandler* output_stream) 
{
    pimpl_->store(output_stream);
}

template <typename Profile>
typename ThreadInMemAllocator<Profile>::SnapshotPtr ThreadInMemAllocator<Profile>::master() 
{
    return SnapshotPtr(pimpl_->master());
}


template <typename Profile>
typename ThreadInMemAllocator<Profile>::SnapshotPtr ThreadInMemAllocator<Profile>::find(const TxnId& snapshot_id) 
{
    return pimpl_->find(snapshot_id);
}

template <typename Profile>
typename ThreadInMemAllocator<Profile>::SnapshotPtr ThreadInMemAllocator<Profile>::find_branch(StringRef name) 
{
    return pimpl_->find_branch(name);
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::set_master(const TxnId& txn_id) 
{
    pimpl_->set_master(txn_id);
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::set_branch(StringRef name, const TxnId& txn_id) 
{
    pimpl_->set_branch(name, txn_id);
}

template <typename Profile>
ContainerMetadataRepository* ThreadInMemAllocator<Profile>::metadata() const 
{
    return pimpl_->getMetadata();
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::walk_containers(ContainerWalker* walker, const char* allocator_descr) 
{
     return pimpl_->walkContainers(walker, allocator_descr);
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::dump(boost::filesystem::path dump_at) 
{
    pimpl_->dump(dump_at.string().c_str());
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::reset() 
{
    pimpl_.reset();
}


template <typename Profile>
void ThreadInMemAllocator<Profile>::pack() 
{
    return pimpl_->pack();
}

template <typename Profile>
Logger& ThreadInMemAllocator<Profile>::logger()
{
    return pimpl_->logger();
}


template <typename Profile>
bool ThreadInMemAllocator<Profile>::check() 
{
    return pimpl_->check();
}*/

}}

