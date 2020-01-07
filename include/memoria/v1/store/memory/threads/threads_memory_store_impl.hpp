
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
#include <memoria/v1/core/tools/stream.hpp>
#include <memoria/v1/core/tools/pair.hpp>
#include <memoria/v1/core/tools/latch.hpp>
#include <memoria/v1/core/tools/memory.hpp>

#include <memoria/v1/store/memory/common/store_base.hpp>

#include <memoria/v1/store/memory/threads/threads_snapshot_impl.hpp>

#include <cstdlib>
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
class ThreadsMemoryStoreImpl: public IGraph, public MemoryStoreBase<Profile, ThreadsMemoryStoreImpl<Profile>> {
public:
    using Base      = MemoryStoreBase<Profile, ThreadsMemoryStoreImpl<Profile>>;
    using MyType    = ThreadsMemoryStoreImpl<Profile>;
    
    using typename Base::BlockType;

    using SnapshotT             = ThreadsSnapshot<Profile, MyType>;
    using SnapshotPtr           = SnpSharedPtr<SnapshotT>;
    using SnapshotApiPtr        = SnpSharedPtr<IMemorySnapshot<Profile>>;

    using AllocatorPtr          = AllocSharedPtr<MyType>;
    
    using MutexT			= std::recursive_mutex;
    using SnapshotMutexT	= std::recursive_mutex;
    using StoreMutexT		= std::mutex;

    using LockGuardT			= std::lock_guard<MutexT>;
    using StoreLockGuardT		= std::lock_guard<StoreMutexT>;
    using SnapshotLockGuardT	= std::lock_guard<SnapshotMutexT>;
    
    using typename Base::HistoryNode;
    using typename Base::HistoryNodeBuffer;
    using typename Base::SnapshotID;
    using typename Base::BlockID;
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
    friend class ThreadsSnapshot;

    template <typename, typename>
    friend class MemoryStoreBase;

private:
    
    mutable MutexT mutex_;
    mutable StoreMutexT store_mutex_;
    
    CountDownLatch<int64_t> active_snapshots_;
 
public:
    ThreadsMemoryStoreImpl() {
        auto snapshot = snp_make_shared_init<SnapshotT>(history_tree_, this);
        snapshot->commit();
    }
    
    
    virtual ~ThreadsMemoryStoreImpl() noexcept
    {
        free_memory(history_tree_);
    }


    ThreadsMemoryStoreImpl(int32_t v): Base(v) {}

private:
    auto ref_active() {
        return active_snapshots_.inc();
    }

    auto unref_active() {
        return active_snapshots_.dec();
    }

protected:
    auto& store_mutex() {
    	return store_mutex_;
    }

    const auto& store_mutex() const {
    	return store_mutex_;
    }

    SnapshotApiPtr upcast(SnapshotPtr ptr) {
        return memoria_static_pointer_cast<IMemorySnapshot<Profile>>(ptr);
    }

public:
    
    int64_t active_snapshots() {
        return active_snapshots_.get();
    }

    
    MutexT& mutex() {
    	return mutex_;
    }

    const MutexT& mutex() const {
    	return mutex_;
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
    
    void pack()
    {
        std::lock_guard<MutexT> lock(mutex_);
    	do_pack(history_tree_);
    }


    SnapshotID root_shaphot_id() const
    {
        LockGuardT lock_guard(mutex_);
        return history_tree_->snapshot_id();
    }

    std::vector<SnapshotID> children_of(const SnapshotID& snapshot_id) const
    {
        std::vector<SnapshotID> ids;

        LockGuardT lock_guard(mutex_);

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
        LockGuardT lock_guard(mutex_);
        named_branches_.erase(U16String(name));
    }

    std::vector<U16String> branch_names()
    {
        std::lock_guard<MutexT> lock(mutex_);

        std::vector<U16String> branches;

        for (auto pair: named_branches_)
        {
            branches.push_back(pair.first);
        }

        return branches;
    }

    SnapshotID branch_head(const U16String& branch_name)
    {
        std::lock_guard<MutexT> lock(mutex_);

        auto ii = named_branches_.find(branch_name);
        if (ii != named_branches_.end())
        {
            // TODO: need to take a lock here on the snapshot
            return ii->second->snapshot_id();
        }

        return SnapshotID{};
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

    
    SnapshotMetadata<SnapshotID> describe(const SnapshotID& snapshot_id) const
    {
    	LockGuardT lock_guard2(mutex_);

        auto iter = snapshot_map_.find(snapshot_id);
        if (iter != snapshot_map_.end())
        {
            const auto history_node = iter->second;

            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());

            std::vector<SnapshotID> children;

        	for (const auto& node: history_node->children())
        	{
                children.emplace_back(node->snapshot_id());
        	}

            auto parent_id = history_node->parent() ? history_node->parent()->snapshot_id() : SnapshotID{};

            return SnapshotMetadata<SnapshotID>(
                parent_id, history_node->snapshot_id(), children, history_node->metadata(), history_node->status()
			);
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot id {} is unknown", snapshot_id));
        }
    }


    virtual int32_t snapshot_status(const SnapshotID& snapshot_id)
    {
        LockGuardT lock_guard2(mutex_);

        auto iter = snapshot_map_.find(snapshot_id);
        if (iter != snapshot_map_.end())
        {
            const auto history_node = iter->second;
            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());
            return (int32_t)history_node->status();
        }
        else {
            MMA1_THROW(Exception()) << fmt::format_ex(u"Snapshot id {} is unknown", snapshot_id);
        }
    }

    SnapshotID snapshot_parent(const SnapshotID& snapshot_id)
    {
        LockGuardT lock_guard2(mutex_);

        auto iter = snapshot_map_.find(snapshot_id);
        if (iter != snapshot_map_.end())
        {
            const auto history_node = iter->second;
            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());

            auto parent_id = history_node->parent() ? history_node->parent()->snapshot_id() : SnapshotID{};
            return parent_id;
        }
        else {
            MMA1_THROW(Exception()) << fmt::format_ex(u"Snapshot id {} is unknown", snapshot_id);
        }
    }

    U16String snapshot_description(const SnapshotID& snapshot_id)
    {
        LockGuardT lock_guard2(mutex_);

        auto iter = snapshot_map_.find(snapshot_id);
        if (iter != snapshot_map_.end())
        {
            const auto history_node = iter->second;
            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());
            return history_node->metadata();
        }
        else {
            MMA1_THROW(Exception()) << fmt::format_ex(u"Snapshot id {} is unknown", snapshot_id);
        }
    }




    SnapshotApiPtr find(const SnapshotID& snapshot_id)
    {
    	LockGuardT lock_guard(mutex_);

        auto iter = snapshot_map_.find(snapshot_id);
        if (iter != snapshot_map_.end())
        {
            auto history_node = iter->second;

            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());

            if (history_node->is_committed())
            {
                return upcast(snp_make_shared_init<SnapshotT>(history_node, this->shared_from_this()));
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
    }




    SnapshotApiPtr find_branch(U16StringRef name)
    {
    	LockGuardT lock_guard(mutex_);

    	auto iter = named_branches_.find(name);
        if (iter != named_branches_.end())
        {
            auto history_node = iter->second;

            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());

            if (history_node->is_committed())
            {
                return upcast(snp_make_shared_init<SnapshotT>(history_node, this->shared_from_this()));
            }
            else if (history_node->is_data_locked())
            {
            	if (history_node->references() == 0)
            	{
                    return upcast(snp_make_shared_init<SnapshotT>(history_node, this->shared_from_this()));
            	}
            	else {
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot id {} is locked and open", history_node->snapshot_id()));
            	}
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} is {} ", history_node->snapshot_id(), (history_node->is_active() ? u"active" : u"dropped")));
            }
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Named branch \"{}\" is not known", name));
        }
    }

    SnapshotApiPtr master()
    {
    	std::lock(mutex_, master_->snapshot_mutex());

    	LockGuardT lock_guard(mutex_, std::adopt_lock);
    	SnapshotLockGuardT snapshot_lock_guard(master_->snapshot_mutex(), std::adopt_lock);

        return upcast(snp_make_shared_init<SnapshotT>(master_, this->shared_from_this()));
    }

    SnapshotMetadata<SnapshotID> describe_master() const
    {
    	std::lock(mutex_, master_->snapshot_mutex());
    	LockGuardT lock_guard2(mutex_, std::adopt_lock);
    	SnapshotLockGuardT snapshot_lock_guard(master_->snapshot_mutex(), std::adopt_lock);

        std::vector<SnapshotID> children;

    	for (const auto& node: master_->children())
    	{
            children.emplace_back(node->snapshot_id());
    	}

        auto parent_id = master_->parent() ? master_->parent()->snapshot_id() : SnapshotID{};

        return SnapshotMetadata<SnapshotID>(
            parent_id, master_->snapshot_id(), children, master_->metadata(), master_->status()
    	);
    }

    void set_master(const SnapshotID& txn_id)
    {
    	LockGuardT lock_guard(mutex_);

        auto iter = snapshot_map_.find(txn_id);
        if (iter != snapshot_map_.end())
        {
            auto history_node = iter->second;

            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());

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
    }

    void set_branch(U16StringRef name, const SnapshotID& txn_id)
    {
    	LockGuardT lock_guard(mutex_);

        auto iter = snapshot_map_.find(txn_id);
        if (iter != snapshot_map_.end())
        {
            auto history_node = iter->second;

            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());

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
    }

    virtual void walk_containers(ContainerWalker<Profile>* walker, const char16_t* allocator_descr = nullptr)
    {
    	this->build_snapshot_labels_metadata();

    	LockGuardT lock_guard(mutex_);

        walker->beginAllocator(u"PersistentInMemAllocator", allocator_descr);

        walk_containers(history_tree_, walker);

        walker->endAllocator();

        snapshot_labels_metadata().clear();
    }

    
private:
    virtual void do_store(OutputStreamHandler *output)
    {
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
    }
public:

    virtual void store(OutputStreamHandler *output, int64_t wait_duration)
    {
        std::lock(mutex_, store_mutex_);

        LockGuardT lock_guard2(mutex_, std::adopt_lock);
        StoreLockGuardT lock_guard1(store_mutex_, std::adopt_lock);

        if (wait_duration == 0) {
            active_snapshots_.wait(0);
        }
        else if (!active_snapshots_.waitFor(0, wait_duration)) {
            MMA1_THROW(Exception()) << fmt::format_ex(u"Active snapshots commit/drop waiting timeout: {} ms", wait_duration);
        }

        do_store(output);
    }


    void store(memoria::filesystem::path file_name, int64_t wait_duration)
    {
        this->store(file_name.string().c_str(), wait_duration);
    }

    virtual void store(U8String file_name, int64_t wait_duration)
    {
        this->store(file_name.data(), wait_duration);
    }

    virtual void store(const char* file, int64_t wait_duration)
    {
        std::lock(mutex_, store_mutex_);

        LockGuardT lock_guard2(mutex_, std::adopt_lock);
        StoreLockGuardT lock_guard1(store_mutex_, std::adopt_lock);

        if (wait_duration == 0) {
            active_snapshots_.wait(0);
        }
        else if (!active_snapshots_.waitFor(0, wait_duration)){
            MMA1_THROW(Exception()) << fmt::format_ex(u"Active snapshots commit/drop waiting timeout: {} ms", wait_duration);
        }

    	auto fileh = FileOutputStreamHandler::create(file);
        do_store(fileh.get());
    }


    static AllocSharedPtr<MyType> load(const char16_t* file)
    {
        auto fileh = FileInputStreamHandler::create(U16String(file).to_u8().data());
        return Base::load(fileh.get());
    }

    static AllocSharedPtr<MyType> load(const U16String& file)
    {
        auto fileh = FileInputStreamHandler::create(file.to_u8().data());
        return Base::load(fileh.get());
    }

    SharedPtr<AllocatorMemoryStat> memory_stat(bool include_containers)
    {
        LockGuardT lock_guard(mutex_);

        _::BlockSet visited_blocks;

        SharedPtr<AllocatorMemoryStat> alloc_stat = MakeShared<AllocatorMemoryStat>(0);

        auto history_visitor = [&](HistoryNode* node) {
            SnapshotLockGuardT snapshot_lock_guard(node->snapshot_mutex());

            if (node->is_committed() || node->is_dropped())
            {
                auto snp = snp_make_shared_init<SnapshotT>(node, this->shared_from_this());
                auto snp_stat = snp->do_compute_memory_stat(visited_blocks, include_containers);
                alloc_stat->add_snapshot_stat(snp_stat);
            }
        };

        this->walk_version_tree(history_tree_, history_visitor);

        alloc_stat->compute_total_size();

        return alloc_stat;
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

protected:
    
    void walk_version_tree(HistoryNode* node, std::function<void (HistoryNode*, SnapshotT*)> fn)
    {
        if (node->is_committed())
        {
            auto txn = snp_make_shared_init<SnapshotT>(node, this);
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
    	SnapshotLockGuardT snapshot_lock_guard(node->snapshot_mutex());

        if (node->is_committed())
        {
            auto txn = snp_make_shared_init<SnapshotT>(node, this);
            txn->walkContainers(walker, get_labels_for(node));
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
        if (node)
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
    }
    
    void do_delete_dropped()
    {
        do_delete_dropped(history_tree_);
    }

    void do_delete_dropped(HistoryNode* node)
    {
        if (node->is_dropped() && node->root() != nullptr)
        {
            if (this->is_dump_snapshot_lifecycle()) {
                std::cout << "MEMORIA: DELETE snapshot's DATA (do_delete_dropped): " << node->snapshot_id() << std::endl;
            }

            SnapshotT::delete_snapshot(node);
        }

        for (auto child: node->children())
        {
            do_delete_dropped(child);
        }
    }
    
    virtual void forget_snapshot(HistoryNode* history_node)
    {
    	LockGuardT lock_guard(mutex_);

        snapshot_map_.erase(history_node->snapshot_id());

        if (this->is_dump_snapshot_lifecycle()) {
            std::cout << "MEMORIA: FORGET snapshot from allocator: " << history_node->snapshot_id() << std::endl;
        }

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
        	SnapshotLockGuardT lock_guard(node->snapshot_mutex());

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
    return store::memory::ThreadsMemoryStoreImpl<Profile>::load(input_stream);
}

template <typename Profile>
AllocSharedPtr<IMemoryStore<Profile>> IMemoryStore<Profile>::load(U8String input_file)
{
    auto fileh = FileInputStreamHandler::create(input_file.data());
    return store::memory::ThreadsMemoryStoreImpl<Profile>::load(fileh.get());
}


template <typename Profile>
AllocSharedPtr<IMemoryStore<Profile>> IMemoryStore<Profile>::create()
{
    return MakeShared<store::memory::ThreadsMemoryStoreImpl<Profile>>();
}


}}
