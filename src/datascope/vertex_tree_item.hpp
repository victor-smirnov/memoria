
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

#include <memoria/v1/api/allocator/allocator_inmem_api.hpp>

#include <QList>
#include <QVariant>
#include <QVector>

namespace memoria {
namespace v1 {

using VertexSchemaFn = std::function<QVariant(Vertex&, int)>;

VertexSchemaFn get_vertex_schema(const U16String& label);

QVariant to_variant(const VertexProperty& prop);

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

    void add_inmem_allocator(InMemAllocator<> allocator, QString label);
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
    InMemAllocator<> allocator_;
    QString label_;
public:
    InMemAllocatorTreeItem(InMemAllocator<> allocator, const QString& label, AbstractTreeItem* parent):
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

class VertexTreeItem: public AbstractTreeItem {
protected:
    Vertex vertex_;
    VertexSchemaFn schema_fn_;
public:
    VertexTreeItem(Vertex vertex, VertexSchemaFn schema_fn, AbstractTreeItem *parent);

    virtual QVariant data(int column);

    virtual QString node_type() const {
        return QString::fromUtf16(vertex_.label().data());
    }

    Vertex& vertex() {return vertex_;}
    const Vertex& vertex() const {return vertex_;}

    virtual void expand();
};






}}
