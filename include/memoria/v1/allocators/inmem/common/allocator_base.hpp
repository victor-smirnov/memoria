
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
#include <memoria/v1/core/tools/stream.hpp>
#include <memoria/v1/core/tools/pair.hpp>
#include <memoria/v1/core/tools/latch.hpp>
#include <memoria/v1/core/tools/memory.hpp>
#include <memoria/v1/core/memory/malloc.hpp>

#include <memoria/v1/core/tools/pair.hpp>

#include <memoria/v1/profiles/common/common.hpp>
#include <memoria/v1/profiles/common/metadata.hpp>

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

    template <typename BlockT>
    struct BlockPtr {
		using RefCntT = int64_t;
	private:
        BlockT* block_;
		std::atomic<RefCntT> refs_;

		// Currently for debug purposes
		static std::atomic<int64_t> block_cnt_;
	public:
        BlockPtr(BlockT* block, int64_t refs): block_(block), refs_(refs) {}

        ~BlockPtr() {
            free_system(block_);
		}

        BlockT* raw_data() {return block_;}
        const BlockT* raw_data() const {return block_;}

        BlockT* operator->() {return block_;}
        const BlockT* operator->() const {return block_;}

		void clear() {
            block_ = nullptr;
		}

		void ref() {
			refs_++;
		}

		RefCntT references() const {return refs_;}

		RefCntT unref() {
			return --refs_;
		}
	};

    template <typename BlockT>
    std::atomic<int64_t> BlockPtr<BlockT>::block_cnt_(0);


    template <typename ValueT, typename SnapshotIdT>
	class PersistentTreeValue {
		ValueT block_;
        SnapshotIdT snapshot_id_;
	public:
		using Value = ValueT;

        PersistentTreeValue(): block_(), snapshot_id_() {}
        PersistentTreeValue(const ValueT& block, const SnapshotIdT& snapshot_id): block_(block), snapshot_id_(snapshot_id) {}

		const ValueT& block_ptr() const {return block_;}
		ValueT& block_ptr() {return block_;}

        const SnapshotIdT& snapshot_id() const {return snapshot_id_;}
        SnapshotIdT& snapshot_id() {return snapshot_id_;}
	};

}

}

template <typename V, typename T>
OutputStreamHandler& operator<<(OutputStreamHandler& out, const persistent_inmem::_::PersistentTreeValue<V, T>& value)
{
    out << value.block_ptr();
    out << value.snapshot_id();
    return out;
}

template <typename V, typename T>
InputStreamHandler& operator>>(InputStreamHandler& in, persistent_inmem::_::PersistentTreeValue<V, T>& value)
{
    in >> value.block_ptr();
    in >> value.snapshot_id();
    return in;
}

template <typename V, typename T>
std::ostream& operator<<(std::ostream& out, const persistent_inmem::_::PersistentTreeValue<V, T>& value)
{
    out << "PersistentTreeValue[";
    out << value.block_ptr()->raw_data();
    out << ", ";
    out << value.block_ptr()->references();
    out << ", ";
    out << value.snapshot_id();
    out << "]";

    return out;
}    

namespace persistent_inmem {

template <typename Profile, typename MyType>
class InMemAllocatorBase: public EnableSharedFromThis<MyType> {
public:
    using BlockType = ProfileBlockType<Profile>;

    static constexpr int32_t NodeIndexSize  = 32;
    static constexpr int32_t NodeSize       = NodeIndexSize * 32;

    using RCBlockPtr    = persistent_inmem::_::BlockPtr<BlockType>;

    using Key           = ProfileBlockID<Profile>;
    using Value         = BlockType*;

    using SnapshotID        = ProfileSnapshotID<Profile>;
    using BlockID           = ProfileBlockID<Profile>;

    using LeafNodeT         = persistent_inmem::LeafNode<Key, persistent_inmem::_::PersistentTreeValue<RCBlockPtr*, SnapshotID>, NodeSize, NodeIndexSize, BlockID, SnapshotID>;
    using LeafNodeBufferT   = persistent_inmem::LeafNode<Key, persistent_inmem::_::PersistentTreeValue<BlockID, SnapshotID>, NodeSize, NodeIndexSize, BlockID, SnapshotID>;

    using BranchNodeT       = persistent_inmem::BranchNode<Key, NodeSize, NodeIndexSize, BlockID, SnapshotID>;
    using BranchNodeBufferT = persistent_inmem::BranchNode<Key, NodeSize, NodeIndexSize, BlockID, SnapshotID, BlockID>;
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

        BlockID root_id_{};

        Status status_;

        SnapshotID snapshot_id_;

        int64_t references_ = 0;

        mutable HMutexT mutex_;

        int64_t uses_ = 0;

    public:

        HistoryNode(MyType* allocator, Status status = Status::ACTIVE):
        	allocator_(allocator),
            parent_(nullptr),
            root_(nullptr),
            status_(status),
            snapshot_id_(IDTools<SnapshotID>::make_random())
        {
            if (parent_) {
                parent_->children().push_back(this);
            }
        }

        HistoryNode(HistoryNode* parent, Status status = Status::ACTIVE):
        	allocator_(parent->allocator_),
			parent_(parent),
			root_(nullptr),
			status_(status),
            snapshot_id_(IDTools<SnapshotID>::make_random())
        {
        	if (parent_) {
        		parent_->children().push_back(this);
        	}
        }

        HistoryNode(MyType* allocator, const SnapshotID& snapshot_id, HistoryNode* parent, Status status):
        	allocator_(allocator),
            parent_(parent),
            root_(nullptr),
            root_id_(),
            status_(status),
            snapshot_id_(snapshot_id)
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

        Status status() const {
            return status_;
        }

        const SnapshotID& snapshot_id() const {return snapshot_id_;}

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

        BlockID new_node_id() {
            return IDTools<BlockID>::make_random();
        }

        const auto& parent() const {return parent_;}
        auto& parent() {return parent_;}

        auto& children() {
            return children_;
        }

        const auto& children() const {
            return children_;
        }


        void set_status(const Status& status) {
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
        SnapshotID parent_;

        U16String metadata_;

        std::vector<SnapshotID> children_;

        BlockID root_{};
        BlockID root_id_{};

        typename HistoryNode::Status status_;

        SnapshotID snapshot_id_;

    public:

        HistoryNodeBuffer(){}

        const SnapshotID& snapshot_id() const {
            return snapshot_id_;
        }

        SnapshotID& snapshot_id() {
            return snapshot_id_;
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


    using PersistentTreeT       = typename persistent_inmem::PersistentTree<BranchNodeT, LeafNodeT, HistoryNode, BlockType>;
    using SnapshotMap           = std::unordered_map<SnapshotID, HistoryNode*>;

    using HistoryTreeNodeMap    = std::unordered_map<BlockID, HistoryNodeBuffer*>;
    using PersistentTreeNodeMap = std::unordered_map<BlockID, std::pair<NodeBaseBufferT*, NodeBaseT*>>;
    using BlockMap              = std::unordered_map<BlockID, RCBlockPtr*>;
    using RCBlockSet            = std::unordered_set<const void*>;
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
        SnapshotID master_;
        SnapshotID root_;

        std::unordered_map<U16String, SnapshotID> named_branches_;

    public:
        AllocatorMetadata() {}

        SnapshotID& master() {return master_;}
        SnapshotID& root() {return root_;}

        const SnapshotID& master() const {return master_;}
        const SnapshotID& root()   const {return root_;}

        auto& named_branches() {return named_branches_;}
        const auto& named_branches() const {return named_branches_;}
    };

    class Checksum {
        int64_t records_;
    public:
        int64_t& records() {return records_;}
        const int64_t& records() const {return records_;}
    };

    enum {TYPE_UNKNOWN, TYPE_METADATA, TYPE_HISTORY_NODE, TYPE_BRANCH_NODE, TYPE_LEAF_NODE, TYPE_DATA_BLOCK, TYPE_CHECKSUM};

    Logger logger_;

    HistoryNode* history_tree_  = nullptr;
    HistoryNode* master_ 		= nullptr;

    SnapshotMap snapshot_map_;

    BranchMap named_branches_;

    int64_t records_ = 0;

    PairPtr pair_;

    ReverseBranchMap snapshot_labels_metadata_;

    std::atomic<bool> dump_snapshot_lifecycle_{false};

public:
    InMemAllocatorBase():
        logger_("PersistentInMemAllocator")
    {
        master_ = history_tree_ = new HistoryNode(&self(), HistoryNode::Status::ACTIVE);

        snapshot_map_[history_tree_->snapshot_id()] = history_tree_;

        auto leaf = new LeafNodeT(history_tree_->snapshot_id(), IDTools<BlockID>::make_random());
        history_tree_->set_root(leaf);
    }

protected:
    InMemAllocatorBase(int32_t)
    {}

public:

    virtual ~InMemAllocatorBase()
    {
        
    }

    bool isDumpSnapshotLifecycle() {return dump_snapshot_lifecycle_.load();}
    void set_dump_snapshot_lifecycle(bool do_dump) {dump_snapshot_lifecycle_.store(do_dump);}

    auto get_root_snapshot_uuid() const {
        return history_tree_->snapshot_id();
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
        return IDTools<BlockID>::make_random();
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
        BlockMap                block_map;

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
                case TYPE_DATA_BLOCK:    allocator->read_data_block(*input, block_map); break;
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

                    auto block_iter = block_map.find(descr.block_ptr());

                    if (block_iter != block_map.end())
                    {
                        leaf_node->data(c) = typename LeafNodeT::Value(block_iter->second, descr.snapshot_id());
                    }
                    else {
                        MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Specified uuid {} is not found in the block map", descr.block_ptr()));
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
        const SnapshotID& snapshot_id,
        HistoryNode* parent,
        const HistoryTreeNodeMap& history_map,
        const PersistentTreeNodeMap& ptree_map
    )
    {
        auto iter = history_map.find(snapshot_id);

        if (iter != history_map.end())
        {
            HistoryNodeBuffer* buffer = iter->second;

            HistoryNode* node = new HistoryNode(&self(), snapshot_id, parent, buffer->status());

            node->root_id() = buffer->root_id();
            node->set_metadata(buffer->metadata());

            snapshot_map_[snapshot_id] = node;

            if (!buffer->root().is_null())
            {
                auto ptree_iter = ptree_map.find(buffer->root());

                if (ptree_iter != ptree_map.end())
                {
                    node->assign_root_no_ref(ptree_iter->second.second);

                    for (const auto& child_snapshot_id: buffer->children())
                    {
                        build_history_tree(child_snapshot_id, node, history_map, ptree_map);
                    }

                    return node;
                }
                else {
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Specified node_id {} is not found in persistent tree data", buffer->root()));
                }
            }
            else {
                for (const auto& child_snapshot_id: buffer->children())
                {
                    build_history_tree(child_snapshot_id, node, history_map, ptree_map);
                }

                return node;
            }
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Specified snapshot_id {} is not found in history data", snapshot_id));
        }
    }


    //virtual void free_memory(HistoryNode* node) = 0;


    /**
     * Deletes recursively all data quickly. Must not be used to delete data of a selected snapshot.
     */

    void free_memory(const NodeBaseT* node)
    {
        const auto& snapshot_id = node->snapshot_id();
        if (node->is_leaf())
        {
            auto leaf = PersistentTreeT::to_leaf_node(node);

            for (int32_t c = 0; c < leaf->size(); c++)
            {
                auto& data = leaf->data(c);
                if (data.snapshot_id() == snapshot_id)
                {
                    free_system(data.block_ptr());
                }
            }

            leaf->del();
        }
        else {
            auto branch = PersistentTreeT::to_branch_node(node);

            for (int32_t c = 0; c < branch->size(); c++)
            {
                auto child = branch->data(c);
                if (child->snapshot_id() == snapshot_id)
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

            SnapshotID value;
            in >> value;

            metadata.named_branches()[name] = value;
        }
    }

    void read_checksum(InputStreamHandler& in, Checksum& checksum)
    {
        in >> checksum.records();
    }

    void read_data_block(InputStreamHandler& in, BlockMap& map)
    {
        typename RCBlockPtr::RefCntT references;

    	in >> references;

    	int32_t block_data_size;
        in >> block_data_size;

        int32_t block_size;
        in >> block_size;

        uint64_t ctr_hash;
        in >> ctr_hash;

        uint64_t block_hash;
        in >> block_hash;

        auto block_data = allocate_system<int8_t>(block_data_size);
        BlockType* block = allocate_system<BlockType>(block_size).release();

        in.read(block_data.get(), 0, block_data_size);

        ProfileMetadata<Profile>::get_thread_local()
                ->get_block_operations(ctr_hash, block_hash)
                ->deserialize(block_data.get(), block_data_size, block);

        if (map.find(block->uuid()) == map.end())
        {
            map[block->uuid()] = new RCBlockPtr(block, references);
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Block {} was already registered", block->uuid()));
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

        in >> node->snapshot_id();
        in >> node->root();
        in >> node->root_id();

        in >> node->parent();

        in >> node->metadata();

        int64_t children;
        in >>children;

        for (int64_t c = 0; c < children; c++)
        {
            SnapshotID child_snapshot_id;
            in >> child_snapshot_id;

            node->children().push_back(child_snapshot_id);
        }

        if (map.find(node->snapshot_id()) == map.end())
        {
            map[node->snapshot_id()] = node;
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"HistoryTree Node {} has been already registered", node->snapshot_id()));
        }
    }




//    void update_leaf_references(LeafNodeT* node)
//    {
//        for (int32_t c = 0; c < node->size(); c++)
//        {
//        	node->data(c)->block_ptr()->raw_data()->references() = node->data(c)->block_ptr()->references();
//        }
//    }



    std::unique_ptr<LeafNodeBufferT> to_leaf_buffer(const LeafNodeT* node)
    {
        std::unique_ptr<LeafNodeBufferT> buf = std::make_unique<LeafNodeBufferT>();

        buf->populate_as_buffer(node);

        for (int32_t c = 0; c < node->size(); c++)
        {
        	buf->data(c) = typename LeafNodeBufferT::Value(
                node->data(c).block_ptr()->raw_data()->uuid(),
                node->data(c).snapshot_id()
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
//                  node->data(c).block()->uuid(),
//                  node->data(c).snapshot_id()
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

        out << master_->snapshot_id();
        out << history_tree_->snapshot_id();

        out << (int64_t) named_branches_.size();

        for (auto& entry: named_branches_)
        {
            out << entry.first;
            out << entry.second->snapshot_id();
        }

        records_++;
    }

    void write(OutputStreamHandler& out, const Checksum& checksum)
    {
    	uint8_t type = TYPE_CHECKSUM;
        out << type;
        out << checksum.records() + 1;
    }

    void write_history_node(OutputStreamHandler& out, const HistoryNode* history_node, RCBlockSet& stored_blocks)
    {
    	uint8_t type = TYPE_HISTORY_NODE;
        out << type;
        out << (int32_t)history_node->status();
        out << history_node->snapshot_id();

        if (history_node->root())
        {
            out << history_node->root()->node_id();
        }
        else {
            out << BlockID{};
        }

        out << history_node->root_id();

        if (history_node->parent())
        {
            out << history_node->parent()->snapshot_id();
        }
        else {
            out << SnapshotID{};
        }

        out << history_node->metadata();

        out << (int64_t)history_node->children().size();

        for (auto child: history_node->children())
        {
            out << child->snapshot_id();
        }

        records_++;

        if (history_node->root())
        {
            write_persistent_tree(out, history_node->root(), stored_blocks);
        }
    }

    void write_persistent_tree(OutputStreamHandler& out, const NodeBaseT* node, RCBlockSet& stored_blocks)
    {
        if (stored_blocks.count(node) == 0)
        {
            stored_blocks.insert(node);

            if (node->is_leaf())
            {
                auto leaf = PersistentTreeT::to_leaf_node(node);
                auto buf  = to_leaf_buffer(leaf);

                write(out, buf.get());

                for (int32_t c = 0; c < leaf->size(); c++)
                {
                    const auto& data = leaf->data(c);

                    if (stored_blocks.count(data.block_ptr()) == 0)
                    {
                        write(out, data.block_ptr());
                        stored_blocks.insert(data.block_ptr());
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
                    write_persistent_tree(out, child, stored_blocks);
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

    void write(OutputStreamHandler& out, const RCBlockPtr* block_ptr)
    {
    	auto block = block_ptr->raw_data();

        uint8_t type = TYPE_DATA_BLOCK;
        out << type;

        out << block_ptr->references();

        auto block_size = block->memory_block_size();

        auto buffer = allocate_system<uint8_t>(block_size);

        int32_t total_data_size = ProfileMetadata<Profile>::get_thread_local()
                ->get_block_operations(block->ctr_type_hash(), block->block_type_hash())
                ->serialize(block, buffer.get());

        out << total_data_size;
        out << block->memory_block_size();
        out << block->ctr_type_hash();
        out << block->block_type_hash();

        out.write(buffer.get(), 0, total_data_size);

        records_++;
    }


    void dump(const BlockType* block, std::ostream& out = std::cout)
    {
        TextBlockDumper dumper(out);

        auto blk_ops = ProfileMetadata<Profile>::get_thread_local()
                ->get_block_operations(block->ctr_type_hash(), block->block_type_hash());

        blk_ops->generateDataEvents(block, DataEventsParams(), &dumper);
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

            if (this->isDumpSnapshotLifecycle()) {
                std::cout << "MEMORIA: do_remove_history_node: " << node->snapshot_id() << std::endl;
            }

            delete node;

            return true;
        }
        else if (node->children().size() == 1)
        {
        	auto parent = node->parent();
            auto child  = node->children()[0];

            node->remove_from_parent();

            if (this->isDumpSnapshotLifecycle()) {
                std::cout << "MEMORIA: do_remove_history_node: " << node->snapshot_id() << std::endl;
            }

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


}}

