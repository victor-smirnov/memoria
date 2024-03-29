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
        auto branch_names = history_->branch_names();

        for (const auto& name: branch_names) {
            children_.append(new SWMRStoreBranchTreeItem(store_, history_, name, this, this));
        }

        expanded_ = true;
    }
}

QString SWMRStoreTreeItem::get_block_counter(const AnyID& block_id) {
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


QVariant SWMRStoreBranchTreeItem::data(int column) {
    return column == 0 ? node_type() : column == 1 ? QVariant(QString::fromUtf8(branch_name_.data())) : QVariant();
}

void SWMRStoreBranchTreeItem::expand()
{
    if (!expanded_)
    {
        auto snapshots = history_->snapshots(branch_name_).get();

        for (const auto& snapshot_id: snapshots) {
            children_.append(new SWMRStoreSnapshotTreeItem(store_, snapshot_id, this, counter_provider_));
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
        auto snp   = store_->open(snapshot_id_, true);
        auto names = snp->container_names();

        UID256 sp_names[] = {
            UID256::parse("{1|942dcb76868c1d3cada1947784ca9146995aa486d7965fddc153a57846a6cf}"),
            UID256::parse("{1|4b9352517f5be15025dcd68b7d490ed14a55d1675218d872b489a6c5a4c82f}"),
            UID256::parse("{1|cd18ea71da4c4ff035824a9fabcf3cd9f028eeacb6226b765411158ad17f55}"),
            UID256::parse("{1|30e0ee4a7012fde4fc983c6d4d8aeb95bf63183fa51816298b582da10c0803}"),
        };

        for (auto name: sp_names){
            if (snp->find(name)) {
                children_.append(new SWMRStoreContainerTreeItem(store_, snapshot_id_, name, this, counter_provider_));
            }
        }

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
    auto snp = store_->open(snapshot_id_, true);
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
        auto snp = store_->open(snapshot_id_, true);
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
        return QString::fromUtf8(block_->block_id().to_u8().data());
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

