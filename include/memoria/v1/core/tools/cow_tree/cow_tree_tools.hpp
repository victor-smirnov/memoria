
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/core/tools/optional.hpp>
#include <memoria/v1/core/tools/cow_tree/cow_tree_node.hpp>

#include <memoria/v1/core/tools/static_array.hpp>

#include <vector>
#include <mutex>
#include <type_traits>

/**
 * MRSW Copy-on-write B+Tree. Shared Resource Handle.
 */

namespace memoria {
namespace v1 {
namespace cow       {
namespace tree      {

template <typename Handle>
class LocalRefCnt {
    Int refs_ = 0;
    Handle* handle_;

public:
    LocalRefCnt(Handle* handle): handle_(handle)
    {
        handle_->ref();
    }

    void ref() {
        refs_++;
    }

    Int unref()
    {
        if (--refs_ == 0)
        {
            handle_->unref();
        }

        return refs_;
    }

    Handle* get() {
        return handle_;
    }

    const Handle* get() const {
        return handle_;
    }
};

template <typename LogEntry>
class Snapshot {
    using NodeBaseT = typename LogEntry::NodeBaseT;

    using LocalRefCntHandle = LocalRefCnt<LogEntry>;

    LocalRefCntHandle* entry_;

    template <typename, typename> friend class CoWTree;

public:
    using MyType = Snapshot<LogEntry>;

    Snapshot(): entry_(nullptr) {}

    Snapshot(const MyType& other): entry_(other.entry_)
    {
        ref();
    }

    Snapshot(MyType&& other): entry_(other.entry_){
        other.entry_ = nullptr;
    }


    Snapshot(LogEntry* entry): entry_(new LocalRefCntHandle(entry))
    {
        ref();
    }

    ~Snapshot() {
        unref();
    }


    BigInt txn_id() const  {
        return get()->txn_id();
    }

    NodeBaseT* root() {
        return this->get()->root();
    }

    const NodeBaseT* root() const {
        return this->get()->root();
    }

    void free() {
        unref();
        entry_ = nullptr;
    }

    MyType& operator=(const MyType& other)
    {
        unref();
        entry_ = other.entry_;
        if (entry_) entry_->ref();
    }

    MyType& operator=(MyType&& other)
    {
        unref();
        entry_ = other.entry_;
        other.entry_ = nullptr;
    }

    bool operator==(const MyType& other) const
    {
        return (entry_ == nullptr && other.entry_ == nullptr)  ||
               (entry_ != nullptr && other.entry_ != nullptr && entry_ == other.entry_);
    }

    bool operator!=(const MyType& other) const
    {
        return entry_ != nullptr && other.entry_ != nullptr && entry_ != other.entry_;
    }

    MyType new_thread()
    {
        return Snapshot(*get());
    }

private:

    LogEntry* get() {
        return entry_->get();
    }

    const LogEntry* get() const {
        return entry_->get();
    }


    void unref()
    {
        if (entry_ &&  entry_->unref() == 0){
            delete entry_;
            entry_ = nullptr;
        }
    }

    void ref() {
        if (entry_) {
            entry_->ref();
        }
    }
};


template <typename TxnData>
class Transaction {

public:
    using NodeBaseT = typename TxnData::NodeBaseT;

    TxnData* txn_data_;

    template <typename, typename> friend class CoWTree;

public:
    using MyType = Transaction<TxnData>;

    Transaction(): txn_data_(nullptr) {}

    Transaction(const MyType& other): txn_data_(other.txn_data_) {
        ref();
    }

    Transaction(MyType&& other): txn_data_(other.txn_data_) {
        other.txn_data_ = nullptr;
    }

    Transaction(TxnData* data): txn_data_(data){
        ref();
    }

    ~Transaction()  {
        unref();
    }



    void commit() {
        this->get()->commit();
    }
    void rollback() {
        this->get()->rollback();
        delete txn_data_;
        txn_data_ = nullptr;
    }

    BigInt txn_id() const  {
        return root()->txn_id();
    }

    NodeBaseT* root() {
        return this->get()->root();
    }

    const NodeBaseT* root() const {
        return this->get()->root();
    }

    MyType& operator=(const MyType& other)
    {
        unref();
        txn_data_ = other.txn_data_;
        ref();
    }

    MyType& operator=(MyType&& other)
    {
        unref();
        txn_data_ = other.txn_data_;
        other.txn_data_ = nullptr;
    }

    bool operator==(const MyType& other) const
    {
        return (txn_data_ == nullptr && other.txn_data_ == nullptr)  ||
               (txn_data_ != nullptr && other.txn_data_ != nullptr && txn_data_ == other.txn_data_);
    }

    bool operator!=(const MyType& other) const
    {
        return txn_data_ != nullptr && other.txn_data_ != nullptr && txn_data_ != other.txn_data_;
    }

private:

    TxnData* get() {
        return txn_data_;
    }

    const TxnData* get() const {
        return txn_data_;
    }

    void unref()
    {
        if (txn_data_ &&  txn_data_->unref() == 0) {
            rollback();
        }
    }

    void ref() {
        if (txn_data_) {
            txn_data_->ref();
        }
    }
};


}
}
}}