
// Copyright 2013 Victor Smirnov
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


#include <memoria/v1/containers/vector/vctr_names.hpp>

#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>



namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(mvector::CtrApiCommonName)

public:
    using Types = typename Base::Types;

protected:
    using typename Base::NodeBaseG;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;
    using typename Base::CtrSizeT;
    using typename Base::IteratorPtr;
    using typename Base::Profile;

    using Value = typename Types::Value;
    using ValueDataType = typename Types::ValueDataType;
    using ViewType  = DTTViewType<ValueDataType>;

public:

    using typename Base::BranchNodeExtData;
    using typename Base::LeafNodeExtData;
    using typename Base::ContainerTypeName;

    void configure_types(
        const ContainerTypeName& type_name,
        BranchNodeExtData& branch_node_ext_data,
        LeafNodeExtData& leaf_node_ext_data
    ) {

    }

    CtrSizeT size() const {
        return self().sizes()[0];
    }

    CtrSharedPtr<VectorIterator<ValueDataType, Profile>> seek(CtrSizeT pos) const
    {
        typename Types::template SkipForwardWalker<Types, IntList<0>> walker(pos);
        return self().ctr_find(walker);
    }


    Datum<Value> get(CtrSizeT pos) const
    {
        auto ii = self().seek(pos);
        return ii->value();
    }

    void set(CtrSizeT pos, ViewType view)
    {
        auto ii = self().seek(pos);
        ii->set(view);
    }

    void prepend(io::IOVectorProducer& producer)
    {
        self().ctr_begin()->insert_iovector(producer, 0, std::numeric_limits<CtrSizeT>::max());
    }

    void append(io::IOVectorProducer& producer)
    {
        self().ctr_end()->insert_iovector(producer, 0, std::numeric_limits<CtrSizeT>::max());
    }

    void insert(CtrSizeT at, io::IOVectorProducer& producer)
    {
        auto ii = self().ctr_seek(at);
        ii->insert_iovector(producer, 0, std::numeric_limits<CtrSizeT>::max());
    }

    CtrSizeT remove(CtrSizeT from, CtrSizeT to)
    {
        auto& self = this->self();

        auto ii_from = self.ctr_seek(from);
        return ii_from->remove_from(to - from);
    }

    CtrSizeT remove_from(CtrSizeT from)
    {
        return remove(from, size());
    }

    CtrSizeT remove_up_to(CtrSizeT pos)
    {
        auto& self = this->self();

        auto ii_from = self.ctr_begin();
        return ii_from->remove_from(pos);
    }


MEMORIA_V1_CONTAINER_PART_END

}}
