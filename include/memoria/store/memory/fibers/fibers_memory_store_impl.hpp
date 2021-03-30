
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

#include <memoria/core/tools/uuid.hpp>
#include <memoria/core/tools/stream.hpp>
#include <memoria/core/tools/pair.hpp>
#include <memoria/core/tools/latch.hpp>
#include <memoria/core/memory/memory.hpp>

#include <memoria/store/memory/common/store_base.hpp>
#include <memoria/store/memory/fibers/fibers_snapshot_impl.hpp>

#include <memoria/reactor/reactor.hpp>

#include <memoria/fiber/detail/spinlock.hpp>
#include <memoria/fiber/count_down_latch.hpp>
#include <memoria/fiber/shared_mutex.hpp>
#include <memoria/fiber/recursive_shared_mutex.hpp>
#include <memoria/fiber/shared_lock.hpp>

#include <memoria/api/store/memory_store_event_listener.hpp>

#include <stdlib.h>
#include <memory>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>



namespace memoria {
namespace store {
namespace memory {


template <typename Profile>
class FibersMemoryStoreImpl: public MemoryStoreBase<Profile, FibersMemoryStoreImpl<Profile>> {

public:
    using Base      = MemoryStoreBase<Profile, FibersMemoryStoreImpl<Profile>>;
    using MyType    = FibersMemoryStoreImpl<Profile>;
    using ApiProfileT = ApiProfile<Profile>;
    
    using typename Base::BlockType;

    using SnapshotT             = FibersSnapshot<Profile, MyType>;
    using SnapshotPtr           = SnpSharedPtr<SnapshotT>;
    using SnapshotApiPtr        = SnpSharedPtr<IMemorySnapshot<ApiProfileT>>;

    using StorePtr              = AllocSharedPtr<MyType>;
    
    using MutexT                = memoria::fibers::detail::spinlock;
    using LockGuardT            = memoria::fibers::detail::spinlock_lock;
    
    using SnapshotMutexT        = memoria::fibers::detail::spinlock;
    using SnapshotLockGuardT    = memoria::fibers::detail::spinlock_lock;
        
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
    
    memoria::fibers::recursive_shared_mutex store_rw_mutex_;
    
    memoria::fibers::count_down_latch<int64_t> active_snapshots_;

    LocalSharedPtr<StoreEventListener> event_listener_;

    Result<SnapshotApiPtr> upcast(Result<SnapshotPtr>&& ptr) {
        return memoria_static_pointer_cast<IMemorySnapshot<ApiProfileT>>(std::move(ptr));
    }
 
public:
    FibersMemoryStoreImpl(MaybeError& maybe_error) noexcept:
        Base(maybe_error)
    {
        wrap_construction(maybe_error, [&]() noexcept -> VoidResult {
            cpu_ = reactor::engine().cpu();
            MEMORIA_TRY(snapshot, snp_make_shared_init<SnapshotT>(history_tree_, this, OperationType::OP_CREATE));
            return snapshot->commit();
        });
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
    
    int64_t active_snapshots() noexcept {
        return active_snapshots_.get();
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
    
    int32_t cpu() const noexcept {return cpu_;}
    
    Result<void> pack() noexcept
    {
        return reactor::engine().run_at(cpu_, [&]{
            return do_pack(history_tree_);
        });
    }


    SnapshotID root_shaphot_id() const noexcept
    {
        return reactor::engine().run_at(cpu_, [&]{
            return history_tree_->snapshot_id();
        });
    }

    Result<std::vector<SnapshotID>> children_of(const SnapshotID& snapshot_id) const noexcept
    {
        using ResultT = Result<std::vector<SnapshotID>>;
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

            return ResultT::of(std::move(ids));
        });
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

    Result<void> remove_named_branch(const std::string& name) noexcept
    {
        return reactor::engine().run_at(cpu_, [&]{
            named_branches_.erase(U8String(name));
            return Result<void>::of();
        });
    }

    Result<std::vector<U8String>> branch_names() noexcept
    {
        using ResultT = Result<std::vector<U8String>>;
        return reactor::engine().run_at(cpu_, [&]{
            std::vector<U8String> branches;

            for (auto pair: named_branches_)
            {
                branches.push_back(pair.first);
            }

            return ResultT::of(branches);
        });
    }

    Result<SnapshotID> branch_head(const U8String& branch_name) noexcept
    {
        using ResultT = Result<SnapshotID>;
        return reactor::engine().run_at(cpu_, [&]{

            auto ii = named_branches_.find(branch_name);
            if (ii != named_branches_.end())
            {
                return ResultT::of(ii->second->snapshot_id());
            }

            return ResultT::of(SnapshotID{});
        });
    }

    Result<std::vector<SnapshotID>> branch_heads() noexcept
    {
        using ResultT = Result<std::vector<SnapshotID>>;

        // FIXME: rewrite to use Reactor instead of locks
        std::lock_guard<MutexT> lock(mutex_);
        std::unordered_set<SnapshotID> ids;

        for (const auto& entry: named_branches_)
        {
            ids.insert(entry.second->snapshot_id());
        }

        ids.insert(master_->snapshot_id());

        return ResultT::of(std::vector<SnapshotID>(ids.begin(), ids.end()));
    }

    Result<std::vector<SnapshotID>> heads() noexcept
    {
        using ResultT = Result<std::vector<SnapshotID>>;

        // FIXME: rewrite to use Reactor instead of locks
        std::lock_guard<MutexT> lock(mutex_);
        std::unordered_set<SnapshotID> ids;

//        for (const auto& entry: named_branches_)
//        {
//            // TODO: need to take a lock here on the snapshot
//            ids.insert(entry.second->snapshot_id());
//        }

//        ids.insert(master_->snapshot_id());

        return ResultT::of(std::vector<SnapshotID>(ids.begin(), ids.end()));
    }

    Int32Result snapshot_status(const SnapshotID& snapshot_id) noexcept
    {
        return reactor::engine().run_at(cpu_, [&] () -> Int32Result {
            auto iter = snapshot_map_.find(snapshot_id);
            if (iter != snapshot_map_.end())
            {
                const auto history_node = iter->second;
                return Int32Result::of((int32_t)history_node->status());
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Snapshot id {} is unknown", snapshot_id);
            }
        });
    }

    Result<SnapshotID> snapshot_parent(const SnapshotID& snapshot_id) noexcept
    {
        using ResultT = Result<SnapshotID>;
        return reactor::engine().run_at(cpu_, [&]() -> ResultT {
            auto iter = snapshot_map_.find(snapshot_id);
            if (iter != snapshot_map_.end())
            {
                const auto history_node = iter->second;
                auto parent_id = history_node->parent() ? history_node->parent()->snapshot_id() : SnapshotID{};
                return ResultT::of(parent_id);
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Snapshot id {} is unknown", snapshot_id);
            }
        });
    }

    Result<U8String> snapshot_description(const SnapshotID& snapshot_id) noexcept
    {
        using ResultT = Result<U8String>;
        return reactor::engine().run_at(cpu_, [&]() -> ResultT{
            auto iter = snapshot_map_.find(snapshot_id);
            if (iter != snapshot_map_.end())
            {
                const auto history_node = iter->second;
                return ResultT::of(history_node->metadata());
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Snapshot id {} is unknown", snapshot_id);
            }
        });
    }




    Result<SnpSharedPtr<SnapshotMetadata<ApiProfileT>>> describe(const SnapshotID& snapshot_id) const noexcept
    {
        using ResultT = Result<SnpSharedPtr<SnapshotMetadata<ApiProfileT>>>;
        return reactor::engine().run_at(cpu_, [&] () -> ResultT {
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

                return ResultT::of(snp_make_shared<SnapshotMetadata<ApiProfileT>>(
                    parent_id, history_node->snapshot_id(), children, history_node->metadata(), history_node->status()
                ));
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Snapshot id {} is unknown", snapshot_id);
            }
        });
    }



    Result<SnapshotApiPtr> find(const SnapshotID& snapshot_id) noexcept
    {
        using ResultT = Result<SnapshotApiPtr>;
        return reactor::engine().run_at(cpu_, [&]() -> ResultT
        {
            auto iter = snapshot_map_.find(snapshot_id);
            if (iter != snapshot_map_.end())
            {
                auto history_node = iter->second;

                if (history_node->is_committed())
                {
                    return upcast(snp_make_shared_init<SnapshotT>(history_node, this->shared_from_this(), OperationType::OP_FIND));
                }
                if (history_node->is_data_locked())
                {
                    return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} data is locked", history_node->snapshot_id());
                }
                else {
                    return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is {}", history_node->snapshot_id(), (history_node->is_active() ? "active" : "dropped"));
                }
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Snapshot id {} is unknown", snapshot_id);
            }
        });
    }




    Result<SnapshotApiPtr> find_branch(U8StringRef name) noexcept
    {
        using ResultT = Result<SnapshotApiPtr>;

        return reactor::engine().run_at(cpu_, [&] () -> ResultT {
            auto iter = named_branches_.find(name);
            if (iter != named_branches_.end())
            {
                auto history_node = iter->second;

                if (history_node->is_committed())
                {
                    return upcast(snp_make_shared_init<SnapshotT>(history_node, this->shared_from_this(), OperationType::OP_FIND));
                }
                else if (history_node->is_data_locked())
                {
                    if (history_node->references() == 0)
                    {
                        return upcast(snp_make_shared_init<SnapshotT>(history_node, this->shared_from_this(), OperationType::OP_FIND));
                    }
                    else {
                        return MEMORIA_MAKE_GENERIC_ERROR("Snapshot id {} is locked and open", history_node->snapshot_id());
                    }
                }
                else {
                    return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is {}", history_node->snapshot_id(), (history_node->is_active() ? "active" : "dropped"));
                }
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Named branch \"{}\" is not known", name);
            }
        });
    }

    Result<SnapshotApiPtr> master() noexcept
    {
        return reactor::engine().run_at(cpu_, [&]{
            return upcast(snp_make_shared_init<SnapshotT>(master_, this->shared_from_this(), OperationType::OP_FIND));
        });
    }

    SnpSharedPtr<SnapshotMetadata<ApiProfileT>> describe_master() const
    {
        return reactor::engine().run_at(cpu_, [&]{
            std::vector<SnapshotID> children;

            for (const auto& node: master_->children())
            {
                children.emplace_back(node->snapshot_id());
            }

            auto parent_id = master_->parent() ? master_->parent()->snapshot_id() : SnapshotID{};

            return snp_make_shared<SnapshotMetadata<ApiProfileT>>(
                parent_id, master_->snapshot_id(), children, master_->metadata(), master_->status()
            );
        });
    }

    Result<void> set_master(const SnapshotID& txn_id) noexcept
    {
        using ResultT = Result<void>;
        return reactor::engine().run_at(cpu_, [&] () -> ResultT
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
        });
    }

    Result<void> set_branch(U8StringRef name, const SnapshotID& txn_id) noexcept
    {
        using ResultT = Result<void>;
        return reactor::engine().run_at(cpu_, [&]() -> ResultT
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
                    return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} hasn't been committed yet", txn_id);
                }

                return ResultT::of();
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is not known in this allocator", txn_id);
            }
        });
    }


    virtual Result<void> walk_containers(ContainerWalker<Profile>* walker, const char* allocator_descr) noexcept
    {
        using ResultT = Result<void>;
        return reactor::engine().run_at(cpu_, [&]() noexcept -> ResultT
        {
            MEMORIA_TRY_VOID(this->build_snapshot_labels_metadata());

            walker->beginAllocator("PersistentInMemAllocator", allocator_descr);

            MEMORIA_TRY_VOID(walk_containers(history_tree_, walker));

            walker->endAllocator();

            snapshot_labels_metadata().clear();
            return ResultT::of();
        });
    }

    

    virtual Result<void> store(OutputStreamHandler *output, int64_t wait_duration) noexcept
    {
        using ResultT = Result<void>;
        return reactor::engine().run_at(cpu_, [&]() noexcept -> ResultT {

            active_snapshots_.wait(0);
            
            MEMORIA_TRY_VOID(do_pack(history_tree_));

            records_ = 0;

            char signature[12] = "MEMORIA";
            for (size_t c = 7; c < sizeof(signature); c++) signature[c] = 0;

            output->write(&signature, 0, sizeof(signature));

            MEMORIA_TRY_VOID(write_metadata(*output));

            RCBlockSet stored_blocks;

            auto res2 = walk_version_tree(history_tree_, [&](const HistoryNode* history_tree_node) {
                return write_history_node(*output, history_tree_node, stored_blocks);
            });
            MEMORIA_RETURN_IF_ERROR(res2);

            Checksum checksum;
            checksum.records() = records_;

            MEMORIA_TRY_VOID(write(*output, checksum));

            output->close();

            return ResultT::of();
        });
    }

    virtual Result<void> store(U8String file_name, int64_t wait_duration) noexcept
    {
        auto fileh = FileOutputStreamHandler::create_buffered(file_name.to_std_string());
        return this->store(fileh.get(), wait_duration);
    }

    virtual Result<void> store(filesystem::path file) noexcept
    {
    	auto fileh = FileOutputStreamHandler::create_buffered(file);
        return store(fileh.get(), 0);
    }


    static Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>> load(const char* file, int32_t cpu) noexcept
    {
        auto rr = reactor::engine().run_at(cpu, [&]{
            auto fileh = FileInputStreamHandler::create(U8String(file).data());
            return Base::load(fileh.get());
        });

        if (rr.is_ok()) {
            return Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>>::of(std::move(rr).get());
        }
        else
        {
            return std::move(rr).transfer_error();
        }
    }
    
    static Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>> load(const char* file) noexcept
    {
        return load(file, reactor::engine().cpu());
    }
    
    static Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>> create(int32_t cpu) noexcept
    {
        using ResultT = Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>>;
        return reactor::engine().run_at(cpu, [cpu]() noexcept -> ResultT {
            MEMORIA_TRY(alloc, Base::create());
            static_cast<FibersMemoryStoreImpl*>(alloc.get())->cpu_ = cpu;
            return alloc_result;
        });
    }
    
    static Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>> create() noexcept
    {
        return create(reactor::engine().cpu());
    }




    Result<SharedPtr<AllocatorMemoryStat<ApiProfileT>>> memory_stat() noexcept
    {
        using ResultT = Result<SharedPtr<AllocatorMemoryStat<ApiProfileT>>>;
        return reactor::engine().run_at(cpu_, [&]() noexcept -> ResultT {
            _::BlockSet visited_blocks;

            SharedPtr<AllocatorMemoryStat<ApiProfileT>> alloc_stat = MakeShared<AllocatorMemoryStat<ApiProfileT>>(0);

            auto history_visitor = [&](HistoryNode* node) noexcept -> VoidResult {
                return wrap_throwing([&]() -> VoidResult {
                    if (node->is_committed() || node->is_dropped())
                    {
                        MEMORIA_TRY(snp, snp_make_shared_init<SnapshotT>(node, this->shared_from_this(), OperationType::OP_FIND));
                        auto snp_stat = snp->do_compute_memory_stat(visited_blocks);
                        alloc_stat->add_snapshot_stat(snp_stat);
                    }
                    return VoidResult::of();
                });
            };

            MEMORIA_TRY_VOID(this->walk_version_tree(history_tree_, history_visitor));
            alloc_stat->compute_total_size();

            return ResultT::of(std::move(alloc_stat));
        });
    }

    virtual Result<std::vector<SnapshotID>> linear_history(
            const SnapshotID& start_id,
            const SnapshotID& stop_id
    ) noexcept
    {
        using ResultT = Result<std::vector<SnapshotID>>;
        return ResultT::of();
    }

    virtual Result<std::vector<SnapshotID>> heads(const SnapshotID& start_from) noexcept {
        return heads();
    }

protected:
    
    VoidResult walk_version_tree(HistoryNode* node, std::function<VoidResult (HistoryNode*, SnapshotT*)> fn) noexcept
    {
        return wrap_throwing([&]() -> VoidResult {
            if (node->is_committed())
            {
                MEMORIA_TRY(txn, snp_make_shared_init<SnapshotT>(node, this, OperationType::OP_FIND));
                MEMORIA_TRY_VOID(fn(node, txn.get()));
            }

            for (auto child: node->children())
            {
                MEMORIA_TRY_VOID(walk_version_tree(child, fn));
            }

            return VoidResult::of();
        });
    }

    virtual VoidResult walk_version_tree(HistoryNode* node, std::function<VoidResult (HistoryNode*)> fn) noexcept
    {
        return wrap_throwing([&]() -> VoidResult {
            MEMORIA_TRY_VOID(fn(node));
            for (auto child: node->children())
            {
                MEMORIA_TRY_VOID(walk_version_tree(child, fn));
            }

            return VoidResult::of();
        });
    }

    VoidResult walk_containers(HistoryNode* node, ContainerWalker<Profile>* walker) noexcept
    {
        return wrap_throwing([&]() -> VoidResult {
            if (node->is_committed())
            {
                MEMORIA_TRY(txn, snp_make_shared_init<SnapshotT>(node, this, OperationType::OP_FIND));
                MEMORIA_TRY_VOID(txn->walk_containers(walker, get_labels_for(node)));
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
        });
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
    
    
    virtual Result<void> do_pack(HistoryNode* node, int32_t depth, const std::unordered_set<HistoryNode*>& branches) noexcept
    {
    	// FIXME: use dedicated stack data structure

        using ResultT = Result<void>;

        auto children = node->children();
        for (auto child: children)
        {
            MEMORIA_TRY_VOID(do_pack(child, depth + 1, branches));
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

        return ResultT::of();
    }
};


}}

/*
template <typename Profile>
Result<AllocSharedPtr<IMemoryStore<Profile>>> IMemoryStore<Profile>::load(InputStreamHandler* input_stream) noexcept
{
    auto rr = store::memory::FibersMemoryStoreImpl<DefaultProfile<>>::load(input_stream);
    if (rr.is_ok()) {
        return Result<AllocSharedPtr<IMemoryStore<Profile>>>::of(std::move(rr).get());
    }
    else
    {
        return std::move(rr).transfer_error();
    }
}

template <typename Profile>
Result<AllocSharedPtr<IMemoryStore<Profile>>> IMemoryStore<Profile>::load(U8String input_file) noexcept
{
    auto fileh = FileInputStreamHandler::create(input_file.data());
    auto rr = store::memory::FibersMemoryStoreImpl<DefaultProfile<>>::load(fileh.get());
    if (rr.is_ok()) {
        return Result<AllocSharedPtr<IMemoryStore<Profile>>>::of(std::move(rr).get());
    }
    else
    {
        return std::move(rr).transfer_error();
    }
}


template <typename Profile>
Result<AllocSharedPtr<IMemoryStore<Profile>>> IMemoryStore<Profile>::create() noexcept
{
    using ResultT = Result<AllocSharedPtr<IMemoryStore<Profile>>>;
    MaybeError maybe_error;

    auto snp = MakeShared<store::memory::FibersMemoryStoreImpl<DefaultProfile<>>>(maybe_error);

    if (!maybe_error) {
        return ResultT::of(std::move(snp));
    }
    else {
        return std::move(maybe_error.get());
    }
}
*/


}
