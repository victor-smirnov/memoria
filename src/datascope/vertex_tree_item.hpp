
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

#include <memoria/profiles/core_cow_api/core_cow_api_profile.hpp>
#include <memoria/api/store/memory_store_api.hpp>
#include <memoria/api/common/ctr_api.hpp>



#include <QList>
#include <QVariant>
#include <QVector>

namespace memoria {

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

    void add_inmem_allocator(IMemoryStorePtr<> allocator, QString label);
    void remove_inmem_allocator(AbstractTreeItem* item);

    virtual QString node_type() const {
        return QString::fromUtf8("root");
    }

protected:
    virtual void expand() {
        expanded_ = true;
    }
};



class InMemAllocatorTreeItem: public AbstractTreeItem {
protected:
    IMemoryStorePtr<> allocator_;
    QString label_;
public:
    InMemAllocatorTreeItem(IMemoryStorePtr<> allocator, const QString& label, AbstractTreeItem* parent):
        AbstractTreeItem(parent),
        allocator_(allocator),
        label_(label)
    {}

    virtual QVariant data(int column);

    virtual QString node_type() const {
        return QString::fromUtf8("inmem_allocator");
    }

protected:
    virtual void expand();
};


class SnapshotTreeItem: public AbstractTreeItem {
protected:
    IMemoryStorePtr<> store_;
    UUID snapshot_id_;
public:
    SnapshotTreeItem(IMemoryStorePtr<> store, const UUID& snapshot_id, AbstractTreeItem* parent):
        AbstractTreeItem(parent),
        store_(store),
        snapshot_id_(snapshot_id)
    {}

    virtual QVariant data(int column);

    virtual QString node_type() const {
        return QString::fromUtf8("snapshot");
    }

protected:
    virtual void expand();
};


class ContainerTreeItem: public AbstractTreeItem {
protected:
    IMemoryStorePtr<> store_;
    UUID snapshot_id_;
    UUID ctr_id_;
public:
    ContainerTreeItem(IMemoryStorePtr<> store, const UUID& snapshot_id, const UUID& ctr_id, AbstractTreeItem* parent):
        AbstractTreeItem(parent),
        store_(store),
        snapshot_id_(snapshot_id),
        ctr_id_(ctr_id)
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
    CtrBlockPtr<CoreCowApiProfile<>> block_;
public:
    CtrBlockTreeItem(size_t idx, CtrBlockPtr<CoreCowApiProfile<>> block, AbstractTreeItem* parent):
        AbstractTreeItem(parent),
        idx_(idx),
        block_(block)
    {}

    CtrBlockPtr<CoreCowApiProfile<>> block() const {
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
