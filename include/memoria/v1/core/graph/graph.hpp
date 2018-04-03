
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
#include <memoria/v1/core/graph/graph_edge.hpp>
#include <memoria/v1/core/graph/graph_vertex.hpp>



namespace memoria {
namespace v1 {


struct IGraph {
    virtual ~IGraph() noexcept {}

    virtual Collection<Vertex> vertices() = 0;
    virtual Collection<Vertex> vertices(const IDList& ids) = 0;

    virtual Collection<Vertex> roots() = 0;
    virtual Collection<Vertex> roots(const LabelList& vx_labels) = 0;

    virtual Collection<Edge> edges() = 0;
    virtual Collection<Edge> edges(const IDList& ids) = 0;
};


class Graph: public PimplBase<IGraph> {
    using Base = PimplBase<IGraph>;
public:
    MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS(Graph)

    Collection<Vertex> vertices() {
        return this->ptr_->vertices();
    }

    Collection<Vertex> vertices(const IDList& ids)
    {
        return this->ptr_->vertices(ids);
    }

    Collection<Vertex> roots() {
        return this->ptr_->roots();
    }

    Collection<Vertex> roots(const LabelList& vx_labels) {
        return this->ptr_->roots(vx_labels);
    }

    Collection<Edge> edges() {
        return this->ptr_->edges();
    }

    Collection<Edge> edges(const IDList& ids) {
        return this->ptr_->edges(ids);
    }
};

inline Graph Vertex::graph() const {
    return this->ptr_->graph();
}


inline Vertex Edge::in_vertex() const {
    return this->ptr_->in_vertex();
}

inline Vertex Edge::out_vertex() const {
    return this->ptr_->out_vertex();
}


inline Graph Edge::graph() const {
    return this->ptr_->graph();
}



struct CollectionTools {
    CollectionTools() = delete;

    template <typename SrcC, typename TgtC, typename Fn>
    static void map(const SrcC& src, TgtC& tgt, Fn&& fn)
    {
        for (auto& item: src)
        {
            tgt.emplace_back(fn(item));
        }
    }

    template <typename SrcC, typename Fn>
    static SrcC map(const SrcC& src, Fn&& fn)
    {
        SrcC tgt;

        for (auto& item: src)
        {
            tgt.emplace_back(fn(item));
        }

        return tgt;
    }
};


}}
