
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

#include <memoria/v1/core/graph/graph.hpp>
#include <memoria/v1/core/tools/uuid.hpp>

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

#include <unordered_map>

namespace memoria {
namespace v1 {

class AbstractTreeItem;
class RootTreeItem;

class AllocatorModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    AllocatorModel(const QStringList &headers, QObject *parent = nullptr);
    virtual ~AllocatorModel() override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

//    bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
//    bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;


    void createNewInMemAllocator();
private:
    AbstractTreeItem *get_item(const QModelIndex &index) const;

    RootTreeItem *root_item_;

    std::unordered_map<UUID, AbstractTreeItem*> tree_content_;
};

}}
