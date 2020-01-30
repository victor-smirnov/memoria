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



void RootTreeItem::add_inmem_allocator(IMemoryStorePtr<> allocator, QString label)
{
    AbstractTreeItem* item = new InMemAllocatorTreeItem(allocator, label, this);
    this->children_.append(item);
}

void RootTreeItem::remove_inmem_allocator(AbstractTreeItem* item)
{
    this->children_.removeAll(item);
}








QVariant InMemAllocatorTreeItem::data(int column) {
    return column == 0 ? node_type() : column == 1 ? label_ : QVariant();
}

void InMemAllocatorTreeItem::expand()
{
    if (!expanded_)
    {
        children_.append(new SnapshotTreeItem(allocator_, allocator_->root_shaphot_id(), this));
        expanded_ = true;
    }
}


QVariant SnapshotTreeItem::data(int column) {
    return column == 0 ? node_type() : column == 1 ? QVariant(QString::fromUtf8(toString(snapshot_id_).data())) : QVariant();
}

void SnapshotTreeItem::expand()
{
    if (!expanded_)
    {
        auto child_snps = store_->children_of(snapshot_id_).get_or_throw();

        for (auto snp_id: child_snps) {
            children_.append(new SnapshotTreeItem(store_, snp_id, this));
        }

        auto snp   = store_->find(snapshot_id_).get_or_throw();
        auto names = snp->container_names().get_or_throw();

        for (auto name: names) {
            children_.append(new ContainerTreeItem(store_, snapshot_id_, name, this));
        }

        expanded_ = true;
    }
}


QVariant ContainerTreeItem::data(int column)
{
    auto snp = store_->find(snapshot_id_).get_or_throw();
    auto ctr = snp->find(ctr_id_).get_or_throw();

    switch (column) {
        case 0: return node_type();
        case 1: return QString::fromUtf8(toString(ctr_id_).data());
        case 2: {
            return QString::fromUtf8(ctr->describe_datatype().data());
        }
    }

    return QVariant{};
}

void ContainerTreeItem::expand()
{
    if (!expanded_)
    {
        auto snp = store_->find(snapshot_id_).get_or_throw();
        auto ctr = snp->find(ctr_id_).get_or_throw();

        children_.append(new CtrBlockTreeItem(0, ctr->root_block().get_or_throw(), this));

        expanded_ = true;
    }
}


QVariant CtrBlockTreeItem::data(int column)
{
    switch (column) {
        case 0: return node_type() + QString::fromUtf8((U8String(" :: ") + toString(idx_)).data());
        case 1: return QString::fromUtf8(toString(block_->block_id()).data());
    }

    return QVariant{};
}

void CtrBlockTreeItem::expand()
{
    if (!expanded_)
    {
        auto children = block_->children().get_or_throw();

        size_t idx = 0;
        for (auto child: children) {
            children_.append(new CtrBlockTreeItem(idx, child, this));
            idx++;
        }

        expanded_ = true;
    }
}







}

