
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

#include <memoria/v1/core/strings/string.hpp>
#include <memoria/v1/core/strings/format.hpp>
#include <memoria/v1/core/memory/smart_ptrs.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/tools/pimpl_base.hpp>
#include <memoria/v1/core/tools/ptr_cast.hpp>

#include <memoria/v1/core/graph/graph_commons.hpp>
#include <memoria/v1/core/graph/graph_property.hpp>
#include <memoria/v1/core/graph/graph_collection.hpp>
#include <memoria/v1/core/graph/graph_element.hpp>


namespace memoria {
namespace v1 {

struct IVertex: IElement {
    virtual Collection<VertexProperty> properties() const = 0;

    virtual Collection<Edge> edges(Direction direction = Direction::BOTH) const = 0;
    virtual Collection<Edge> edges(Direction direction, const LabelList& labels) const;

    virtual Collection<Vertex> vertices(Direction direction = Direction::BOTH) const;
    virtual Collection<Vertex> vertices(Direction direction, const LabelList& edge_labels) const;

    virtual VertexProperty property(const U16String& name) const;
    virtual VertexProperty property(const U16String& name, const Any& value) const;
};


class Vertex: public PimplBase<IVertex> {
    using Base = PimplBase<IVertex>;
public:
    MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS(Vertex)

    Graph graph() const;

    Any id() const {
        return this->ptr_->id();
    }

    U16String label() const {
        return this->ptr_->label();
    }

    void remove() {
        return this->ptr_->remove();
    }

    bool is_removed() const {
        return this->ptr_->is_removed();
    }

    Collection<VertexProperty> properties() const {
        return this->ptr_->properties();
    }

    Collection<Edge> edges(Direction direction = Direction::BOTH) const
    {
        return this->ptr_->edges(direction);
    }

    Collection<Edge> edges(Direction direction, const LabelList& labels) const {
        return this->ptr_->edges(direction, labels);
    }

    Collection<Vertex> vertices(Direction direction = Direction::BOTH) const {
        return this->ptr_->vertices(direction);
    }

    Collection<Vertex> vertices(Direction direction, const LabelList& edge_labels) const
    {
        return this->ptr_->vertices(direction, edge_labels);
    }

    Element as_element() const {
        return Element(StaticPointerCast<IElement>(this->ptr_));
    }

    VertexProperty property(const U16String& name) const {
        return this->ptr_->property(name);
    }

    VertexProperty property(const U16String& name, const Any& value) const {
        return this->ptr_->property(name, value);
    }
};


}}
