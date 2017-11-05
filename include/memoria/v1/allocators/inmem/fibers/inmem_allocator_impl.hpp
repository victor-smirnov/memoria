
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

#include "fibers_snapshot_impl.hpp"

#include <memoria/v1/reactor/reactor.hpp>

#include <memoria/v1/fiber/detail/spinlock.hpp>
#include <memoria/v1/fiber/count_down_latch.hpp>
#include <memoria/v1/fiber/shared_mutex.hpp>
#include <memoria/v1/fiber/recursive_shared_mutex.hpp>
#include <memoria/v1/fiber/shared_lock.hpp>

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
class InMemAllocatorImpl: public InMemAllocatorBase<Profile, InMemAllocatorImpl<Profile>> { 
public:
    using Base      = InMemAllocatorBase<Profile, InMemAllocatorImpl<Profile>>;
    using MyType    = InMemAllocatorImpl<Profile>;
    
    using typename Base::Page;

    using SnapshotT             = v1::persistent_inmem::Snapshot<Profile, MyType>;
    using SnapshotPtr           = SnpSharedPtr<SnapshotT>;
    using AllocatorPtr          = AllocSharedPtr<MyType>;
    
    using MutexT                = memoria::v1::fibers::detail::spinlock;
    using LockGuardT            = memoria::v1::fibers::detail::spinlock_lock;
    
    using SnapshotMutexT        = memoria::v1::fibers::detail::spinlock;
    using SnapshotLockGuardT    = memoria::v1::fibers::detail::spinlock_lock;
        
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
    friend class Snapshot;
    

private:
        
    int32_t cpu_;
    
    MutexT mutex_;
    MutexT store_mutex_;
    
    memoria::v1::fibers::recursive_shared_mutex store_rw_mutex_;
    
    memoria::v1::fibers::count_down_latch<int64_t> active_snapshots_;
 
public:
    InMemAllocatorImpl() {
        auto snapshot = snp_make_shared_init<SnapshotT>(history_tree_, this);
        snapshot->commit();
    }
    
    virtual ~InMemAllocatorImpl()
    {
        free_memory(history_tree_);
    }

private:
    InMemAllocatorImpl(int32_t v): Base(v) {}
    
    auto ref_active() {
        return active_snapshots_.inc();
    }

    auto unref_active() {
        return active_snapshots_.dec();
    }
    
public:
    
    int64_t active_snapshots() {
        return active_snapshots_.get();
    }

    
    
    int32_t cpu() const {return cpu_;}
    
    void pack()
    {
        return reactor::engine().run_at(cpu_, [&]{
            do_pack(history_tree_);
        });
    }
    
    SnpSharedPtr<SnapshotMetadata<TxnId>> describe(const TxnId& snapshot_id) const
    {
    	return reactor::engine().run_at(cpu_, [&]{
            auto iter = snapshot_map_.find(snapshot_id);
            if (iter != snapshot_map_.end())
            {
                const auto history_node = iter->second;

                std::vector<TxnId> children;

                for (const auto& node: history_node->children())
                {
                    children.emplace_back(node->txn_id());
                }

                auto parent_id = history_node->parent() ? history_node->parent()->txn_id() : UUID();

                return snp_make_shared<SnapshotMetadata<TxnId>>(
                    parent_id, history_node->txn_id(), children, history_node->metadata(), history_node->status()
                );
            }
            else {
                throw Exception(MA_SRC, SBuf() << "Snapshot id " << snapshot_id << " is unknown");
            }
        });
    }



    SnapshotPtr find(const TxnId& snapshot_id)
    {
        return reactor::engine().run_at(cpu_, [&]
        {
            auto iter = snapshot_map_.find(snapshot_id);
            if (iter != snapshot_map_.end())
            {
                auto history_node = iter->second;

                if (history_node->is_committed())
                {
                    return snp_make_shared_init<SnapshotT>(history_node, this->shared_from_this());
                }
                if (history_node->is_data_locked())
                {
                    throw Exception(MA_SRC, SBuf() << "Snapshot " << history_node->txn_id() << " data is locked");
                }
                else {
                    throw Exception(MA_SRC, SBuf() << "Snapshot " << history_node->txn_id() << " is " << (history_node->is_active() ? "active" : "dropped"));
                }
            }
            else {
                throw Exception(MA_SRC, SBuf() << "Snapshot id " << snapshot_id << " is unknown");
            }
        });
    }




    SnapshotPtr find_branch(StringRef name)
    {
        return reactor::engine().run_at(cpu_, [&]{
            auto iter = named_branches_.find(name);
            if (iter != named_branches_.end())
            {
                auto history_node = iter->second;

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
                        throw Exception(MA_SRC, SBuf() << "Snapshot id " << history_node->txn_id() << " is locked and open");
                    }
                }
                else {
                    throw Exception(
                        MA_SRC, 
                        SBuf() << "Snapshot " << history_node->txn_id() << " is " << (history_node->is_active() ? "active" : "dropped")
                    );
                }
            }
            else {
                throw Exception(MA_SRC, SBuf() << "Named branch \"" << name << "\" is not known");
            }
        });
    }

    SnapshotPtr master()
    {
        return reactor::engine().run_at(cpu_, [&]{
            return snp_make_shared_init<SnapshotT>(master_, this->shared_from_this());
        });
    }

    SnpSharedPtr<SnapshotMetadata<TxnId>> describe_master() const
    {
        return reactor::engine().run_at(cpu_, [&]{
            std::vector<TxnId> children;

            for (const auto& node: master_->children())
            {
                children.emplace_back(node->txn_id());
            }

            auto parent_id = master_->parent() ? master_->parent()->txn_id() : UUID();

            return snp_make_shared<SnapshotMetadata<TxnId>>(
                parent_id, master_->txn_id(), children, master_->metadata(), master_->status()
            );
        });
    }

    void set_master(const TxnId& txn_id)
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
                    throw Exception(MA_SRC, SBuf() << "Snapshot " << txn_id << " has been dropped");
                }
                else {
                    throw Exception(MA_SRC, SBuf() << "Snapshot " << txn_id << " hasn't been committed yet");
                }
            }
            else {
                throw Exception(MA_SRC, SBuf() << "Snapshot " << txn_id << " is not known in this allocator");
            }
        });
    }

    void set_branch(StringRef name, const TxnId& txn_id)
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
                    throw Exception(MA_SRC, SBuf() << "Snapshot " << txn_id << " hasn't been committed yet");
                }
            }
            else {
                throw Exception(MA_SRC, SBuf()<<"Snapshot " << txn_id << " is not known in this allocator");
            }
        });
    }

    ContainerMetadataRepository* getMetadata() const
    {
        return metadata_;
    }

    virtual void walkContainers(ContainerWalker* walker, const char* allocator_descr = nullptr)
    {
        return reactor::engine().run_at(cpu_, [&]
        {
            this->build_snapshot_labels_metadata();

            walker->beginAllocator("PersistentInMemAllocator", allocator_descr);

            walk_containers(history_tree_, walker);

            walker->endAllocator();

            snapshot_labels_metadata().clear();
        
        });
    }

    

    virtual void store(OutputStreamHandler *output)
    {
        return reactor::engine().run_at(cpu_, [&]{
            
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
        });
    }


    virtual void store(filesystem::path file)
    {
    	auto fileh = FileOutputStreamHandler::create_buffered(file);
        store(fileh.get());
    }

    void dump(filesystem::path path)
    {
        using Walker = FiberFSDumpContainerWalker<Page>;

        Walker walker(this->getMetadata(), path);
        this->walkContainers(&walker);
    }
 
    static AllocSharedPtr<MyType> load(const char* file, int32_t cpu)
    {
        auto fileh = FileInputStreamHandler::create(file);
        return Base::load(fileh.get());
    }
    
    static AllocSharedPtr<MyType> load(const char* file)
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
InMemAllocator<Profile>::InMemAllocator() {}

template <typename Profile>
InMemAllocator<Profile>::InMemAllocator(AllocSharedPtr<PImpl> impl): pimpl_(impl) {}

template <typename Profile>
InMemAllocator<Profile>::InMemAllocator(InMemAllocator&& other): pimpl_(std::move(other.pimpl_)) {}

template <typename Profile>
InMemAllocator<Profile>::InMemAllocator(const InMemAllocator& other): pimpl_(other.pimpl_) {}

template <typename Profile>
InMemAllocator<Profile>& InMemAllocator<Profile>::operator=(const InMemAllocator& other) 
{
    pimpl_ = other.pimpl_;
    return *this;
}

template <typename Profile>
InMemAllocator<Profile>& InMemAllocator<Profile>::operator=(InMemAllocator&& other) 
{
    pimpl_ = std::move(other.pimpl_);
    return *this;
}

template <typename Profile>
InMemAllocator<Profile>::~InMemAllocator() {}


template <typename Profile>
bool InMemAllocator<Profile>::operator==(const InMemAllocator& other) const
{
    return pimpl_ == other.pimpl_;
}


template <typename Profile>
InMemAllocator<Profile>::operator bool() const 
{
    return pimpl_ != nullptr;
}



template <typename Profile>
InMemAllocator<Profile> InMemAllocator<Profile>::load(InputStreamHandler* input_stream) 
{
    return InMemAllocator<Profile>(PImpl::load(input_stream));
}

template <typename Profile>
InMemAllocator<Profile> InMemAllocator<Profile>::load(InputStreamHandler* input_stream, int32_t cpu) 
{
    return InMemAllocator<Profile>(PImpl::load(input_stream));
}

template <typename Profile>
InMemAllocator<Profile> InMemAllocator<Profile>::load(memoria::v1::filesystem::path file_name) 
{
    return InMemAllocator<Profile>(PImpl::load(file_name.string().c_str()));
}

template <typename Profile>
InMemAllocator<Profile> InMemAllocator<Profile>::load(memoria::v1::filesystem::path file_name, int32_t cpu) 
{
    return InMemAllocator<Profile>(PImpl::load(file_name.string().c_str()));
}

template <typename Profile>
InMemAllocator<Profile> InMemAllocator<Profile>::create() 
{
    return InMemAllocator<Profile>(PImpl::create());
}

template <typename Profile>
InMemAllocator<Profile> InMemAllocator<Profile>::create(int32_t cpu) 
{
    return InMemAllocator<Profile>(PImpl::create(cpu));
}

template <typename Profile>
void InMemAllocator<Profile>::store(filesystem::path file_name) 
{
    pimpl_->store(file_name);
}

template <typename Profile>
void InMemAllocator<Profile>::store(OutputStreamHandler* output_stream) 
{
    pimpl_->store(output_stream);
}

template <typename Profile>
typename InMemAllocator<Profile>::SnapshotPtr InMemAllocator<Profile>::master() 
{
    return SnapshotPtr(pimpl_->master());
}


template <typename Profile>
typename InMemAllocator<Profile>::SnapshotPtr InMemAllocator<Profile>::find(const TxnId& snapshot_id) 
{
    return pimpl_->find(snapshot_id);
}

template <typename Profile>
typename InMemAllocator<Profile>::SnapshotPtr InMemAllocator<Profile>::find_branch(StringRef name) 
{
    return pimpl_->find_branch(name);
}

template <typename Profile>
void InMemAllocator<Profile>::set_master(const TxnId& txn_id) 
{
    pimpl_->set_master(txn_id);
}

template <typename Profile>
void InMemAllocator<Profile>::set_branch(StringRef name, const TxnId& txn_id) 
{
    pimpl_->set_branch(name, txn_id);
}

template <typename Profile>
ContainerMetadataRepository* InMemAllocator<Profile>::metadata() const 
{
    return pimpl_->getMetadata();
}

template <typename Profile>
void InMemAllocator<Profile>::walk_containers(ContainerWalker* walker, const char* allocator_descr) 
{
     return pimpl_->walkContainers(walker, allocator_descr);
}

template <typename Profile>
void InMemAllocator<Profile>::dump(filesystem::path dump_at) 
{
    pimpl_->dump(dump_at);
}

template <typename Profile>
void InMemAllocator<Profile>::reset() 
{
    pimpl_.reset();
}


template <typename Profile>
void InMemAllocator<Profile>::pack() 
{
    return pimpl_->pack();
}

template <typename Profile>
Logger& InMemAllocator<Profile>::logger()
{
    return pimpl_->logger();
}


template <typename Profile>
bool InMemAllocator<Profile>::check() 
{
    return pimpl_->check();
}

template <typename Profile>
SnpSharedPtr<SnapshotMetadata<typename InMemAllocator<Profile>::TxnId>> InMemAllocator<Profile>::describe(const TxnId& snapshot_id) const 
{
    return pimpl_->describe(snapshot_id);
}

template <typename Profile>
SnpSharedPtr<SnapshotMetadata<typename InMemAllocator<Profile>::TxnId>> InMemAllocator<Profile>::describe_master() const {
    return pimpl_->describe_master();
}

}}