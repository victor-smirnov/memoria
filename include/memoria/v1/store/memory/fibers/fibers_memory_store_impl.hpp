
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

#include <memoria/v1/containers/map/map_factory.hpp>

#include <memoria/v1/core/tools/pool.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/stream.hpp>
#include <memoria/v1/core/tools/pair.hpp>
#include <memoria/v1/core/tools/latch.hpp>
#include <memoria/v1/core/tools/memory.hpp>

#include <memoria/v1/store/memory/common/store_base.hpp>
#include <memoria/v1/store/memory/fibers/fibers_snapshot_impl.hpp>

#include <memoria/v1/reactor/reactor.hpp>

#include <memoria/v1/fiber/detail/spinlock.hpp>
#include <memoria/v1/fiber/count_down_latch.hpp>
#include <memoria/v1/fiber/shared_mutex.hpp>
#include <memoria/v1/fiber/recursive_shared_mutex.hpp>
#include <memoria/v1/fiber/shared_lock.hpp>

#include <memoria/v1/core/graph/graph.hpp>

#include <memoria/v1/api/store/memory_store_event_listener.hpp>

#include <stdlib.h>
#include <memory>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>



namespace memoria {
namespace v1 {
namespace store {
namespace memory {


template <typename Profile>
class FibersMemoryStoreImpl: public IGraph, public MemoryStoreBase<Profile, FibersMemoryStoreImpl<Profile>> {
public:
    using Base      = MemoryStoreBase<Profile, FibersMemoryStoreImpl<Profile>>;
    using MyType    = FibersMemoryStoreImpl<Profile>;
    
    using typename Base::BlockType;

    using SnapshotT             = FibersSnapshot<Profile, MyType>;
    using SnapshotPtr           = SnpSharedPtr<SnapshotT>;
    using SnapshotApiPtr        = SnpSharedPtr<IMemorySnapshot<Profile>>;

    using StorePtr              = AllocSharedPtr<MyType>;
    
    using MutexT                = memoria::v1::fibers::detail::spinlock;
    using LockGuardT            = memoria::v1::fibers::detail::spinlock_lock;
    
    using SnapshotMutexT        = memoria::v1::fibers::detail::spinlock;
    using SnapshotLockGuardT    = memoria::v1::fibers::detail::spinlock_lock;
        
    using typename Base::HistoryNode;
    using typename Base::HistoryNodeBuffer;
    using typename Base::SnapshotID;
    using typename Base::RCBlockSet;
    using typename Base::Checksum;
    
    using Base::load;

protected:
    using Base::history_tree_;
    using Base::snapshot_map_;
    using Base::named_branches_;
    using Base::master_;
    using Base::snapshot_labels_metadata;
    using Base::records_;
    using Base::write_metadata;
    using Base::write_history_node;
    using Base::write;
    using Base::do_pack;
    using Base::get_labels_for;
    using Base::do_remove_history_node;

public:    
    template <typename, typename>
    friend class MemoryStoreBase;
    
    template <typename, typename, typename>
    friend class SnapshotBase;
    
    template <typename, typename>
    friend class FibersSnapshot;
    

private:
        
    int32_t cpu_;
    
    MutexT mutex_;
    MutexT store_mutex_;
    
    memoria::v1::fibers::recursive_shared_mutex store_rw_mutex_;
    
    memoria::v1::fibers::count_down_latch<int64_t> active_snapshots_;

    LocalSharedPtr<StoreEventListener> event_listener_;
 
public:
    FibersMemoryStoreImpl() {
        cpu_ = reactor::engine().cpu();
        auto snapshot = snp_make_shared_init<SnapshotT>(history_tree_, this, OperationType::OP_CREATE);
        snapshot->commit();
    }
    
    virtual ~FibersMemoryStoreImpl() noexcept
    {
        free_memory(history_tree_);
    }


    FibersMemoryStoreImpl(int32_t v): Base(v) {
        cpu_ = reactor::engine().cpu();
    }
private:
    
    auto ref_active() {
        return active_snapshots_.inc();
    }

    auto unref_active() {
        return active_snapshots_.dec();
    }
    
public:

    void set_event_listener(LocalSharedPtr<StoreEventListener> ptr) {
        event_listener_ = ptr;
    }

    LocalSharedPtr<StoreEventListener> event_listener() {
        return event_listener_;
    }
    
    int64_t active_snapshots() {
        return active_snapshots_.get();
    }

    void lock() {
        mutex_.lock();
    }

    void unlock() {
        mutex_.unlock();
    }

    bool try_lock() {
        return mutex_.try_lock();
    }
    
    int32_t cpu() const {return cpu_;}
    
    void pack()
    {
        return reactor::engine().run_at(cpu_, [&]{
            do_pack(history_tree_);
        });
    }


    SnapshotID root_shaphot_id() const
    {
        return reactor::engine().run_at(cpu_, [&]{
            return history_tree_->snapshot_id();
        });
    }

    std::vector<SnapshotID> children_of(const SnapshotID& snapshot_id) const
    {
        return reactor::engine().run_at(cpu_, [&]{
            std::vector<SnapshotID> ids;

            auto iter = snapshot_map_.find(snapshot_id);
            if (iter != snapshot_map_.end())
            {
                const auto history_node = iter->second;
                for (const auto* child: history_node->children())
                {
                    ids.push_back(child->snapshot_id());
                }
            }

            return ids;
        });
    }

    std::vector<std::string> children_of_str(const SnapshotID& snapshot_id) const
    {
        std::vector<std::string> ids;
        auto uids = children_of(snapshot_id);

        for (const auto& uid: uids)
        {
            ids.push_back(uid.str());
        }

        return ids;
    }

    void remove_named_branch(const std::string& name)
    {
        return reactor::engine().run_at(cpu_, [&]{
            named_branches_.erase(U16String(name));
        });
    }

    std::vector<U16String> branch_names()
    {
        return reactor::engine().run_at(cpu_, [&]{
            std::vector<U16String> branches;

            for (auto pair: named_branches_)
            {
                branches.push_back(pair.first);
            }

            return branches;
        });
    }

    SnapshotID branch_head(const U16String& branch_name)
    {
        return reactor::engine().run_at(cpu_, [&]{

            auto ii = named_branches_.find(branch_name);
            if (ii != named_branches_.end())
            {
                return ii->second->snapshot_id();
            }

            return SnapshotID{};
        });
    }

    std::vector<SnapshotID> heads()
    {
        std::lock_guard<MutexT> lock(mutex_);
        std::unordered_set<SnapshotID> ids;

        for (const auto& entry: named_branches_)
        {
            // TODO: need to take a lock here on the snapshot
            ids.insert(entry.second->snapshot_id());
        }

        ids.insert(master_->snapshot_id());

        return std::vector<SnapshotID>(ids.begin(), ids.end());
    }

    int32_t snapshot_status(const SnapshotID& snapshot_id)
    {
        return reactor::engine().run_at(cpu_, [&]{
            auto iter = snapshot_map_.find(snapshot_id);
            if (iter != snapshot_map_.end())
            {
                const auto history_node = iter->second;
                return (int32_t)history_node->status();
            }
            else {
                MMA1_THROW(Exception()) << fmt::format_ex(u"Snapshot id {} is unknown", snapshot_id);
            }
        });
    }

    SnapshotID snapshot_parent(const SnapshotID& snapshot_id)
    {
        return reactor::engine().run_at(cpu_, [&]{
            auto iter = snapshot_map_.find(snapshot_id);
            if (iter != snapshot_map_.end())
            {
                const auto history_node = iter->second;
                auto parent_id = history_node->parent() ? history_node->parent()->snapshot_id() : SnapshotID{};
                return parent_id;
            }
            else {
                MMA1_THROW(Exception()) << fmt::format_ex(u"Snapshot id {} is unknown", snapshot_id);
            }
        });
    }

    U16String snapshot_description(const SnapshotID& snapshot_id)
    {
        return reactor::engine().run_at(cpu_, [&]{
            auto iter = snapshot_map_.find(snapshot_id);
            if (iter != snapshot_map_.end())
            {
                const auto history_node = iter->second;
                return history_node->metadata();
            }
            else {
                MMA1_THROW(Exception()) << fmt::format_ex(u"Snapshot id {} is unknown", snapshot_id);
            }
        });
    }




    SnpSharedPtr<SnapshotMetadata<SnapshotID>> describe(const SnapshotID& snapshot_id) const
    {
    	return reactor::engine().run_at(cpu_, [&]{
            auto iter = snapshot_map_.find(snapshot_id);
            if (iter != snapshot_map_.end())
            {
                const auto history_node = iter->second;

                std::vector<SnapshotID> children;

                for (const auto& node: history_node->children())
                {
                    children.emplace_back(node->snapshot_id());
                }

                auto parent_id = history_node->parent() ? history_node->parent()->snapshot_id() : SnapshotID{};

                return snp_make_shared<SnapshotMetadata<SnapshotID>>(
                    parent_id, history_node->snapshot_id(), children, history_node->metadata(), history_node->status()
                );
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot id {} is unknown", snapshot_id));
            }
        });
    }



    SnapshotApiPtr find(const SnapshotID& snapshot_id)
    {
        return reactor::engine().run_at(cpu_, [&]
        {
            auto iter = snapshot_map_.find(snapshot_id);
            if (iter != snapshot_map_.end())
            {
                auto history_node = iter->second;

                if (history_node->is_committed())
                {
                    return snp_make_shared_init<SnapshotT>(history_node, this->shared_from_this(), OperationType::OP_FIND);
                }
                if (history_node->is_data_locked())
                {
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} data is locked", history_node->snapshot_id()));
                }
                else {
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} is {}", history_node->snapshot_id(), (history_node->is_active() ? u"active" : u"dropped")));
                }
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot id {} is unknown", snapshot_id));
            }
        });
    }




    SnapshotApiPtr find_branch(U16StringRef name)
    {
        return reactor::engine().run_at(cpu_, [&]{
            auto iter = named_branches_.find(name);
            if (iter != named_branches_.end())
            {
                auto history_node = iter->second;

                if (history_node->is_committed())
                {
                    return snp_make_shared_init<SnapshotT>(history_node, this->shared_from_this(), OperationType::OP_FIND);
                }
                else if (history_node->is_data_locked())
                {
                    if (history_node->references() == 0)
                    {
                        return snp_make_shared_init<SnapshotT>(history_node, this->shared_from_this(), OperationType::OP_FIND);
                    }
                    else {
                        MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot id {} is locked and open", history_node->snapshot_id()));
                    }
                }
                else {
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} is {}", history_node->snapshot_id(), (history_node->is_active() ? u"active" : u"dropped")));
                }
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Named branch \"{}\" is not known", name));
            }
        });
    }

    SnapshotApiPtr master()
    {
        return reactor::engine().run_at(cpu_, [&]{
            return snp_make_shared_init<SnapshotT>(master_, this->shared_from_this(), OperationType::OP_FIND);
        });
    }

    SnpSharedPtr<SnapshotMetadata<SnapshotID>> describe_master() const
    {
        return reactor::engine().run_at(cpu_, [&]{
            std::vector<SnapshotID> children;

            for (const auto& node: master_->children())
            {
                children.emplace_back(node->snapshot_id());
            }

            auto parent_id = master_->parent() ? master_->parent()->snapshot_id() : SnapshotID{};

            return snp_make_shared<SnapshotMetadata<SnapshotID>>(
                parent_id, master_->snapshot_id(), children, master_->metadata(), master_->status()
            );
        });
    }

    void set_master(const SnapshotID& txn_id)
    {
        return reactor::engine().run_at(cpu_, [&]
        {
            auto iter = snapshot_map_.find(txn_id);
            if (iter != snapshot_map_.end())
            {
                auto history_node = iter->second;

                if (history_node->is_committed())
                {
                    master_ = iter->second;
                }
                else if (history_node->is_data_locked())
                {
                    master_ = iter->second;
                }
                else if (history_node->is_dropped())
                {
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} has been dropped", txn_id));
                }
                else {
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} hasn't been committed yet", txn_id));
                }
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} is not known in this allocator", txn_id));
            }
        });
    }

    void set_branch(U16StringRef name, const SnapshotID& txn_id)
    {
        return reactor::engine().run_at(cpu_, [&]
        {
            auto iter = snapshot_map_.find(txn_id);
            if (iter != snapshot_map_.end())
            {
                auto history_node = iter->second;

                if (history_node->is_committed())
                {
                    named_branches_[name] = history_node;
                }
                else if (history_node->is_data_locked())
                {
                    named_branches_[name] = history_node;
                }
                else {
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} hasn't been committed yet", txn_id));
                }
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} is not known in this allocator", txn_id));
            }
        });
    }


    virtual void walk_containers(ContainerWalker<Profile>* walker, const char16_t* allocator_descr)
    {
        return reactor::engine().run_at(cpu_, [&]
        {
            this->build_snapshot_labels_metadata();

            walker->beginAllocator(u"PersistentInMemAllocator", allocator_descr);

            walk_containers(history_tree_, walker);

            walker->endAllocator();

            snapshot_labels_metadata().clear();
        });
    }

    

    virtual void store(OutputStreamHandler *output, int64_t wait_duration)
    {
        return reactor::engine().run_at(cpu_, [&]{

            active_snapshots_.wait(0);
            
            do_pack(history_tree_);

            records_ = 0;

            char signature[12] = "MEMORIA";
            for (size_t c = 7; c < sizeof(signature); c++) signature[c] = 0;

            output->write(&signature, 0, sizeof(signature));

            write_metadata(*output);

            RCBlockSet stored_blocks;

            walk_version_tree(history_tree_, [&](const HistoryNode* history_tree_node) {
                write_history_node(*output, history_tree_node, stored_blocks);
            });

            Checksum checksum;
            checksum.records() = records_;

            write(*output, checksum);

            output->close();
        });
    }

    virtual void store(U8String file_name, int64_t wait_duration)
    {
        auto fileh = FileOutputStreamHandler::create_buffered(file_name.to_u16());
        this->store(fileh.get(), wait_duration);
    }

    virtual void store(filesystem::path file)
    {
    	auto fileh = FileOutputStreamHandler::create_buffered(file);
        store(fileh.get(), 0);
    }


    static AllocSharedPtr<MyType> load(const char16_t* file, int32_t cpu)
    {
        return reactor::engine().run_at(cpu, [&]{
            auto fileh = FileInputStreamHandler::create(U16String(file).to_u8().data());
            return Base::load(fileh.get());
        });
    }
    
    static AllocSharedPtr<MyType> load(const char16_t* file)
    {
        return load(file, reactor::engine().cpu());
    }
    
    static auto create(int32_t cpu)
    {
        return reactor::engine().run_at(cpu, [cpu]{
            auto alloc = Base::create();
            alloc->cpu_ = cpu;
            return alloc;
        });
    }
    
    static auto create()
    {
        return create(reactor::engine().cpu());
    }


    // Graph API

    Graph as_graph() {
        return Graph(memoria_static_pointer_cast<IGraph>(this->shared_from_this()));
    }

    virtual Collection<Vertex> vertices()
    {
        return EmptyCollection<Vertex>::make();
    }

    virtual Collection<Vertex> vertices(const IDList& ids)
    {
        return EmptyCollection<Vertex>::make();
    }

    virtual Collection<Vertex> roots()
    {
        return roots({u"snapshot"});
    }

    virtual Collection<Vertex> roots(const LabelList& vx_labels)
    {
        std::vector<Vertex> vxx;

        append_snapsots(vxx);

        return STLCollection<Vertex>::make(std::move(vxx));
    }

    template <typename StlCtr>
    void append_snapsots(StlCtr& stl_ctr)
    {
        auto uuid = this->get_root_snapshot_uuid();
        SnapshotPtr root_snapshot = memoria_static_pointer_cast<SnapshotT>(this->find(uuid));

        stl_ctr.emplace_back(root_snapshot->as_vertex());
    }

    virtual Collection<Edge> edges()
    {
        return EmptyCollection<Edge>::make();
    }

    virtual Collection<Edge> edges(const IDList& ids)
    {
        return EmptyCollection<Edge>::make();
    }

    SharedPtr<AllocatorMemoryStat> memory_stat(bool include_containers)
    {
        return reactor::engine().run_at(cpu_, [&]{
            _::BlockSet visited_blocks;

            SharedPtr<AllocatorMemoryStat> alloc_stat = MakeShared<AllocatorMemoryStat>(0);

            auto history_visitor = [&](HistoryNode* node) {
                if (node->is_committed() || node->is_dropped())
                {
                    auto snp = snp_make_shared_init<SnapshotT>(node, this->shared_from_this(), OperationType::OP_FIND);
                    auto snp_stat = snp->do_compute_memory_stat(visited_blocks, include_containers);
                    alloc_stat->add_snapshot_stat(snp_stat);
                }
            };

            this->walk_version_tree(history_tree_, history_visitor);

            alloc_stat->compute_total_size();

            return alloc_stat;
        });
    }

protected:
    
    void walk_version_tree(HistoryNode* node, std::function<void (HistoryNode*, SnapshotT*)> fn)
    {
        if (node->is_committed())
        {
            auto txn = snp_make_shared_init<SnapshotT>(node, this, OperationType::OP_FIND);
            fn(node, txn.get());
        }

        for (auto child: node->children())
        {
            walk_version_tree(child, fn);
        }
    }

    virtual void walk_version_tree(HistoryNode* node, std::function<void (HistoryNode*)> fn)
    {
        fn(node);

        for (auto child: node->children())
        {
            walk_version_tree(child, fn);
        }
    }

    void walk_containers(HistoryNode* node, ContainerWalker<Profile>* walker)
    {
    	if (node->is_committed())
        {
            auto txn = snp_make_shared_init<SnapshotT>(node, this, OperationType::OP_FIND);
            txn->walk_containers(walker, get_labels_for(node));
        }

        if (node->children().size())
        {
            walker->beginSnapshotSet(u"Branches", node->children().size());
            for (auto child: node->children())
            {
                walk_containers(child, walker);
            }
            walker->endSnapshotSet();
        }
    }

    
    virtual void free_memory(HistoryNode* node)
    {
        for (auto child: node->children())
        {
            free_memory(child);
        }

        if (node->root())
        {
            SnapshotT::delete_snapshot(node);
        }

        delete node;
    }
    
    void do_delete_dropped()
    {
        do_delete_dropped(history_tree_);
    }

    void do_delete_dropped(HistoryNode* node)
    {
        if (node->is_dropped() && node->root() != nullptr)
        {
            SnapshotT::delete_snapshot(node);
        }

        for (auto child: node->children())
        {
            do_delete_dropped(child);
        }
    }
    
    virtual void forget_snapshot(HistoryNode* history_node)
    {
        snapshot_map_.erase(history_node->snapshot_id());

        history_node->remove_from_parent();

        delete history_node;
    }
    
    
    virtual void do_pack(HistoryNode* node, int32_t depth, const std::unordered_set<HistoryNode*>& branches)
    {
    	// FIXME: use dedicated stack data structure

        auto children = node->children();
        for (auto child: children)
        {
            do_pack(child, depth + 1, branches);
        }

        bool remove_node = false;
        {
        	if (node->root() == nullptr && node->references() == 0 && branches.find(node) == branches.end())
        	{
        		remove_node = true;
        	}
        }

        if (remove_node) {
        	do_remove_history_node(node);
        }
    }
};


}}


template <typename Profile>
AllocSharedPtr<IMemoryStore<Profile>> IMemoryStore<Profile>::load(InputStreamHandler* input_stream)
{
    return store::memory::FibersMemoryStoreImpl<Profile>::load(input_stream);
}

template <typename Profile>
AllocSharedPtr<IMemoryStore<Profile>> IMemoryStore<Profile>::load(U8String input_file)
{
    auto fileh = FileInputStreamHandler::create(input_file.data());
    return store::memory::FibersMemoryStoreImpl<Profile>::load(fileh.get());
}


template <typename Profile>
AllocSharedPtr<IMemoryStore<Profile>> IMemoryStore<Profile>::create()
{
    return MakeShared<store::memory::FibersMemoryStoreImpl<Profile>>();
}



}}
