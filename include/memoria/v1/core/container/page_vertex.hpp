
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
#include <memoria/v1/metadata/metadata.hpp>
#include <memoria/v1/core/tools/uuid.hpp>


namespace memoria {
namespace v1 {

template <typename AllocatorPtr, typename CtrInterfacePtr>
class PageVertex: public IVertex {

    Graph graph_;
    AllocatorPtr allocator_;
    CtrInterfacePtr ctr_interface_;
    UUID page_id_;
    UUID name_;

public:
    PageVertex(Graph graph, AllocatorPtr allocator, CtrInterfacePtr ctr_interface, UUID page_id, UUID name):
        graph_(std::move(graph)), allocator_(std::move(allocator)),
        ctr_interface_(std::move(ctr_interface)), page_id_(page_id), name_(name)
    {
    }

    static Vertex make(Graph graph, AllocatorPtr allocator, CtrInterfacePtr ctr_interface, UUID page_id, UUID name) {
        return Vertex(MakeLocalShared<PageVertex>(graph, allocator, ctr_interface, page_id, name));
    }

    virtual Graph graph() const
    {
        return graph_;
    }

    virtual Any id() const {
        return page_id_;
    }

    virtual U16String label() const {
        return u"page";
    }

    virtual void remove() {
        MMA1_THROW(GraphException()) << WhatCInfo("Can't remove page via Vertex::remove()");
    }

    virtual bool is_removed() const {
        return false; // FIXME: check with the allocator
    }

    virtual Collection<VertexProperty> properties() const
    {
        return EmptyCollection<VertexProperty>::make();
    }

    virtual Collection<Edge> edges(Direction direction = Direction::BOTH) const {
        return EmptyCollection<Edge>::make();
    }
};


}}
