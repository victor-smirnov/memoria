
// Copyright 2018 Victor Smirnov
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

#include <memoria/profiles/core_api/core_api_profile.hpp>

#include <memoria/api/store/memory_store_api.hpp>
#include <memoria/api/store/swmr_store_api.hpp>

#include <memoria/api/common/ctr_api.hpp>



#include <QList>
#include <QVariant>
#include <QVector>

namespace memoria {


using SWMRStorePtr = SharedPtr<ISWMRStore<CoreApiProfile<>>>;
using LMDBStorePtr = SharedPtr<ILMDBStore<CoreApiProfile<>>>;

class AbstractTreeItem {
protected:
    QList<AbstractTreeItem*> children_;
    AbstractTreeItem *parent_;
    bool expanded_{false};
public:
    AbstractTreeItem(AbstractTreeItem* parent = nullptr):
        parent_(parent)
    {}

    virtual ~AbstractTreeItem() noexcept;

    AbstractTreeItem *child(int number);
    int children();

    void remove_child(int child_num);

    AbstractTreeItem *parent();

    int child_number();

    virtual QVariant data(int column);
    virtual QString node_type() const = 0;

protected:
    virtual void expand() = 0;
};


class RootTreeItem: public AbstractTreeItem {
    QVector<QVariant> data_;
public:
    RootTreeItem(QVector<QVariant> data);

    virtual QVariant data(int column) const {
        return data_[column];
    }

    void add_inmem_store(IMemoryStorePtr<> store, QString label);
    void add_swmr_store(SWMRStorePtr store, QString label);
    void add_lmdb_store(LMDBStorePtr store, QString label);

    void remove_store(AbstractTreeItem* item);

    virtual QString node_type() const {
        return QString::fromUtf8("root");
    }

protected:
    virtual void expand() {
        expanded_ = true;
    }
};

struct BlockCounterProvider {
    using StoreBlockID = ApiProfileBlockID<CoreApiProfile<>>;

    virtual ~BlockCounterProvider() noexcept = default;

    virtual bool has_block_counters() const noexcept = 0;
    virtual QString get_block_counter(const StoreBlockID& block_id) = 0;
};

class MemStoreTreeItem: public AbstractTreeItem, public BlockCounterProvider  {
protected:
    IMemoryStorePtr<> allocator_;
    QString label_;
public:
    MemStoreTreeItem(IMemoryStorePtr<> allocator, const QString& label, AbstractTreeItem* parent):
        AbstractTreeItem(parent),
        allocator_(allocator),
        label_(label)
    {}

    virtual QVariant data(int column);

    virtual QString node_type() const {
        return QString::fromUtf8("Memory Store");
    }

protected:
    virtual void expand();

    virtual bool has_block_counters() const noexcept {return false;}
    virtual QString get_block_counter(const StoreBlockID& block_id) {
        return QString();
    }
};





class SWMRStoreTreeItem: public AbstractTreeItem, public BlockCounterProvider {
protected:
    SWMRStorePtr store_;
    QString label_;
public:
    SWMRStoreTreeItem(SWMRStorePtr store, const QString& label, AbstractTreeItem* parent):
        AbstractTreeItem(parent),
        store_(store),
        label_(label)
    {}

    virtual QVariant data(int column);

    virtual QString node_type() const {
        return QString::fromUtf8(store_->describe().data());
    }

protected:
    virtual void expand();

    virtual bool has_block_counters() const noexcept {return true;}
    virtual QString get_block_counter(const StoreBlockID& block_id);
};



class LMDBStoreTreeItem: public AbstractTreeItem, public BlockCounterProvider {
    using CommitPtr = typename ILMDBStore<CoreApiProfile<>>::ReadOnlyCommitPtr;

protected:
    LMDBStorePtr store_;
    CommitPtr commit_;

    QString label_;
public:
    LMDBStoreTreeItem(
            LMDBStorePtr store,
            const QString& label,
            AbstractTreeItem* parent
    ):
        AbstractTreeItem(parent),
        store_(store),
        label_(label)
    {
        commit_ = store_->open();
    }

    virtual QVariant data(int column);

    virtual QString node_type() const {
        return QString::fromUtf8(store_->describe().data());
    }

protected:
    virtual void expand();

    virtual bool has_block_counters() const noexcept {return false;}
    virtual QString get_block_counter(const StoreBlockID& block_id) {
        return QString();
    }
};


class MemStoreSnapshotTreeItem: public AbstractTreeItem {
protected:
    IMemoryStorePtr<> store_;
    UUID snapshot_id_;

    BlockCounterProvider* counter_provider_;
public:
    MemStoreSnapshotTreeItem(
            IMemoryStorePtr<> store,
            const UUID& snapshot_id,
            AbstractTreeItem* parent,
            BlockCounterProvider* counter_provider
    ):
        AbstractTreeItem(parent),
        store_(store),
        snapshot_id_(snapshot_id),
        counter_provider_(counter_provider)
    {}

    virtual QVariant data(int column);

    virtual QString node_type() const {
        return QString::fromUtf8("snapshot");
    }

protected:
    virtual void expand();
};

class SWMRStoreSnapshotTreeItem: public AbstractTreeItem {
protected:
    SWMRStorePtr store_;
    UUID snapshot_id_;
    BlockCounterProvider* counter_provider_;
public:
    SWMRStoreSnapshotTreeItem(
            SWMRStorePtr store,
            const UUID& snapshot_id,
            AbstractTreeItem* parent,
            BlockCounterProvider* counter_provider
    ):
        AbstractTreeItem(parent),
        store_(store),
        snapshot_id_(snapshot_id),
        counter_provider_(counter_provider)
    {}

    virtual QVariant data(int column);

    virtual QString node_type() const {
        return QString::fromUtf8("snapshot");
    }

protected:
    virtual void expand();
};


class MemStoreContainerTreeItem: public AbstractTreeItem {
protected:
    IMemoryStorePtr<> store_;
    UUID snapshot_id_;
    UUID ctr_id_;
    BlockCounterProvider* counter_provider_;
public:
    MemStoreContainerTreeItem(
            IMemoryStorePtr<> store,
            const UUID& snapshot_id,
            const UUID& ctr_id,
            AbstractTreeItem* parent,
            BlockCounterProvider* counter_provider
    ):
        AbstractTreeItem(parent),
        store_(store),
        snapshot_id_(snapshot_id),
        ctr_id_(ctr_id),
        counter_provider_(counter_provider)
    {}

    virtual QVariant data(int column);

    virtual QString node_type() const {
        return QString::fromUtf8("container");
    }

protected:
    virtual void expand();
};


class SWMRStoreContainerTreeItem: public AbstractTreeItem {
protected:
    SWMRStorePtr store_;
    UUID snapshot_id_;
    UUID ctr_id_;
    BlockCounterProvider* counter_provider_;
public:
    SWMRStoreContainerTreeItem(
            SWMRStorePtr store,
            const UUID& snapshot_id,
            const UUID& ctr_id,
            AbstractTreeItem* parent,
            BlockCounterProvider* counter_provider
    ):
        AbstractTreeItem(parent),
        store_(store),
        snapshot_id_(snapshot_id),
        ctr_id_(ctr_id),
        counter_provider_(counter_provider)
    {}

    virtual QVariant data(int column);

    virtual QString node_type() const {
        return QString::fromUtf8("container");
    }

protected:
    virtual void expand();
};

class LMDBStoreContainerTreeItem: public AbstractTreeItem {
protected:
    using CommitPtr = typename ILMDBStore<CoreApiProfile<>>::ReadOnlyCommitPtr;
    CommitPtr commit_;
    UUID snapshot_id_;
    UUID ctr_id_;
    BlockCounterProvider* counter_provider_;
public:
    LMDBStoreContainerTreeItem(
            CommitPtr commit,
            const UUID& ctr_id,
            AbstractTreeItem* parent,
            BlockCounterProvider* counter_provider
    ):
        AbstractTreeItem(parent),
        commit_(commit),
        ctr_id_(ctr_id),
        counter_provider_(counter_provider)
    {}

    virtual QVariant data(int column);

    virtual QString node_type() const {
        return QString::fromUtf8("container");
    }

protected:
    virtual void expand();
};


class CtrBlockTreeItem: public AbstractTreeItem {
protected:
    size_t idx_;
    CtrBlockPtr<CoreApiProfile<>> block_;
    BlockCounterProvider* counter_provider_;
public:
    CtrBlockTreeItem(
            size_t idx,
            CtrBlockPtr<CoreApiProfile<>> block,
            AbstractTreeItem* parent,
            BlockCounterProvider* counter_provider
    ):
        AbstractTreeItem(parent),
        idx_(idx),
        block_(block),
        counter_provider_(counter_provider)
    {}

    CtrBlockPtr<CoreApiProfile<>> block() const {
        return block_;
    }

    virtual QVariant data(int column);

    virtual QString node_type() const {
        return QString::fromUtf8("block");
    }

protected:
    virtual void expand();
};



}
