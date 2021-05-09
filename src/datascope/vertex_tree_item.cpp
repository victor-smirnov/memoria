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


#include "vertex_tree_item.hpp"

#include <QStringList>

namespace memoria {

AbstractTreeItem::~AbstractTreeItem() noexcept
{
    qDeleteAll(children_);
}

AbstractTreeItem *AbstractTreeItem::child(int number)
{
    expand();

    AbstractTreeItem* item = children_.value(number);
    return item;
}

int AbstractTreeItem::children()
{
    expand();
    return children_.count();
}

int AbstractTreeItem::child_number()
{
    if (parent_)
        return parent_->children_.indexOf(const_cast<AbstractTreeItem*>(this));

    return 0;
}


void AbstractTreeItem::remove_child(int child_num)
{
    if (child_num >= 0 && child_num < children_.size())
    {
        children_.removeAt(child_num);
    }
}


QVariant AbstractTreeItem::data(int)
{
    return QVariant();
}


AbstractTreeItem *AbstractTreeItem::parent()
{
    return parent_;
}


RootTreeItem::RootTreeItem(QVector<QVariant> data):
    AbstractTreeItem(),
    data_(data)
{}



void RootTreeItem::add_inmem_store(IMemoryStorePtr<> store, QString label)
{
    AbstractTreeItem* item = new MemStoreTreeItem(store, label, this);
    this->children_.append(item);
}

void RootTreeItem::add_swmr_store(SWMRStorePtr store, QString label)
{
    AbstractTreeItem* item = new SWMRStoreTreeItem(store, label, this);
    this->children_.append(item);
}

void RootTreeItem::add_lmdb_store(LMDBStorePtr store, QString label)
{
    AbstractTreeItem* item = new LMDBStoreTreeItem(store, label, this);
    this->children_.append(item);
}

void RootTreeItem::remove_store(AbstractTreeItem* item)
{
    this->children_.removeAll(item);
}








QVariant MemStoreTreeItem::data(int column) {
    return column == 0 ? node_type() : column == 1 ? label_ : QVariant();
}

void MemStoreTreeItem::expand()
{
    if (!expanded_)
    {
        children_.append(new MemStoreSnapshotTreeItem(allocator_, allocator_->root_shaphot_id(), this, this));
        expanded_ = true;
    }
}

QVariant SWMRStoreTreeItem::data(int column) {
    return column == 0 ? node_type() : column == 1 ? label_ : QVariant();
}


void SWMRStoreTreeItem::expand()
{
    if (!expanded_)
    {
        auto commits = store_->commits(false);

        for (const auto& commit_id: commits) {
            children_.append(new SWMRStoreSnapshotTreeItem(store_, commit_id, this, this));
        }

        expanded_ = true;
    }
}

QString SWMRStoreTreeItem::get_block_counter(const StoreBlockID& block_id) {
    return "(" + QString::fromStdString(std::to_string(store_->count_refs(block_id))) + ")";
}

QVariant LMDBStoreTreeItem::data(int column) {
    return column == 0 ? node_type() : column == 1 ? label_ : QVariant();
}

void LMDBStoreTreeItem::expand()
{
    if (!expanded_)
    {
        auto names = commit_->container_names();

        for (auto name: names) {
            children_.append(new LMDBStoreContainerTreeItem(commit_, name, this, this));
        }

        expanded_ = true;
    }
}



QVariant MemStoreSnapshotTreeItem::data(int column) {
    return column == 0 ? node_type() : column == 1 ? QVariant(QString::fromUtf8(toString(snapshot_id_).data())) : QVariant();
}

void MemStoreSnapshotTreeItem::expand()
{
    if (!expanded_)
    {
        auto child_snps = store_->children_of(snapshot_id_);

        for (auto snp_id: child_snps) {
            children_.append(new MemStoreSnapshotTreeItem(store_, snp_id, this, counter_provider_));
        }

        auto snp   = store_->find(snapshot_id_);
        auto names = snp->container_names();

        for (auto name: names) {
            children_.append(new MemStoreContainerTreeItem(store_, snapshot_id_, name, this, counter_provider_));
        }

        expanded_ = true;
    }
}




QVariant SWMRStoreSnapshotTreeItem::data(int column) {
    return column == 0 ? node_type() : column == 1 ? QVariant(QString::fromUtf8(toString(snapshot_id_).data())) : QVariant();
}

void SWMRStoreSnapshotTreeItem::expand()
{
    if (!expanded_)
    {
        auto snp   = store_->open(snapshot_id_, false);
        auto names = snp->container_names();

        for (auto name: names) {
            children_.append(new SWMRStoreContainerTreeItem(store_, snapshot_id_, name, this, counter_provider_));
        }

        expanded_ = true;
    }
}





QVariant MemStoreContainerTreeItem::data(int column)
{
    auto snp = store_->find(snapshot_id_);
    auto ctr = snp->find(ctr_id_);

    switch (column) {
        case 0: return node_type();
        case 1: return QString::fromUtf8(toString(ctr_id_).data());
        case 2: {
            return QString::fromUtf8(ctr->describe_datatype().data());
        }
    }

    return QVariant{};
}

void MemStoreContainerTreeItem::expand()
{
    if (!expanded_)
    {
        auto snp = store_->find(snapshot_id_);
        auto ctr = snp->find(ctr_id_);

        children_.append(new CtrBlockTreeItem(0, ctr->root_block(), this, counter_provider_));

        expanded_ = true;
    }
}


QVariant SWMRStoreContainerTreeItem::data(int column)
{
    auto snp = store_->open(snapshot_id_, false);
    auto ctr = snp->find(ctr_id_);

    switch (column) {
        case 0: return node_type();
        case 1: return QString::fromUtf8(toString(ctr_id_).data());
        case 2: {
            return QString::fromUtf8(ctr->describe_datatype().data());
        }
    }

    return QVariant{};
}

void SWMRStoreContainerTreeItem::expand()
{
    if (!expanded_)
    {
        auto snp = store_->open(snapshot_id_, false);
        auto ctr = snp->find(ctr_id_);

        children_.append(new CtrBlockTreeItem(0, ctr->root_block(), this, counter_provider_));

        expanded_ = true;
    }
}


QVariant LMDBStoreContainerTreeItem::data(int column)
{
    auto ctr = commit_->find(ctr_id_);

    switch (column) {
        case 0: return node_type();
        case 1: return QString::fromUtf8(toString(ctr_id_).data());
        case 2: {
            return QString::fromUtf8(ctr->describe_datatype().data());
        }
    }

    return QVariant{};
}

void LMDBStoreContainerTreeItem::expand()
{
    if (!expanded_)
    {
        auto ctr = commit_->find(ctr_id_);
        children_.append(new CtrBlockTreeItem(0, ctr->root_block(), this, counter_provider_));
        expanded_ = true;
    }
}



QVariant CtrBlockTreeItem::data(int column)
{
    switch (column) {
    case 0: {
        QString descr = node_type() + QString::fromUtf8((U8String(" :: ") + toString(idx_)).data());

        if (counter_provider_->has_block_counters()) {
            descr = descr + " " + counter_provider_->get_block_counter(block_->block_id());
        }

        return descr;
    }
    case 1: {
        UUID uuid;
        block_id_holder_to(block_->block_id(), uuid);
        return QString::fromUtf8(toString(uuid).data());
    }
    }

    return QVariant{};
}

void CtrBlockTreeItem::expand()
{
    if (!expanded_)
    {
        auto children = block_->children();

        size_t idx = 0;
        for (auto& child: children) {
            children_.append(new CtrBlockTreeItem(idx, child, this, counter_provider_));
            idx++;
        }

        expanded_ = true;
    }
}







}

