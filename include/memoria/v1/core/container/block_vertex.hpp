
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


namespace memoria {
namespace v1 {

template <typename AllocatorPtr, typename CtrInterfacePtr>
class BlockVertex: public IVertex, public EnableSharedFromThis<BlockVertex<AllocatorPtr, CtrInterfacePtr>> {

    Graph graph_;
    AllocatorPtr allocator_;
    CtrInterfacePtr ctr_interface_;
    UUID block_id_;
    UUID name_;

public:
    BlockVertex(Graph graph, AllocatorPtr allocator, CtrInterfacePtr ctr_interface, UUID block_id, UUID name):
        graph_(std::move(graph)), allocator_(std::move(allocator)),
        ctr_interface_(std::move(ctr_interface)), block_id_(block_id), name_(name)
    {
    }

    static Vertex make(Graph graph, AllocatorPtr allocator, CtrInterfacePtr ctr_interface, UUID block_id, UUID name) {
        return Vertex(MakeLocalShared<BlockVertex>(graph, allocator, ctr_interface, block_id, name));
    }

    Vertex self() {
        return Vertex(DynamicPointerCast<IVertex>(this->shared_from_this()));
    }

    Vertex self() const {
        return Vertex(DynamicPointerCast<IVertex>(ConstPointerCast<BlockVertex>(this->shared_from_this())));
    }

    virtual Graph graph()
    {
        return graph_;
    }

    virtual Any id() const {
        return block_id_;
    }

    virtual U8String label() const {
        return "block";
    }

    virtual void remove() {
        MMA1_THROW(GraphException()) << WhatCInfo("Can't remove block via Vertex::remove()");
    }

    virtual bool is_removed() const {
        return false; // FIXME: check with the allocator
    }

    virtual Collection<VertexProperty> properties()
    {
        return ctr_interface_->block_properties(self(), block_id_, name_, allocator_);
    }

    virtual Collection<Edge> edges(Direction direction) {
        return ctr_interface_->describe_block_links(block_id_, name_, allocator_, direction);
    }
};


}}
