
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

#ifndef MMA1_NO_REACTOR
#   include <memoria/v1/reactor/reactor.hpp>
#endif



namespace memoria {
namespace v1 {



class DefaultEdge: public IEdge {
    Graph graph_;
    Any id_;
    U16String label_;

    Vertex out_vertex_;
    Vertex in_vertex_;

    Collection<Property> properties_;

public:
    DefaultEdge(Graph graph, Any id, U16String label, Vertex out_vertex, Vertex in_vertex, Collection<Property> properties):
        graph_(graph), id_(std::move(id)), label_(std::move(label)),
        out_vertex_(std::move(out_vertex)), in_vertex_(std::move(in_vertex)),
        properties_(std::move(properties))
    {}

    DefaultEdge(Graph graph, U16String label, Vertex out_vertex, Vertex in_vertex):
        graph_(graph), id_(), label_(std::move(label)),
        out_vertex_(std::move(out_vertex)), in_vertex_(std::move(in_vertex)),
        properties_(EmptyCollection<Property>::make())
    {}

    static Edge make(Graph graph, Any id, U16String label, Vertex out_vertex, Vertex in_vertex, Collection<Property> properties)
    {
        return Edge(MakeLocalShared<DefaultEdge>(
            std::move(graph), std::move(id), std::move(label),
            std::move(out_vertex), std::move(in_vertex),
            std::move(properties)
        ));
    }


    static Edge make(Graph graph, U16String label, Vertex out_vertex, Vertex in_vertex)
    {
        return Edge(MakeLocalShared<DefaultEdge>(
            std::move(graph), std::move(label),
            std::move(out_vertex), std::move(in_vertex)
        ));
    }

    virtual Graph graph() {
        return graph_;
    }

    virtual Any id() const
    {
        return id_;
    }

    virtual U16String label() const {
        return label_;
    }

    virtual void remove() {
        MMA1_THROW(GraphException()) << WhatCInfo("The edge is not removable");
    }

    virtual bool is_removed() const {
        return false;
    }

    virtual Vertex in_vertex()
    {
        return in_vertex_;
    }

    virtual Vertex out_vertex() {
        return out_vertex_;
    }

    virtual Collection<Property> properties()
    {
        return properties_;
    }

    virtual Property property(const U16String& name)
    {
        return empty_property();
    }

    virtual Property property(const U16String& name, const Any& value)
    {
        MMA1_THROW(GraphException()) << WhatCInfo("Can't set property value");
    }
};




}}
