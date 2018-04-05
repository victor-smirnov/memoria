
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


#include <QtWidgets>

#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/fixed_array.hpp>
#include <memoria/v1/api/set/set_api.hpp>

#include "vertex_tree_item.hpp"
#include "allocator_model.hpp"

namespace memoria {
namespace v1 {

AllocatorModel::AllocatorModel(const QStringList &headers, QObject *parent)
    : QAbstractItemModel(parent)
{
    QVector<QVariant> rootData;
    foreach (QString header, headers)
        rootData << header;

    root_item_ = new RootTreeItem(rootData);
}

AllocatorModel::~AllocatorModel()
{
    for (auto& entry: tree_content_)
    {
        delete entry.second;
    }

    delete root_item_;
}

int AllocatorModel::columnCount(const QModelIndex & /* parent */) const
{
    return 3;
}

QVariant AllocatorModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    AbstractTreeItem *item = get_item(index);

    return item->data(index.column());
}

Qt::ItemFlags AllocatorModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemFlag::NoItemFlags;

    return QAbstractItemModel::flags(index);
}

AbstractTreeItem *AllocatorModel::get_item(const QModelIndex &index) const
{
    if (index.isValid())
    {
        AbstractTreeItem *item = static_cast<AbstractTreeItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return root_item_;
}

QVariant AllocatorModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return root_item_->data(section);

    return QVariant();
}

QModelIndex AllocatorModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    AbstractTreeItem *parentItem = get_item(parent);

    AbstractTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}



QModelIndex AllocatorModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    AbstractTreeItem *childItem = get_item(index);
    AbstractTreeItem *parentItem = childItem->parent();

    if (parentItem == root_item_)
        return QModelIndex();

    return createIndex(parentItem->child_number(), 0, parentItem);
}


int AllocatorModel::rowCount(const QModelIndex &parent) const
{
    AbstractTreeItem *parentItem = get_item(parent);

    int children = parentItem->children();
    return children;
}



void AllocatorModel::createNewInMemAllocator()
{
    QModelIndex root_idx = this->createIndex(0, 0, root_item_);

    auto row_pos = root_item_->children();

    InMemAllocator<> alloc = InMemAllocator<>::create();

    auto bb = alloc.master().branch();

    auto ctr = create<Set<FixedArray<16>>>(bb);

    FixedArray<16> array;

    for (int c = 0; c < 10000; c++) {
        for (int d = 0; d < 16; d++) {
            array[d] = getRandomG(255);
        }

        ctr.insert(array);
    }

    bb.set_snapshot_metadata(u"My cool snapshot");

    bb.commit();
    bb.set_as_master();

    beginInsertRows(root_idx, row_pos, row_pos);
    root_item_->add_inmem_allocator(alloc, QString::fromUtf8("allocator"));
    endInsertRows();

    emit layoutChanged();
}


}}
