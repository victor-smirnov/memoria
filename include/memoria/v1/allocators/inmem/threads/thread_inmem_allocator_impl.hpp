
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

#include "../common/allocator_base.hpp"

#include "threads_snapshot_impl.hpp"


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

template <typename Profile>
class ThreadInMemAllocatorImpl: public InMemAllocatorBase<Profile, ThreadInMemAllocatorImpl<Profile>> { 
public:
    using Base      = InMemAllocatorBase<Profile, ThreadInMemAllocatorImpl<Profile>>;
    using MyType    = ThreadInMemAllocatorImpl<Profile>;
    
    using typename Base::Page;

    using SnapshotT             = v1::persistent_inmem::ThreadSnapshot<Profile, MyType>;
    using SnapshotPtr           = SnpSharedPtr<SnapshotT>;
    using AllocatorPtr          = AllocSharedPtr<MyType>;
    
    using MutexT			= std::recursive_mutex;
    using SnapshotMutexT	= std::recursive_mutex;
    using StoreMutexT		= std::mutex;

    using LockGuardT			= std::lock_guard<MutexT>;
    using StoreLockGuardT		= std::lock_guard<StoreMutexT>;
    using SnapshotLockGuardT	= std::lock_guard<SnapshotMutexT>;
    
    using typename Base::HistoryNode;
    using typename Base::HistoryNodeBuffer;
    using typename Base::TxnId;
    using typename Base::RCPageSet;
    using typename Base::Checksum;
    
    using Base::load;

protected:
    using Base::history_tree_;
    using Base::snapshot_map_;
    using Base::named_branches_;
    using Base::master_;
    using Base::metadata_;
    using Base::snapshot_labels_metadata;
    //using Base::active_snapshots_;
    using Base::records_;
    using Base::write_metadata;
    using Base::write_history_node;
    using Base::write;
    using Base::do_pack;
    using Base::get_labels_for;
    using Base::do_remove_history_node;

public:    
    template <typename, typename>
    friend class InMemAllocatorBase;
    
    template <typename, typename, typename>
    friend class SnapshotBase;
    
    template <typename, typename>
    friend class ThreadSnapshot;

private:
    
    mutable MutexT mutex_;
    mutable StoreMutexT store_mutex_;
    
    CountDownLatch<int64_t> active_snapshots_;
 
public:
    ThreadInMemAllocatorImpl() {
        auto snapshot = snp_make_shared_init<SnapshotT>(history_tree_, this);
        snapshot->commit();
    }
    
    
    virtual ~ThreadInMemAllocatorImpl()
    {
        free_memory(history_tree_);
    }


    ThreadInMemAllocatorImpl(int32_t v): Base(v) {}

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

    UUID root_shaphot_id() const {
        return history_tree_->txn_id();
    }

    std::vector<UUID> children_of(const UUID& snapshot_id) const
    {
        std::vector<UUID> ids;

        LockGuardT lock_guard(mutex_);

        auto iter = snapshot_map_.find(snapshot_id);
        if (iter != snapshot_map_.end())
        {
            const auto history_node = iter->second;
            for (const auto* child: history_node->children())
            {
                ids.push_back(child->txn_id());
            }
        }

        return ids;
    }

    std::vector<std::string> children_of_str(const UUID& snapshot_id) const
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

    UUID branch_head(const U16String& branch_name)
    {
        std::lock_guard<MutexT> lock(mutex_);

        auto ii = named_branches_.find(branch_name);
        if (ii != named_branches_.end())
        {
            return ii->second->txn_id();
        }

        return UUID{};
    }

    
    SnapshotMetadata<TxnId> describe(const TxnId& snapshot_id) const
    {
    	LockGuardT lock_guard2(mutex_);

        auto iter = snapshot_map_.find(snapshot_id);
        if (iter != snapshot_map_.end())
        {
            const auto history_node = iter->second;

            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());

        	std::vector<TxnId> children;

        	for (const auto& node: history_node->children())
        	{
        		children.emplace_back(node->txn_id());
        	}

        	auto parent_id = history_node->parent() ? history_node->parent()->txn_id() : UUID();

        	return SnapshotMetadata<TxnId>(
        		parent_id, history_node->txn_id(), children, history_node->metadata(), history_node->status()
			);
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot id {} is unknown", snapshot_id));
        }
    }



    SnapshotPtr find(const TxnId& snapshot_id)
    {
    	LockGuardT lock_guard(mutex_);

        auto iter = snapshot_map_.find(snapshot_id);
        if (iter != snapshot_map_.end())
        {
            auto history_node = iter->second;

            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());

            if (history_node->is_committed())
            {
                return snp_make_shared_init<SnapshotT>(history_node, this->shared_from_this());
            }
            if (history_node->is_data_locked())
            {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} data is locked", history_node->txn_id()));
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} is {}", history_node->txn_id(), (history_node->is_active() ? u"active" : u"dropped")));
            }
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot id {} is unknown", snapshot_id));
        }
    }




    SnapshotPtr find_branch(U16StringRef name)
    {
    	LockGuardT lock_guard(mutex_);

    	auto iter = named_branches_.find(name);
        if (iter != named_branches_.end())
        {
            auto history_node = iter->second;

            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());

            if (history_node->is_committed())
            {
                return snp_make_shared_init<SnapshotT>(history_node, this->shared_from_this());
            }
            else if (history_node->is_data_locked())
            {
            	if (history_node->references() == 0)
            	{
            		return snp_make_shared_init<SnapshotT>(history_node, this->shared_from_this());
            	}
            	else {
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot id {} is locked and open", history_node->txn_id()));
            	}
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} is {} ", history_node->txn_id(), (history_node->is_active() ? u"active" : u"dropped")));
            }
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Named branch \"{}\" is not known", name));
        }
    }

    SnapshotPtr master()
    {
    	std::lock(mutex_, master_->snapshot_mutex());

    	LockGuardT lock_guard(mutex_, std::adopt_lock);
    	SnapshotLockGuardT snapshot_lock_guard(master_->snapshot_mutex(), std::adopt_lock);

        return snp_make_shared_init<SnapshotT>(master_, this->shared_from_this());
    }

    SnapshotMetadata<TxnId> describe_master() const
    {
    	std::lock(mutex_, master_->snapshot_mutex());
    	LockGuardT lock_guard2(mutex_, std::adopt_lock);
    	SnapshotLockGuardT snapshot_lock_guard(master_->snapshot_mutex(), std::adopt_lock);

    	std::vector<TxnId> children;

    	for (const auto& node: master_->children())
    	{
    		children.emplace_back(node->txn_id());
    	}

    	auto parent_id = master_->parent() ? master_->parent()->txn_id() : UUID();

    	return SnapshotMetadata<TxnId>(
            parent_id, master_->txn_id(), children, master_->metadata(), master_->status()
    	);
    }

    void set_master(const TxnId& txn_id)
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

    void set_branch(U16StringRef name, const TxnId& txn_id)
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

    ContainerMetadataRepository* getMetadata() const
    {
        return metadata_;
    }

    virtual void walkContainers(ContainerWalker* walker, const char16_t* allocator_descr = nullptr)
    {
    	this->build_snapshot_labels_metadata();

    	LockGuardT lock_guard(mutex_);

        walker->beginAllocator(u"PersistentInMemAllocator", allocator_descr);

        walk_containers(history_tree_, walker);

        walker->endAllocator();

        snapshot_labels_metadata().clear();
    }

    

    virtual void store(OutputStreamHandler *output)
    {
    	std::lock(mutex_, store_mutex_);

    	LockGuardT lock_guard2(mutex_, std::adopt_lock);
    	StoreLockGuardT lock_guard1(store_mutex_, std::adopt_lock);

    	active_snapshots_.wait(0);

    	do_pack(history_tree_);

        records_ = 0;

        char signature[12] = "MEMORIA";
        for (size_t c = 7; c < sizeof(signature); c++) signature[c] = 0;

        output->write(&signature, 0, sizeof(signature));

        write_metadata(*output);

        RCPageSet stored_pages;

        walk_version_tree(history_tree_, [&](const HistoryNode* history_tree_node) {
            write_history_node(*output, history_tree_node, stored_pages);
        });

        Checksum checksum;
        checksum.records() = records_;

        write(*output, checksum);

        output->close();
    }


    virtual void store(const char* file)
    {
    	auto fileh = FileOutputStreamHandler::create(file);
        store(fileh.get());
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

    void walk_containers(HistoryNode* node, ContainerWalker* walker)
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
    	LockGuardT lock_guard(mutex_);

        snapshot_map_.erase(history_node->txn_id());

        history_node->remove_from_parent();

        delete history_node;
    }
    
    
    virtual void do_pack(HistoryNode* node, int32_t depth, const std::unordered_set<HistoryNode*>& branches)
    {
    	// FIXME: use dedicated stack data structure

        for (auto child: node->children())
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

}


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
    return ThreadInMemAllocator<Profile>(PImpl::load(U8String(file_name.string()).to_u16()));
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
typename ThreadInMemAllocator<Profile>::SnapshotPtr ThreadInMemAllocator<Profile>::find_branch(U16StringRef name)
{
    return pimpl_->find_branch(name);
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::set_master(const TxnId& txn_id) 
{
    pimpl_->set_master(txn_id);
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::set_branch(U16StringRef name, const TxnId& txn_id)
{
    pimpl_->set_branch(name, txn_id);
}

template <typename Profile>
ContainerMetadataRepository* ThreadInMemAllocator<Profile>::metadata() const 
{
    return pimpl_->getMetadata();
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::walk_containers(ContainerWalker* walker, const char16_t* allocator_descr)
{
     return pimpl_->walkContainers(walker, allocator_descr);
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
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::lock() {
    return pimpl_->lock();
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::unlock(){
    return pimpl_->unlock();
}

template <typename Profile>
bool ThreadInMemAllocator<Profile>::try_lock() {
    return pimpl_->try_lock();
}


template <typename Profile>
std::vector<UUID> ThreadInMemAllocator<Profile>::children_of(const UUID& snapshot_id) const
{
    return pimpl_->children_of(snapshot_id);
}


template <typename Profile>
std::vector<std::string> ThreadInMemAllocator<Profile>::children_of_str(const UUID& snapshot_id) const
{
    return pimpl_->children_of_str(snapshot_id);
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::remove_named_branch(const std::string& name)
{
    return pimpl_->remove_named_branch(name);
}

template <typename Profile>
UUID ThreadInMemAllocator<Profile>::root_shaphot_id() const
{
    return pimpl_->root_shaphot_id();
}


template <typename Profile>
const PairPtr& ThreadInMemAllocator<Profile>::pair() const {
    return pimpl_->pair();
}

template <typename Profile>
PairPtr& ThreadInMemAllocator<Profile>::pair() {
    return pimpl_->pair();
}

template <typename Profile>
std::vector<U16String> ThreadInMemAllocator<Profile>::branch_names()
{
    return pimpl_->branch_names();
}

template <typename Profile>
UUID ThreadInMemAllocator<Profile>::branch_head(const U16String& branch_name)
{
    return pimpl_->branch_head(branch_name);
}


}}
