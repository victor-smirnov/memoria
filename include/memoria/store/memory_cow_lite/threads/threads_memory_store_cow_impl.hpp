
// Copyright 2016-2020 Victor Smirnov
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

#include <memoria/store/memory_cow_lite/common/store_base_cow.hpp>

#include <memoria/store/memory_cow_lite/threads/threads_snapshot_cow_impl.hpp>

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
namespace memory_cow {

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
    
    using MutexT		= std::recursive_mutex;
    using SnapshotMutexT	= std::recursive_mutex;
    using StoreMutexT		= std::mutex;

    using LockGuardT		= std::lock_guard<MutexT>;
    using StoreLockGuardT	= std::lock_guard<StoreMutexT>;
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
            MEMORIA_TRY(snapshot, snp_make_shared_init<SnapshotT>(history_tree_, this));
            return snapshot->commit();
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

    void lock() noexcept {
    	mutex_.lock();
    }

    void unlock() noexcept {
    	mutex_.unlock();
    }

    bool try_lock() noexcept {
    	return mutex_.try_lock();
    }
    
    Result<void> pack() noexcept
    {
        std::lock_guard<MutexT> lock(mutex_);
        return do_pack(history_tree_);
    }


    SnapshotID root_shaphot_id() const noexcept
    {
        LockGuardT lock_guard(mutex_);
        return history_tree_->snapshot_id();
    }

    Result<std::vector<SnapshotID>> children_of(const SnapshotID& snapshot_id) const noexcept
    {
        using ResultT = Result<std::vector<SnapshotID>>;
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

        return ResultT::of(std::move(ids));
    }

    Result<std::vector<std::string>> children_of_str(const SnapshotID& snapshot_id) const noexcept
    {
        using ResultT = Result<std::vector<std::string>>;

        std::vector<std::string> ids;

        MEMORIA_TRY(uids, children_of(snapshot_id));
        for (const auto& uid: uids)
        {
            ids.push_back(uid.str());
        }

        return ResultT::of(std::move(ids));
    }

    VoidResult remove_named_branch(const std::string& name) noexcept
    {
        LockGuardT lock_guard(mutex_);
        named_branches_.erase(U8String(name));
        return VoidResult::of();
    }

    Result<std::vector<U8String>> branch_names() noexcept
    {
        using ResultT = Result<std::vector<U8String>>;
        std::lock_guard<MutexT> lock(mutex_);

        std::vector<U8String> branches;

        for (auto pair: named_branches_)
        {
            branches.push_back(pair.first);
        }

        return ResultT::of(branches);
    }

    Result<SnapshotID> branch_head(const U8String& branch_name) noexcept
    {
        using ResultT = Result<SnapshotID>;
        std::lock_guard<MutexT> lock(mutex_);

        auto ii = named_branches_.find(branch_name);
        if (ii != named_branches_.end())
        {
            // TODO: need to take a lock here on the snapshot
            return ResultT::of(ii->second->snapshot_id());
        }

        return ResultT::of(SnapshotID{});
    }

    Result<std::vector<SnapshotID>> branch_heads() noexcept
    {
        using ResultT = Result<std::vector<SnapshotID>>;

        std::lock_guard<MutexT> lock(mutex_);
        std::unordered_set<SnapshotID> ids;

        for (const auto& entry: named_branches_)
        {
            // TODO: need to take a lock here on the snapshot
            ids.insert(entry.second->snapshot_id());
        }

        ids.insert(master_->snapshot_id());

        return ResultT::of(std::vector<SnapshotID>(ids.begin(), ids.end()));
    }


    Result<std::vector<SnapshotID>> heads() noexcept
    {
        using ResultT = Result<std::vector<SnapshotID>>;
        std::lock_guard<MutexT> lock(mutex_);

        std::vector<SnapshotID> heads;

        MEMORIA_TRY_VOID(walk_version_tree(history_tree_, [&](const auto* history_node) -> VoidResult {
            if (history_node->children().size() == 0)
            {
                heads.emplace_back(history_node->snapshot_id());
            }

            return VoidResult::of();
        }));

        return ResultT::of(heads);
    }

    virtual Result<std::vector<SnapshotID>> heads(const SnapshotID& start_from) noexcept
    {
        using ResultT = Result<std::vector<SnapshotID>>;
        std::lock_guard<MutexT> lock(mutex_);

        auto ii = snapshot_map_.find(start_from);
        if (ii != snapshot_map_.end())
        {
            std::vector<SnapshotID> heads;

            auto current = ii->second;
            MEMORIA_TRY_VOID(walk_version_tree(current, [&](const auto* history_node) -> VoidResult {
                if (history_node->snapshot_id() != start_from && history_node->children().size() == 0)
                {
                    heads.emplace_back(history_node->snapshot_id());
                }

                return VoidResult::of();
            }));

            return ResultT::of(heads);
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is not found.", start_from);
        }
    }

    virtual Result<std::vector<SnapshotID>> linear_history(
            const SnapshotID& start_id,
            const SnapshotID& stop_id
    ) noexcept
    {
        using ResultT = Result<std::vector<SnapshotID>>;
        std::lock(mutex_, store_mutex_);

        std::vector<SnapshotID> snps;

        MEMORIA_TRY_VOID(walk_linear_history(start_id, stop_id, [&](const auto* history_node) {
            snps.emplace_back(history_node->snapshot_id());
            return VoidResult::of();
        }));

        std::reverse(snps.begin(), snps.end());
        return ResultT::of(snps);
    }



    
    Result<SnapshotMetadata<ApiProfileT>> describe(const SnapshotID& snapshot_id) const noexcept
    {
        using ResultT = Result<SnapshotMetadata<ApiProfileT>>;

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

            return ResultT::of(SnapshotMetadata<ApiProfileT>(
                parent_id, history_node->snapshot_id(), children, history_node->metadata(), history_node->status()
            ));
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Snapshot id {} is unknown", snapshot_id);
        }
    }


    virtual Result<int32_t> snapshot_status(const SnapshotID& snapshot_id) noexcept
    {
        using ResultT = Result<int32_t>;
        LockGuardT lock_guard2(mutex_);

        auto iter = snapshot_map_.find(snapshot_id);
        if (iter != snapshot_map_.end())
        {
            const auto history_node = iter->second;
            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());
            return ResultT::of((int32_t)history_node->status());
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Snapshot id {} is unknown", snapshot_id);
        }
    }

    Result<SnapshotID> snapshot_parent(const SnapshotID& snapshot_id) noexcept
    {
        using ResultT = Result<SnapshotID>;
        LockGuardT lock_guard2(mutex_);

        auto iter = snapshot_map_.find(snapshot_id);
        if (iter != snapshot_map_.end())
        {
            const auto history_node = iter->second;
            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());

            auto parent_id = history_node->parent() ? history_node->parent()->snapshot_id() : SnapshotID{};
            return ResultT::of(parent_id);
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Snapshot id {} is unknown", snapshot_id);
        }
    }

    Result<U8String> snapshot_description(const SnapshotID& snapshot_id) noexcept
    {
        using ResultT = Result<U8String>;
        LockGuardT lock_guard2(mutex_);

        auto iter = snapshot_map_.find(snapshot_id);
        if (iter != snapshot_map_.end())
        {
            const auto history_node = iter->second;
            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());
            return ResultT::of(history_node->metadata());
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Snapshot id {} is unknown", snapshot_id);
        }
    }




    Result<SnapshotApiPtr> find(const SnapshotID& snapshot_id) noexcept
    {
        //using ResultT = Result<SnapshotApiPtr>;
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
            else {
                return MEMORIA_MAKE_GENERIC_ERROR(
                            "Snapshot {} is {}",
                            history_node->snapshot_id(),
                            (history_node->is_active() ? "active" : "dropped")
                );
            }
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Snapshot id {} is unknown", snapshot_id);
        }
    }




    Result<SnapshotApiPtr> find_branch(U8StringRef name) noexcept
    {
        //using ResultT = Result<SnapshotApiPtr>;
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
            else {                
                return MEMORIA_MAKE_GENERIC_ERROR(
                            "Snapshot {} is {} ",
                            history_node->snapshot_id(),
                            (history_node->is_active() ? "active" : "dropped")
                );
            }
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Named branch \"{}\" is not known", name);
        }
    }

    Result<SnapshotApiPtr> master() noexcept
    {
        //using ResultT = Result<SnapshotApiPtr>;
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

    VoidResult set_master(const SnapshotID& txn_id) noexcept
    {
        using ResultT = VoidResult;
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
            else if (history_node->is_dropped())
            {
                return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} has been dropped", txn_id);
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} hasn't been committed yet", txn_id);
            }

            return ResultT::of();
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is not known in this allocator", txn_id);
        }
    }

    Result<void> set_branch(U8StringRef name, const SnapshotID& txn_id) noexcept
    {
        using ResultT = Result<void>;
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
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} hasn't been committed yet", txn_id);
            }

            return ResultT::of();
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is not known in this allocator", txn_id);
        }
    }

    virtual Result<void> walk_containers(ContainerWalker<Profile>* walker, const char* allocator_descr = nullptr) noexcept
    {
        MEMORIA_TRY_VOID(this->build_snapshot_labels_metadata());

    	LockGuardT lock_guard(mutex_);

        walker->beginAllocator("PersistentInMemAllocator", allocator_descr);

        MEMORIA_TRY_VOID(walk_containers(history_tree_, walker));

        walker->endAllocator();

        snapshot_labels_metadata().clear();
        return Result<void>::of();
    }

    
private:
    virtual VoidResult do_store(OutputStreamHandler *output) noexcept
    {
        using ResultT = VoidResult;

        MEMORIA_TRY_VOID(do_pack(history_tree_));

        records_ = 0;

        char signature[12] = "MEMORIA";
        for (size_t c = 7; c < sizeof(signature); c++) signature[c] = 0;

        output->write(&signature, 0, sizeof(signature));

        MEMORIA_TRY_VOID(write_metadata(*output));
        RCBlockSet stored_blocks;

        auto res2 = walk_version_tree(history_tree_, [&](const HistoryNode* history_tree_node) noexcept {
            return write_history_node(*output, history_tree_node, stored_blocks);
        });
        MEMORIA_RETURN_IF_ERROR(res2);

        Checksum checksum;
        checksum.records() = records_;

        MEMORIA_TRY_VOID(write(*output, checksum));

        output->close();

        return ResultT::of();
    }
public:

    virtual VoidResult store(OutputStreamHandler *output, int64_t wait_duration) noexcept
    {
        std::lock(mutex_, store_mutex_);

        LockGuardT lock_guard2(mutex_, std::adopt_lock);
        StoreLockGuardT lock_guard1(store_mutex_, std::adopt_lock);

        if (wait_duration == 0) {
            active_snapshots_.wait(0);
        }
        else if (!active_snapshots_.waitFor(0, wait_duration)) {
            return MEMORIA_MAKE_GENERIC_ERROR("Active snapshots commit/drop waiting timeout: {} ms", wait_duration);
        }

        return do_store(output);
    }


    Result<void> store(memoria::filesystem::path file_name, int64_t wait_duration) noexcept
    {
        return this->store(file_name.string().c_str(), wait_duration);
    }

    virtual Result<void> store(U8String file_name, int64_t wait_duration) noexcept
    {
        return this->store(file_name.data(), wait_duration);
    }

    virtual Result<void> store(const char* file, int64_t wait_duration) noexcept
    {
        std::lock(mutex_, store_mutex_);

        LockGuardT lock_guard2(mutex_, std::adopt_lock);
        StoreLockGuardT lock_guard1(store_mutex_, std::adopt_lock);

        if (wait_duration == 0) {
            active_snapshots_.wait(0);
        }
        else if (!active_snapshots_.waitFor(0, wait_duration)){
            return MEMORIA_MAKE_GENERIC_ERROR("Active snapshots commit/drop waiting timeout: {} ms", wait_duration);
        }

    	auto fileh = FileOutputStreamHandler::create(file);
        return do_store(fileh.get());
    }


    static Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>> load(const char* file) noexcept
    {
        auto fileh = FileInputStreamHandler::create(U8String(file).data());
        auto rr = Base::load(fileh.get());
        if (rr.is_ok()) {
            return Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>>::of(rr.get());
        }

        return std::move(rr).transfer_error();
    }

    static Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>> load(const U8String& file) noexcept
    {
        auto fileh = FileInputStreamHandler::create(file.data());
        auto rr = Base::load(fileh.get());
        if (rr.is_ok()) {
            return Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>>::of(rr.get());
        }

        return std::move(rr).transfer_error();
    }

    Result<SharedPtr<AllocatorMemoryStat<ApiProfileT>>> memory_stat() noexcept
    {
        using ResultT = Result<SharedPtr<AllocatorMemoryStat<ApiProfileT>>>;
        LockGuardT lock_guard(mutex_);

        _::BlockSet visited_blocks;

        SharedPtr<AllocatorMemoryStat<ApiProfileT>> alloc_stat = MakeShared<AllocatorMemoryStat<ApiProfileT>>(0);

        auto history_visitor = [&](HistoryNode* node) -> VoidResult {
            return wrap_throwing([&]() -> VoidResult {
                SnapshotLockGuardT snapshot_lock_guard(node->snapshot_mutex());

                if (node->is_committed() || node->is_dropped())
                {
                    MEMORIA_TRY(snp, snp_make_shared_init<SnapshotT>(node, this->shared_from_this()));
                    auto snp_stat = snp->do_compute_memory_stat(visited_blocks);
                    alloc_stat->add_snapshot_stat(snp_stat);
                }

                return VoidResult::of();
            });
        };

        MEMORIA_TRY_VOID(this->walk_version_tree(history_tree_, history_visitor));
        alloc_stat->compute_total_size();

        return ResultT::of(alloc_stat);
    }







protected:
    
    VoidResult walk_version_tree(HistoryNode* node, std::function<VoidResult (HistoryNode*, SnapshotT*)> fn) noexcept
    {
        if (node->is_committed())
        {
            MEMORIA_TRY(txn, snp_make_shared_init<SnapshotT>(node, this));
            MEMORIA_TRY_VOID(fn(node, txn.get()));
        }

        for (auto child: node->children())
        {
            MEMORIA_TRY_VOID(walk_version_tree(child, fn));
        }

        return VoidResult::of();
    }

    virtual VoidResult walk_version_tree(HistoryNode* node, std::function<VoidResult (HistoryNode*)> fn) noexcept
    {
        MEMORIA_TRY_VOID(fn(node));

        for (auto child: node->children())
        {
            MEMORIA_TRY_VOID(walk_version_tree(child, fn));
        }

        return VoidResult::of();
    }

    VoidResult walk_containers(HistoryNode* node, ContainerWalker<Profile>* walker) noexcept
    {
    	SnapshotLockGuardT snapshot_lock_guard(node->snapshot_mutex());

        if (node->is_committed())
        {
            MEMORIA_TRY(txn, snp_make_shared_init<SnapshotT>(node, this));
            MEMORIA_TRY_VOID(txn->walkContainers(walker, get_labels_for(node)));
        }

        if (node->children().size())
        {
            walker->beginSnapshotSet("Branches", node->children().size());
            for (auto child: node->children())
            {
                MEMORIA_TRY_VOID(walk_containers(child, walker));
            }
            walker->endSnapshotSet();
        }

        return Result<void>::of();
    }

    
    virtual void free_memory(HistoryNode* node) noexcept
    {
        if (node)
        {
            for (auto child: node->children())
            {
                free_memory(child);
            }

            if (node->root_id().isSet())
            {
                auto txn = snp_make_shared_init<SnapshotT>(node, this).get_or_terminate();
                txn->do_drop();
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
//        if (node->is_dropped() && node->root_id().isSet())
//        {
//            if (this->is_dump_snapshot_lifecycle()) {
//                std::cout << "MEMORIA: DELETE snapshot's DATA (do_delete_dropped): " << node->snapshot_id() << std::endl;
//            }

//            SnapshotT::delete_snapshot(node);
//        }

//        for (auto child: node->children())
//        {
//            do_delete_dropped(child);
//        }
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
    
    
    virtual VoidResult do_pack(HistoryNode* node, int32_t depth, const std::unordered_set<HistoryNode*>& branches) noexcept
    {
    	// FIXME: use dedicated stack data structure

        auto children = node->children();
        for (auto child: children)
        {
            MEMORIA_TRY_VOID(do_pack(child, depth + 1, branches));
        }

        bool remove_node = false;
        {
        	SnapshotLockGuardT lock_guard(node->snapshot_mutex());

            if (node->root_id().is_null() && node->references() == 0 && branches.find(node) == branches.end())
        	{
        		remove_node = true;
        	}
        }

        if (remove_node) {
        	do_remove_history_node(node);
        }

        return Result<void>::of();
    }

    class BTreeNodeSerializationHandler: public BTreeTraverseNodeHandler<Profile> {
        MyType& store_;
        OutputStreamHandler& out_;
        RCBlockSet& stored_blocks_;
    public:
        BTreeNodeSerializationHandler(
                MyType& store,
                OutputStreamHandler& out,
                RCBlockSet& stored_blocks
        ) noexcept:
            store_(store), out_(out), stored_blocks_(stored_blocks)
        {}

        virtual VoidResult process_branch_node(const BlockType* block) noexcept {
            stored_blocks_.insert(block);
            return store_.write_ctr_block(out_, block);
        }

        virtual VoidResult process_leaf_node(const BlockType* block) noexcept {
            stored_blocks_.insert(block);
            return store_.write_ctr_block(out_, block);
        }

        virtual VoidResult process_directory_leaf_node(const BlockType* block) noexcept {
            stored_blocks_.insert(block);
            return store_.write_ctr_block(out_, block);
        }

        virtual BoolResult proceed_with(const BlockID& block_id) const noexcept
        {
            const BlockType* block = value_cast<const BlockType*>(block_id);
            return BoolResult::of(stored_blocks_.count(block) == 0);
        }
    };


    virtual VoidResult serialize_snapshot(
            OutputStreamHandler& out,
            const HistoryNode* history_node,
            RCBlockSet& stored_blocks
    ) noexcept
    {
        MEMORIA_TRY(txn, snp_make_shared_init<SnapshotT>(const_cast<HistoryNode*>(history_node), this));
        BTreeNodeSerializationHandler handler(*this, out, stored_blocks);
        return txn->traverse_ctr(history_node->root_id(), handler);
    }


};

}}


}
