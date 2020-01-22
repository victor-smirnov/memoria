
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

#include <memoria/core/strings/string.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/memory/smart_ptrs.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/tools/pimpl_base.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/core/graph/graph_commons.hpp>
#include <memoria/core/graph/graph_property.hpp>
#include <memoria/core/graph/graph_collection.hpp>
#include <memoria/core/graph/graph_element.hpp>


namespace memoria {

struct IVertex: IElement {
    virtual Collection<VertexProperty> properties() = 0;

    virtual Collection<Edge> edges(Direction direction = Direction::BOTH) = 0;
    virtual Collection<Edge> edges(Direction direction, const LabelList& labels);

    virtual Collection<Vertex> vertices(Direction direction = Direction::BOTH);
    virtual Collection<Vertex> vertices(Direction direction, const LabelList& edge_labels);

    virtual VertexProperty property(const U8String& name);
    virtual VertexProperty property(const U8String& name, const Any& value);
};


class Vertex: public PimplBase<IVertex> {
    using Base = PimplBase<IVertex>;
public:
    MMA_PIMPL_DECLARE_DEFAULT_FUNCTIONS(Vertex)

    Graph graph();

    Any id() const {
        return this->ptr_->id();
    }

    U8String label() const {
        return this->ptr_->label();
    }

    void remove() {
        return this->ptr_->remove();
    }

    bool is_removed() const {
        return this->ptr_->is_removed();
    }

    Collection<VertexProperty> properties() {
        return this->ptr_->properties();
    }

    Collection<Edge> edges(Direction direction = Direction::BOTH)
    {
        return this->ptr_->edges(direction);
    }

    Collection<Edge> edges(Direction direction, const LabelList& labels) {
        return this->ptr_->edges(direction, labels);
    }

    Collection<Vertex> vertices(Direction direction = Direction::BOTH) {
        return this->ptr_->vertices(direction);
    }

    Collection<Vertex> vertices(Direction direction, const LabelList& edge_labels)
    {
        return this->ptr_->vertices(direction, edge_labels);
    }

    Element as_element() const {
        return Element(StaticPointerCast<IElement>(this->ptr_));
    }

    VertexProperty property(const U8String& name) {
        return this->ptr_->property(name);
    }

    VertexProperty property(const U8String& name, const Any& value) {
        return this->ptr_->property(name, value);
    }
};

}
