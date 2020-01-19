
// Copyright 2015 Victor Smirnov
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

#include <memoria/core/tools/cow_tree/cow_tree_tools.hpp>
#include <memoria/core/types.hpp>

#include <memoria/core/tools/optional.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <deque>
#include <mutex>
#include <iostream>

/**
 * MRSW Copy-on-write B+Tree. Transaction Log.
 */

namespace memoria {
namespace cow {
namespace tree {

class CoWTreeException: std::exception {
    const char* msg_;
public:
    CoWTreeException(const char* msg): msg_(msg) {}

    virtual const char* what() const noexcept {
        return msg_;
    }
};

template <typename NodeBase, typename Tree>
class TxnData {
    NodeBase* root_ = nullptr;

    int32_t refs_ = 0;

    Tree* tree_;

    bool commited_ = false;

public:
    using NodeBaseT = NodeBase;

    TxnData(NodeBase* root, Tree* tree): root_(root), tree_(tree) {}

    NodeBase* root() {
        return root_;
    }

    const NodeBase* root() const {
        return root_;
    }

    void set_root(NodeBase* root) {
        root_ = root;
    }

    int64_t txn_id() const {
        return root_->txn_id();
    }

    void commit() {
        if (!commited_) {
            tree_->commit_txn(this);
            commited_ = true;
        }
    }

    void rollback() {
        if (!commited_) {
            tree_->rollback_txn(this);
        }
    }

    void ref() {
        refs_++;
    }

    int32_t unref() {
        return --refs_;
    }
};


template <typename NodeBase, typename MutexT>
class CoWTreeTxnLogEntry {
    NodeBase* root_;
    int64_t txn_id_;

    MutexT* mutex_;

    int32_t refs_ = 0;

    bool locked_ = false;


    uint64_t md5_sum_;

public:
    using NodeBaseT = NodeBase;

    CoWTreeTxnLogEntry(NodeBase* root, int64_t txn_id, MutexT* lock, uint64_t hash):
        root_(root), txn_id_(txn_id), mutex_(lock), md5_sum_(hash)
    {}

    const uint64_t& md5_sum() const {
        return md5_sum_;
    }

    bool locked() const {
        return locked_;
    }

    void lock() {
        locked_ = true;
    }

    NodeBase* root() {
        return root_;
    }

    const NodeBase* root() const {
        return root_;
    }

    int64_t txn_id() const {
        return txn_id_;
    }

    int32_t refs() const {
        return refs_;
    }

    void ref()
    {
        std::lock_guard<MutexT> lock(*mutex_);
        refs_++;
    }

    int32_t unref()
    {
        std::lock_guard<MutexT> lock(*mutex_);
        refs_--;
        return refs_;
    }
};






template <typename NodeBase>
class TxnLog {
public:
    using MutexT        = std::recursive_mutex;
    using LockT         = std::lock_guard<MutexT>;

    using LogEntryT     = CoWTreeTxnLogEntry<NodeBase, MutexT>;

    using SnapshotT     = Snapshot<LogEntryT>;

    template <typename, typename> friend class CoWTree;

protected:
    MutexT mutex_;

    std::deque<LogEntryT*> events_;

public:

    TxnLog() {}

    MutexT& mutex() {
        return mutex_;
    }

    auto size() {
        LockT lock(mutex_);
        return events_.size();
    }

    SnapshotT snapshot()
    {
        LockT lock(mutex_);

        return SnapshotT(events_.front());
    }

    SnapshotT snapshot(int64_t txn_id)
    {
        LockT lock(mutex_);

        LogEntryT* entry = nullptr;

        for (LogEntryT& e: events_)
        {
            if (e.txn_id() == txn_id && !e.locked())
            {
                entry = &e;
            }
        }

        if (entry) {
            return SnapshotT(entry);
        }
        else {
            throw new CoWTreeException("Snapshot is already deleted");
        }
    }

    void dump(std::ostream& out = std::cout)
    {
        LockT lock(mutex_);

        out<<"TxnLog of "<<events_.size()<<" entries."<<endl;

        int32_t c = 0;
        for (const auto* entry: events_)
        {
            out<<(c++)
               <<": TxnId: "<<entry->txn_id()
               <<", NodeId: "<<entry->root()->node_id()
               <<", Hash: "<<hex<<entry->md5_sum()<<dec
               <<", size: "<<entry->root()->metadata().size()
               <<endl;
        }
    }
protected:

    void new_entry(NodeBase* root, uint64_t hash)
    {
        LockT lock(mutex_);
        LogEntryT* entry = new LogEntryT(root, root->txn_id(), &mutex_, hash);
        events_.push_front(entry);
    }
};


}}}
