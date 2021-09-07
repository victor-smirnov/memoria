
// Copyright 2016-2021 Victor Smirnov
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


#include <memoria/core/tools/stream.hpp>
#include <memoria/core/tools/pair.hpp>
#include <memoria/core/tools/latch.hpp>
#include <memoria/core/memory/ptr_cast.hpp>
#include <memoria/core/memory/memory.hpp>
#include <memoria/core/memory/malloc.hpp>

#include <memoria/core/tools/pair.hpp>

#include <memoria/profiles/common/common.hpp>
#include <memoria/profiles/common/metadata.hpp>
#include <memoria/profiles/impl/cow_lite_profile.hpp>

#include <memoria/api/store/memory_store_api.hpp>

#include <memoria/store/memory_cow/common/snapshot_base_cow.hpp>

#include <stdlib.h>
#include <memory>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>



namespace memoria {

template <typename ValueHolder>
InputStreamHandler& operator>>(InputStreamHandler& in, CowBlockID<ValueHolder>& value)
{
    in >> value.value();
    return in;
}


static inline InputStreamHandler& operator>>(InputStreamHandler& in, CowBlockID<UUID>& value)
{
    in >> value.value().lo();
    in >> value.value().hi();
    return in;
}

namespace store {
namespace memory_cow {

template <typename Profile, typename MyType>
class MemoryStoreBase: public IMemoryStore<ApiProfile<Profile>>, public EnableSharedFromThis<MyType> {
public:
    using BlockType = ProfileBlockType<Profile>;
    using NodeBaseT = BlockType;

    static constexpr int32_t NodeIndexSize  = 32;
    static constexpr int32_t NodeSize       = NodeIndexSize * 32;
    static constexpr uint64_t PROFILE_HASH = TypeHash<Profile>::Value;

    using Key           = ProfileBlockID<Profile>;
    using Value         = BlockType*;

    using SnapshotID        = ProfileSnapshotID<Profile>;
    using BlockID           = ProfileBlockID<Profile>;
    using CtrID             = ProfileCtrID<Profile>;


    struct HistoryNode {

    	using Status 		= SnapshotStatus;
        using HMutexT 		= typename MyType::SnapshotMutexT;
        using HLockGuardT 	= typename MyType::SnapshotLockGuardT;

    private:
        MyType* allocator_;

        HistoryNode* parent_;
        U8String metadata_;

        std::vector<HistoryNode*> children_{};

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
            status_(status),
            snapshot_id_(ProfileTraits<Profile>::make_random_snapshot_id())
        {
        }

        HistoryNode(HistoryNode* parent, Status status = Status::ACTIVE):
        	allocator_(parent->allocator_),
			parent_(parent),
            status_(status),
            snapshot_id_(ProfileTraits<Profile>::make_random_snapshot_id())
        {
            if (parent_)
            {
                root_id_ = parent->root_id();
                parent_->children().push_back(this);
        	}
        }

        HistoryNode(MyType* allocator, const SnapshotID& snapshot_id, HistoryNode* parent, Status status):
        	allocator_(allocator),
            parent_(parent),
            status_(status),
            snapshot_id_(snapshot_id)
        {
            if (parent_) {
                root_id_ = parent->root_id();
                parent_->children().push_back(this);
            }
        }

        void ref_root() noexcept {
            if (root_id_.isSet()) {
                ref_root(root_id_);
            }
        }

        void ref_root(BlockID root) noexcept
        {
            BlockType* root_block = detail::IDValueHolderH<BlockID>::template get_block_ptr<BlockType>(root);
            root_block->ref_block();
        }


        auto* store() {
        	return allocator_;
        }

        const auto* cstore() const {
        	return allocator_;
        }

        auto& allocator_mutex() {return allocator_->mutex_;}
        const auto& allocator_mutex() const {return allocator_->mutex_;}

        auto& store_mutex() {return allocator_->store_mutex_;}
        const auto& store_mutex() const {return allocator_->store_mutex_;}

        auto& snapshot_mutex() {return mutex_;}
        const auto& snapshot_mutex() const {return mutex_;}

        void remove_child(HistoryNode* child) noexcept
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

        void remove_from_parent() noexcept {
            if (parent_) {
                parent_->remove_child(this);
            }
        }

        bool is_committed() const noexcept
        {
            return status_ == Status::COMMITTED;
        }

        bool is_active() const noexcept
        {
            return status_ == Status::ACTIVE;
        }

        bool is_dropped() const noexcept
        {
            return status_ == Status::DROPPED;
        }

        Status status() const noexcept {
            return status_;
        }

        const SnapshotID& snapshot_id() const noexcept {return snapshot_id_;}


        BlockID root_id() const noexcept {
            return root_id_;
        }

        void reset_root_id() noexcept {
            root_id_ = BlockID{};
        }

        BlockID update_directory_root(BlockID new_root)
        {
            BlockID to_remove{};

            if (root_id_.isSet())
            {
                BlockType* current_root_block = detail::IDValueHolderH<BlockID>::template get_block_ptr<BlockType>(root_id_);
                if (current_root_block->unref_block())
                {
                    to_remove = root_id_;
                }
            }

            ref_root(new_root);

            root_id_ = new_root;

            return to_remove;
        }

        void assign_root_no_ref(NodeBaseT* new_root) noexcept {
            root_id_ = new_root->id();
        }

        BlockID new_node_id() {
            return ProfileTraits<Profile>::make_random_block_id();
        }

        const auto& parent() const noexcept {return parent_;}
        auto& parent() noexcept {return parent_;}

        auto& children() noexcept {
            return children_;
        }

        const auto& children() const noexcept {
            return children_;
        }


        void set_status(const Status& status) noexcept {
        	status_ = status;
        }


        void set_metadata(U8StringRef metadata) noexcept {
        	metadata_ = metadata;
        }

        const auto& metadata() const noexcept {
        	HLockGuardT lock(mutex_);
        	return metadata_;
        }

        void commit() noexcept {
            status_ = Status::COMMITTED;
        }

        void mark_to_clear() noexcept {
            status_ = Status::DROPPED;
        }

        void lock_data() noexcept {
        	status_ = Status::DATA_LOCKED;
        }


        auto references() const noexcept
        {
        	return references_;
        }

        auto ref() noexcept {
            return ++references_;
        }

        auto unref() noexcept {
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

        U8String metadata_;

        std::vector<SnapshotID> children_;

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


    using SnapshotMap           = std::unordered_map<SnapshotID, HistoryNode*>;

    using HistoryTreeNodeMap    = std::unordered_map<SnapshotID, HistoryNodeBuffer*>;
    using BlockMap              = std::unordered_map<BlockID, BlockType*>;
    using RCBlockSet            = std::unordered_set<const void*>;
    using BranchMap             = std::unordered_map<U8String, HistoryNode*>;
    using ReverseBranchMap      = std::unordered_map<const HistoryNode*, U8String>;

    friend struct HistoryNode;

    template <typename, typename>
    friend class ThreadSnapshot;
    
    template <typename, typename>
    friend class Snapshot;

    template <typename, typename, typename>
    friend class SnapshotBase;

    template <typename>
    friend struct detail::IDValueHolderH;


protected:
 
    class AllocatorMetadata {
        SnapshotID master_;
        SnapshotID root_;
        int64_t id_counter_;

        std::unordered_map<U8String, SnapshotID> named_branches_;

    public:
        AllocatorMetadata() {}

        SnapshotID& master() {return master_;}
        SnapshotID& root() {return root_;}

        const SnapshotID& master() const {return master_;}
        const SnapshotID& root()   const {return root_;}

        auto& named_branches() {return named_branches_;}
        const auto& named_branches() const {return named_branches_;}

        auto& id_counter() {return id_counter_;}
        const auto& id_counter() const {return id_counter_;}
    };

    class Checksum {
        int64_t records_;
    public:
        int64_t& records() {return records_;}
        const int64_t& records() const {return records_;}
    };

    enum {TYPE_UNKNOWN = 0, TYPE_METADATA = 1, TYPE_HISTORY_NODE = 2, TYPE_DATA_BLOCK = 3, TYPE_CHECKSUM = 4};

    HistoryNode* history_tree_  = nullptr;
    HistoryNode* master_ 	= nullptr;

    SnapshotMap snapshot_map_;

    BranchMap named_branches_;

    int64_t records_ = 0;

    PairPtr pair_;

    ReverseBranchMap snapshot_labels_metadata_;

    std::atomic<bool> dump_snapshot_lifecycle_{false};

    uint64_t id_counter_{1};

public:
    MemoryStoreBase(MaybeError& maybe_error)
    {
        wrap_construction(maybe_error, [&](){
            master_ = history_tree_ = new HistoryNode(&self(), HistoryNode::Status::ACTIVE);
            snapshot_map_[history_tree_->snapshot_id()] = history_tree_;
        });
    }

protected:
    MemoryStoreBase(int32_t)
    {}

public:

    virtual ~MemoryStoreBase() noexcept = default;

    bool is_dump_snapshot_lifecycle() const noexcept {return dump_snapshot_lifecycle_.load();}
    void set_dump_snapshot_lifecycle(bool do_dump) noexcept {dump_snapshot_lifecycle_.store(do_dump);}

    auto get_root_snapshot_uuid() const noexcept {
        return history_tree_->snapshot_id();
    }


    PairPtr& pair() noexcept {
        return pair_;
    }

    const PairPtr& pair() const noexcept {
        return pair_;
    }

    void check(const CheckResultConsumerFn&) noexcept {
        //FIXME: Implementation!
    }


    static AllocSharedPtr<IMemoryStore<Profile>> create()
    {
        MaybeError maybe_error;
        auto store = alloc_make_shared<MyType>(maybe_error);

        if (!maybe_error) {
            return std::move(store);
        }
        else {
            std::move(maybe_error.get()).do_throw();
        }
    }

    auto newBlockId() noexcept {
        return detail::IDValueHolderH<BlockID>::new_id(this);
    }

    class IDToMemBlockIDResolver: public IBlockOperations<Profile>::IDValueResolver {
        const BlockMap* block_map_;
        using BlockIDValueHolder = detail::IDValueHolderT<BlockID>;

    public:
        IDToMemBlockIDResolver(const BlockMap* block_map) noexcept:
            block_map_(block_map)
        {}

        virtual BlockID resolve_id(const BlockID& stored_block_id) const noexcept
        {
            auto ii = block_map_->find(stored_block_id);
            if (ii != block_map_->end())
            {
                BlockType* block = ii->second;
                BlockIDValueHolder id_value = value_cast<BlockIDValueHolder>(block);
                return detail::IDValueHolderH<BlockID>::to_id(id_value);
            }

            MEMORIA_MAKE_GENERIC_ERROR("Requested block ID is not found").do_throw();
        }
    };


    static bool is_my_data(InputStreamHandler* input)
    {
        char signature[16] = {};
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
            return false;
        }

        if (!(signature[7] == 0 || signature[7] == 1))
        {
            return false;
        }

        uint64_t profile_hash = *ptr_cast<uint64_t>(signature + 8);

        if (profile_hash != PROFILE_HASH)
        {
            return false;
        }

        return true;
    }

    static AllocSharedPtr<MyType> load(InputStreamHandler *input)
    {
        auto alloc_ptr = MakeLocalShared<MyType>(0);

        MyType* allocator = alloc_ptr.get();

        char signature[16] = {};

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
            std::string sig_str;
            for (int c = 0; c < 7; c++) sig_str.append(1, signature[c]);

            MEMORIA_MAKE_GENERIC_ERROR("The stream does not start from MEMORIA signature: {}", sig_str).do_throw();
        }

        if (!(signature[7] == 0 || signature[7] == 1))
        {
            MEMORIA_MAKE_GENERIC_ERROR("Endiannes filed value is out of bounds {}", (int32_t)signature[7]).do_throw();
        }

        uint64_t profile_hash = *ptr_cast<uint64_t>(signature + 8);

        if (profile_hash != PROFILE_HASH)
        {
            MEMORIA_MAKE_GENERIC_ERROR("Profile hash value does not match").do_throw();
        }

        allocator->master_ = allocator->history_tree_ = nullptr;

        allocator->snapshot_map_.clear();

        HistoryTreeNodeMap      history_node_map;
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
                case TYPE_METADATA:     {
                    allocator->read_metadata(*input, metadata);
                    allocator->id_counter_ = metadata.id_counter();
                    break;
                }
                case TYPE_HISTORY_NODE: allocator->read_history_node(*input, history_node_map); break;
                case TYPE_DATA_BLOCK:   allocator->read_data_block(*input, block_map); break;
                case TYPE_CHECKSUM:     allocator->read_checksum(*input, checksum); proceed = false; break;
                default:
                    MEMORIA_MAKE_GENERIC_ERROR("Unknown record type: {}", (int32_t)type).do_throw();
            }

            allocator->records_++;
        }

        if (allocator->records_ != checksum.records())
        {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid records checksum: actual={}, expected={}", allocator->records_, checksum.records()).do_throw();
        }

        IDToMemBlockIDResolver id_to_mem_resolver(&block_map);

        for (auto& entry: block_map)
        {
            BlockType* block = entry.second;

            auto ctr_hash   = block->ctr_type_hash();
            auto block_hash = block->block_type_hash();

            ProfileMetadata<Profile>::local()
                    ->get_block_operations(ctr_hash, block_hash)
                    ->cow_resolve_ids(block, &id_to_mem_resolver);

        }

        auto tree_node = allocator->build_history_tree(metadata.root(), nullptr, history_node_map, block_map);

        allocator->history_tree_ = tree_node;

        if (allocator->snapshot_map_.find(metadata.master()) != allocator->snapshot_map_.end())
        {
            allocator->master_ = allocator->snapshot_map_[metadata.master()];
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Specified master uuid {} is not found in the data", metadata.master()).do_throw();
        }

        for (auto& entry: metadata.named_branches())
        {
            auto iter = allocator->snapshot_map_.find(entry.second);

            if (iter != allocator->snapshot_map_.end())
            {
                allocator->named_branches_[entry.first] = iter->second;
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Specified snapshot uuid {} is not found", entry.first).do_throw();
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

    const char* get_labels_for(const HistoryNode* node) const
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

        return walk_version_tree(history_tree_, [&, this](const HistoryNode* node) {
            std::vector<U8String> labels;

    		if (node == history_tree_) {
                labels.emplace_back("Root");
    		}

    		if (node == master_)
    		{
                labels.emplace_back("Master Head");
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

                snapshot_labels_metadata_[node] = U8String(labels_str.str());
    		}
    	});
    }


    HistoryNode* build_history_tree(
        const SnapshotID& snapshot_id,
        HistoryNode* parent,
        const HistoryTreeNodeMap& history_map,
        const BlockMap& block_map
    )
    {
        auto iter = history_map.find(snapshot_id);

        if (iter != history_map.end())
        {
            HistoryNodeBuffer* buffer = iter->second;

            // FIXME memory leak in case of exception!
            HistoryNode* node = new HistoryNode(&self(), snapshot_id, parent, buffer->status());

            IDToMemBlockIDResolver id_resolver(&block_map);

            node->set_metadata(buffer->metadata());

            snapshot_map_[snapshot_id] = node;

            if (!buffer->root_id().is_null())
            {
                auto block_iter = block_map.find(buffer->root_id());

                if (block_iter != block_map.end())
                {
                    node->assign_root_no_ref(block_iter->second);

                    for (const auto& child_snapshot_id: buffer->children())
                    {
                        build_history_tree(child_snapshot_id, node, history_map, block_map);
                    }

                    return node;
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("Specified node_id {} is not found in persistent tree data", buffer->root_id()).do_throw();
                }
            }
            else {
                for (const auto& child_snapshot_id: buffer->children())
                {
                    build_history_tree(child_snapshot_id, node, history_map, block_map);
                }

                return node;
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Specified snapshot_id {} is not found in history data", snapshot_id).do_throw();
        }
    }


    //virtual void free_memory(HistoryNode* node) = 0;


    /**
     * Deletes recursively all data quickly. Must not be used to delete data of a selected snapshot.
     */

    void free_memory(const NodeBaseT* node)
    {
//        const auto& snapshot_id = node->snapshot_id();
//        if (node->is_leaf())
//        {
//            auto leaf = PersistentTreeT::to_leaf_node(node);

//            for (int32_t c = 0; c < leaf->size(); c++)
//            {
//                auto& data = leaf->data(c);
//                if (data.snapshot_id() == snapshot_id)
//                {
//                    free_system(data.block_ptr());
//                }
//            }

//            leaf->del();
//        }
//        else {
//            auto branch = PersistentTreeT::to_branch_node(node);

//            for (int32_t c = 0; c < branch->size(); c++)
//            {
//                auto child = branch->data(c);
//                if (child->snapshot_id() == snapshot_id)
//                {
//                    free_memory(child);
//                }
//            }

//            branch->del();
//        }
    }

    void read_metadata(InputStreamHandler& in, AllocatorMetadata& metadata)
    {
        in >> metadata.master();
        in >> metadata.root();
        in >> metadata.id_counter();

        int64_t size;

        in >>size;

        for (int64_t c = 0; c < size; c++)
        {
            U8String name;
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

    void read_history_node(InputStreamHandler& in, HistoryTreeNodeMap& map)
    {
        HistoryNodeBuffer* node = new HistoryNodeBuffer();

        int32_t status;

        in >> status;

        node->status() = static_cast<typename HistoryNode::Status>(status);

        in >> node->snapshot_id();
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
            MEMORIA_MAKE_GENERIC_ERROR("HistoryTree Node {} has been already registered", node->snapshot_id()).do_throw();
        }
    }



    void read_data_block(InputStreamHandler& in, BlockMap& map)
    {
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

        ProfileMetadata<Profile>::local()
                ->get_block_operations(ctr_hash, block_hash)
                ->deserialize(block_data.get(), block_data_size, block);

        if (map.find(block->id()) == map.end())
        {
            map[block->id()] = block;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Block {} was already registered", block->uuid()).do_throw();
        }
    }





    void write_metadata(OutputStreamHandler& out)
    {
        uint8_t type = TYPE_METADATA;
        out << type;

        out << master_->snapshot_id();
        out << history_tree_->snapshot_id();
        out << id_counter_;

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



    virtual void serialize_snapshot(
            OutputStreamHandler& out,
            const HistoryNode* history_node,
            RCBlockSet& stored_blocks
    ) = 0;


    void write_history_node(OutputStreamHandler& out, const HistoryNode* history_node, RCBlockSet& stored_blocks)
    {
        MemToIDBlockIDResolver id_resolver;

    	uint8_t type = TYPE_HISTORY_NODE;
        out << type;
        out << (int32_t)history_node->status();
        out << history_node->snapshot_id();

        if (history_node->root_id().isSet())
        {
            auto root_id_value = id_resolver.resolve_id(history_node->root_id());
            out << root_id_value;
        }
        else {
            out << BlockID{};
        }

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

        if (history_node->root_id().isSet())
        {
            const BlockType* block = detail::IDValueHolderH<BlockID>::template get_block_ptr<BlockType>(history_node->root_id());
            if (stored_blocks.count(block) == 0)
            {
                return serialize_snapshot(out, history_node, stored_blocks);
            }
        }
    }



    class MemToIDBlockIDResolver: public IBlockOperations<Profile>::IDValueResolver {
        using BlockIDValueHolder = detail::IDValueHolderT<BlockID>;


    public:
        MemToIDBlockIDResolver() noexcept {}

        virtual BlockID resolve_id(const BlockID& mem_block_id) const
        {
            BlockType* block = value_cast<BlockType*>(detail::IDValueHolderH<BlockID>::from_id(mem_block_id));
            return BlockID{block->id_value()};
        }
    };



    void write_ctr_block(OutputStreamHandler& out, const BlockType* block)
    {
        MemToIDBlockIDResolver id_resolver;

        uint8_t type = TYPE_DATA_BLOCK;
        out << type;

        auto block_size = block->memory_block_size();
        auto buffer     = allocate_system<uint8_t>(block_size);

        int32_t total_data_size = ProfileMetadata<Profile>::local()
                ->get_block_operations(block->ctr_type_hash(), block->block_type_hash())
                ->serialize(block, buffer.get(), &id_resolver);

        out << (total_data_size);
        out << block->memory_block_size();
        out << block->ctr_type_hash();
        out << block->block_type_hash();

        out.write(buffer.get(), 0, total_data_size);

        records_++;
    }



    void dump(const BlockType* block, std::ostream& out = std::cout)
    {
        TextBlockDumper dumper(out);

        auto blk_ops = ProfileMetadata<Profile>::local()
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
            if (node->root_id().is_null() && node->references() == 0)
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

            if (this->is_dump_snapshot_lifecycle()) {
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

            if (this->is_dump_snapshot_lifecycle()) {
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
            std::unordered_set<U8String> branch_set_;
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

    void walk_linear_history(
            const SnapshotID& start_id,
            const SnapshotID& stop_id,
            std::function<void (const HistoryNode* history_node)> fn
    )
    {
        auto ii = snapshot_map_.find(start_id);
        if (ii != snapshot_map_.end())
        {
            auto current = ii->second;

            while (current && current->snapshot_id() != stop_id)
            {
                fn(current);
                current = current->parent();
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is not found.", start_id).do_throw();
        }
    }


    MyType& self() noexcept {return *static_cast<MyType*>(this);}
    const MyType& self() const noexcept {return *static_cast<const MyType*>(this);}
};

}


}}

