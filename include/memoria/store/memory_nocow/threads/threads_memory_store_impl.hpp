
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

#include <memoria/containers/map/map_factory.hpp>


#include <memoria/core/tools/stream.hpp>
#include <memoria/core/tools/pair.hpp>
#include <memoria/core/tools/latch.hpp>
#include <memoria/core/memory/memory.hpp>

#include <memoria/store/memory_nocow/common/store_base.hpp>

#include <memoria/store/memory_nocow/threads/threads_snapshot_impl.hpp>

#include <memoria/filesystem/path.hpp>

#include <cstdlib>
#include <memory>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

namespace memoria {
namespace store {
namespace memory_nocow {

template <typename Profile>
class ThreadsMemoryStoreImpl: public MemoryStoreBase<Profile, ThreadsMemoryStoreImpl<Profile>> {
public:
    using Base      = MemoryStoreBase<Profile, ThreadsMemoryStoreImpl<Profile>>;
    using MyType    = ThreadsMemoryStoreImpl<Profile>;

    using ApiProfileT = ApiProfile<Profile>;
    
    using typename Base::BlockType;

    using SnapshotT             = ThreadsSnapshot<Profile, MyType>;
    using SnapshotPtr           = SnpSharedPtr<SnapshotT>;
    using SnapshotApiPtr        = SnpSharedPtr<IMemorySnapshot<ApiProfileT>>;

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
    using Base::walk_linear_history;

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
    ThreadsMemoryStoreImpl(MaybeError& maybe_error) noexcept:
        Base(maybe_error)
    {
        wrap_construction(maybe_error, [&]() -> VoidResult {
            auto snapshot = snp_make_shared_init<SnapshotT>(history_tree_, this);
            snapshot->commit();
            return VoidResult::of();
        });
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

    SnapshotApiPtr upcast(SnapshotPtr&& ptr) {
        return memoria_static_pointer_cast<IMemorySnapshot<ApiProfileT>>(ptr);
    }

    Result<SnapshotApiPtr> upcast(Result<SnapshotPtr>&& ptr) {
        return memoria_static_pointer_cast<IMemorySnapshot<ApiProfileT>>(std::move(ptr));
    }

public:
    
    int64_t active_snapshots() noexcept {
        return active_snapshots_.get();
    }

    
    MutexT& mutex() noexcept{
    	return mutex_;
    }

    const MutexT& mutex() const noexcept {
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
        return do_pack(history_tree_);
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

        return std::move(ids);
    }

    std::vector<std::string> children_of_str(const SnapshotID& snapshot_id) const
    {
        std::vector<std::string> ids;

        auto uids = children_of(snapshot_id);
        for (const auto& uid: uids)
        {
            ids.push_back(uid.str());
        }

        return std::move(ids);
    }

    void remove_named_branch(const std::string& name)
    {
        LockGuardT lock_guard(mutex_);
        named_branches_.erase(U8String(name));
    }

    std::vector<U8String> branch_names()
    {
        std::lock_guard<MutexT> lock(mutex_);

        std::vector<U8String> branches;

        for (auto pair: named_branches_)
        {
            branches.push_back(pair.first);
        }

        return branches;
    }

    SnapshotID branch_head(const U8String& branch_name)
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

    std::vector<SnapshotID> branch_heads()
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


    std::vector<SnapshotID> heads()
    {
        std::lock_guard<MutexT> lock(mutex_);

        std::vector<SnapshotID> heads;

        walk_version_tree(history_tree_, [&](const auto* history_node) {
            if (history_node->children().size() == 0)
            {
                heads.emplace_back(history_node->snapshot_id());
            }

        });

        return heads;
    }

    virtual std::vector<SnapshotID> heads(const SnapshotID& start_from)
    {
        std::lock_guard<MutexT> lock(mutex_);

        auto ii = snapshot_map_.find(start_from);
        if (ii != snapshot_map_.end())
        {
            std::vector<SnapshotID> heads;

            auto current = ii->second;
            walk_version_tree(current, [&](const auto* history_node) {
                if (history_node->snapshot_id() != start_from && history_node->children().size() == 0)
                {
                    heads.emplace_back(history_node->snapshot_id());
                }
            });

            return heads;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is not found.", start_from).do_throw();
        }
    }

    virtual std::vector<SnapshotID> linear_history(
            const SnapshotID& start_id,
            const SnapshotID& stop_id
    ) noexcept
    {
        std::lock(mutex_, store_mutex_);

        std::vector<SnapshotID> snps;

        walk_linear_history(start_id, stop_id, [&](const auto* history_node) {
            snps.emplace_back(history_node->snapshot_id());
        });

        std::reverse(snps.begin(), snps.end());
        return snps;
    }



    
    SnapshotMetadata<ApiProfileT> describe(const SnapshotID& snapshot_id) const
    {
    	LockGuardT lock_guard2(mutex_);

        auto iter = snapshot_map_.find(snapshot_id);
        if (iter != snapshot_map_.end())
        {
            const auto history_node = iter->second;

            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());

            std::vector<SnapshotID> children;

            for (const auto& node: history_node->children()) {
                    children.emplace_back(node->snapshot_id());
            }

            auto parent_id = history_node->parent() ? history_node->parent()->snapshot_id() : SnapshotID{};

            return SnapshotMetadata<ApiProfileT>(
                parent_id, history_node->snapshot_id(), children, history_node->metadata(), history_node->status()
            );
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot id {} is unknown", snapshot_id).do_throw();
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
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot id {} is unknown", snapshot_id).do_throw();
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
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot id {} is unknown", snapshot_id).do_throw();
        }
    }

    U8String snapshot_description(const SnapshotID& snapshot_id)
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
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot id {} is unknown", snapshot_id).do_throw();
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
                MEMORIA_MAKE_GENERIC_ERROR(
                            "Snapshot {} data is locked",
                            history_node->snapshot_id()
                ).do_throw();
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR(
                            "Snapshot {} is {}",
                            history_node->snapshot_id(),
                            (history_node->is_active() ? "active" : "dropped")
                ).do_throw();
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot id {} is unknown", snapshot_id).do_throw();
        }
    }




    SnapshotApiPtr find_branch(U8StringRef name)
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
                    MEMORIA_MAKE_GENERIC_ERROR(
                                "Snapshot id {} is locked and open",
                                history_node->snapshot_id()
                    ).do_throw();
            	}
            }
            else {                
                MEMORIA_MAKE_GENERIC_ERROR(
                            "Snapshot {} is {} ",
                            history_node->snapshot_id(),
                            (history_node->is_active() ? "active" : "dropped")
                ).do_throw();
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Named branch \"{}\" is not known", name).do_throw();
        }
    }

    SnapshotApiPtr master()
    {
        std::lock(mutex_, master_->snapshot_mutex());

    	LockGuardT lock_guard(mutex_, std::adopt_lock);
    	SnapshotLockGuardT snapshot_lock_guard(master_->snapshot_mutex(), std::adopt_lock);

        return upcast(snp_make_shared_init<SnapshotT>(master_, this->shared_from_this()));
    }

    SnapshotMetadata<ApiProfileT> describe_master() const noexcept
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

        return SnapshotMetadata<ApiProfileT>(
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
                MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} has been dropped", txn_id).do_throw();
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} hasn't been committed yet", txn_id).do_throw();
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is not known in this allocator", txn_id).do_throw();
        }
    }

    void set_branch(U8StringRef name, const SnapshotID& txn_id)
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
                MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} hasn't been committed yet", txn_id).do_throw();
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is not known in this allocator", txn_id).do_throw();
        }
    }

    virtual void walk_containers(ContainerWalker<Profile>* walker, const char* allocator_descr = nullptr)
    {
        this->build_snapshot_labels_metadata();

    	LockGuardT lock_guard(mutex_);

        walker->beginAllocator("PersistentInMemAllocator", allocator_descr);

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
            return write_history_node(*output, history_tree_node, stored_blocks);
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
            MEMORIA_MAKE_GENERIC_ERROR("Active snapshots commit/drop waiting timeout: {} ms", wait_duration).do_throw();
        }

        return do_store(output);
    }


    void store(memoria::filesystem::path file_name, int64_t wait_duration)
    {
        return this->store(file_name.string().c_str(), wait_duration);
    }

    virtual void store(U8String file_name, int64_t wait_duration)
    {
        return this->store(file_name.data(), wait_duration);
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
            MEMORIA_MAKE_GENERIC_ERROR("Active snapshots commit/drop waiting timeout: {} ms", wait_duration).do_throw();
        }

    	auto fileh = FileOutputStreamHandler::create(file);
        return do_store(fileh.get());
    }


    static AllocSharedPtr<IMemoryStore<ApiProfileT>> load(const char* file)
    {
        auto fileh = FileInputStreamHandler::create(U8String(file).data());
        return Base::load(fileh.get());
    }

    static AllocSharedPtr<IMemoryStore<ApiProfileT>> load(const U8String& file)
    {
        auto fileh = FileInputStreamHandler::create(file.data());
        return Base::load(fileh.get());
    }

    SharedPtr<StoreMemoryStat<ApiProfileT>> memory_stat()
    {
        LockGuardT lock_guard(mutex_);

        _::BlockSet visited_blocks;

        SharedPtr<StoreMemoryStat<ApiProfileT>> alloc_stat = MakeShared<StoreMemoryStat<ApiProfileT>>(0);

        auto history_visitor = [&](HistoryNode* node) {
            return wrap_throwing([&] {
                SnapshotLockGuardT snapshot_lock_guard(node->snapshot_mutex());

                if (node->is_committed() || node->is_dropped())
                {
                    auto snp = snp_make_shared_init<SnapshotT>(node, this->shared_from_this());
                    auto snp_stat = snp->do_compute_memory_stat(visited_blocks);
                    alloc_stat->add_snapshot_stat(snp_stat);
                }
            });

        };

        this->walk_version_tree(history_tree_, history_visitor);
        alloc_stat->compute_total_size();

        return alloc_stat;
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
            walker->beginSnapshotSet("Branches", node->children().size());
            for (auto child: node->children())
            {
                walk_containers(child, walker);
            }
            walker->endSnapshotSet();
        }
    }

    
    virtual void free_memory(HistoryNode* node) noexcept
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

}
