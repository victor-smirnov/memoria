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
namespace v1 {



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




VertexTreeItem::VertexTreeItem(Vertex vertex, VertexSchemaFn schema_fn, AbstractTreeItem *parent):
    AbstractTreeItem (parent),
    vertex_(vertex),
    schema_fn_(std::move(schema_fn))
{}


QVariant VertexTreeItem::data(int column)
{
    return schema_fn_(vertex_, column);
}

void VertexTreeItem::expand()
{
    if (!expanded_)
    {
        for (Edge& ee: vertex_.edges(Direction::EDGE_OUT))
        {
            Vertex vx = ee.in_vertex();
            children_.append(new VertexTreeItem(vx, get_vertex_schema(vx.label()), this));
        }

        expanded_ = true;
    }
}



QVariant InMemAllocatorTreeItem::data(int column) {
    return column == 0 ? node_type() : column == 1 ? label_ : QVariant();
}

void InMemAllocatorTreeItem::expand()
{
    if (!expanded_)
    {
        Graph graph = allocator_->as_graph();
        for (Vertex& root: graph.roots())
        {
            children_.append(new VertexTreeItem(root, get_vertex_schema(root.label()), this));
        }

        expanded_ = true;
    }
}


QVariant to_variant(const VertexProperty& prop)
{
    if (!prop.is_empty())
    {
        Any value = prop.value();
        if (value.type() == typeid (const char*))
        {
            return QString::fromUtf8(boost::any_cast<const char*>(value));
        }
        else if (value.type() == typeid (const char16_t*))
        {
            return QString::fromUtf16(boost::any_cast<const char16_t*>(value));
        }
        else if (value.type() == typeid (U16String))
        {
            return QString::fromUtf16(boost::any_cast<U16String>(value).data());
        }
        else if (value.type() == typeid (U8String))
        {
            return QString::fromUtf8(boost::any_cast<U8String>(value).data());
        }
        else if (value.type() == typeid (UUID))
        {
            return QString::fromUtf8(toString(boost::any_cast<UUID>(value)).data());
        }
        else {
            return QString::fromUtf8("Unknown vertex property value type");
        }
    }

    return QVariant();
}



VertexSchemaFn get_vertex_schema(const U8String& label)
{
    return [=](Vertex& vx, int column) -> QVariant {
        switch (column) {
            case 0: return QString::fromUtf8(vx.label().data());
            case 1: return QString::fromUtf8(toString(boost::any_cast<UUID>(vx.id())).data());
            case 2: {
                if (label == "snapshot") {
                    return to_variant(vx.property("metadata"));
                }
                else if (label == "container") {
                    return to_variant(vx.property("type"));
                }
            }
        }

        return QVariant();
    };
}


}}

