#include <memoria/v1/core/graph/graph.hpp>
#include <memoria/v1/reactor/reactor.hpp>


namespace memoria {
namespace v1 {

Property empty_property()
{
    thread_local Property property(StaticPointerCast<IProperty>(MakeLocalShared<EmptyProperty>()));
    return property;
}

VertexProperty empty_vertex_property()
{
    thread_local VertexProperty property(StaticPointerCast<IVertexProperty>(MakeLocalShared<EmptyVertexProperty>()));
    return property;
}

Collection<Edge> IVertex::edges(Direction direction, const LabelList& labels)
{
    std::vector<Edge> filtered_edges;

    for (Edge& ee: this->edges(direction))
    {
        if (contains(labels, ee.label())) {
            filtered_edges.push_back(ee);
        }
    }

    return STLCollection<Edge>::make(std::move(filtered_edges));
}

Collection<Vertex> IVertex::vertices(Direction direction)
{
    std::vector<Vertex> vxx;

    if (direction == Direction::EDGE_OUT)
    {
        for (Edge& ee: this->edges(direction))
        {
            vxx.push_back(ee.out_vertex());
        }
    }
    else if (direction == Direction::EDGE_IN)
    {
        for (Edge& ee: this->edges(direction))
        {
            vxx.push_back(ee.in_vertex());
        }
    }
    else {
        for (Edge& ee: this->edges(direction))
        {
            vxx.push_back(ee.out_vertex());
            vxx.push_back(ee.in_vertex());
        }
    }

    return STLCollection<Vertex>::make(std::move(vxx));
}

Collection<Vertex> IVertex::vertices(Direction direction, const LabelList& edge_labels)
{
    std::vector<Vertex> vxx;

    if (direction == Direction::EDGE_OUT)
    {
        for (Edge& ee: this->edges(direction, edge_labels))
        {
            vxx.push_back(ee.out_vertex());
        }
    }
    else if (direction == Direction::EDGE_IN)
    {
        for (Edge& ee: this->edges(direction, edge_labels))
        {
            vxx.push_back(ee.in_vertex());
        }
    }
    else {
        for (Edge& ee: this->edges(direction, edge_labels))
        {
            vxx.push_back(ee.out_vertex());
            vxx.push_back(ee.in_vertex());
        }
    }

    return STLCollection<Vertex>::make(std::move(vxx));
}

VertexProperty IVertex::property(const U16String& name)
{
    for (VertexProperty& prop: this->properties())
    {
        if (prop.key() == name) {
            return prop;
        }
    }

    return empty_vertex_property();
}

VertexProperty IVertex::property(const U16String& name, const Any& value)
{
    MMA1_THROW(GraphException()) << WhatCInfo("Property update is not supported");
}



bool contains(const LabelList& list, const U16String& label)
{
    for (auto& item: list) {
        if (item == label) {
            return true;
        }
    }

    return false;
}

}}
