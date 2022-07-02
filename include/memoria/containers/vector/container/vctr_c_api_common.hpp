
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


#include <memoria/containers/vector/vctr_names.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/api/vector/vector_api.hpp>


namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(mvector::CtrApiCommonName)

public:
    using Types = typename Base::Types;


    using typename Base::TreeNodePtr;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::CtrSizeT;
    using typename Base::Profile;
    using typename Base::ApiProfileT;

protected:
    using Value = typename Types::Value;
    using ValueDataType = typename Types::ValueDataType;
    using ViewType  = DTTViewType<ValueDataType>;
    using CtrApiTypes = ICtrApiTypes<typename Types::ContainerTypeName, Profile>;

    using BufferT = DataTypeBuffer<ValueDataType>;

    using CtrInputBuffer = typename Types::CtrInputBuffer;

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

    virtual void read_to(BufferT& buffer, CtrSizeT start, CtrSizeT length) const
    {

    }

    virtual void insert(CtrSizeT at, const BufferT& buffer, size_t start, size_t size)
    {

    }



    ProfileCtrSizeT<Profile> size() const
    {
        auto sizes = self().sizes();
        return sizes[0];
    }

//    IterSharedPtr<VectorIterator<ValueDataType, ApiProfileT>> seek(CtrSizeT pos) const
//    {
//        typename Types::template SkipForwardWalker<Types, IntList<0>> walker(pos);
//        //return memoria_static_pointer_cast<VectorIterator<ValueDataType, ApiProfileT>>(self().ctr_find(walker));
//      return self().ctr_find(walker);
//    }


    Datum<Value> get(CtrSizeT pos) const
    {
//        auto ii = self().seek(pos);
//        return ii->value();

      MEMORIA_MAKE_GENERIC_ERROR("Operation is not implemented").do_throw();
    }

    void set(CtrSizeT pos, ViewType view)
    {
//        auto ii = self().seek(pos);
//        return ii->set(view);
    }

    void prepend(CtrBatchInputFn<CtrInputBuffer> producer)
    {

    }

    void append(CtrBatchInputFn<CtrInputBuffer> producer)
    {

    }

    void insert(CtrSizeT at, CtrBatchInputFn<CtrInputBuffer> producer)
    {

    }

    CtrSizeT remove(CtrSizeT from, CtrSizeT to)
    {
//        auto& self = this->self();

//        auto ii_from = self.ctr_seek(from);

//        return ii_from->remove_from(to - from);
      return CtrSizeT{};
    }

    CtrSizeT remove_from(CtrSizeT from)
    {
//        auto ctr_size = size();
//        return remove(from, ctr_size);
      return CtrSizeT{};
    }

    CtrSizeT remove_up_to(CtrSizeT pos)
    {
//        auto& self = this->self();

//        auto ii_from = self.ctr_begin();
//        return ii_from->remove_from(pos);

      return CtrSizeT{};
    }


MEMORIA_V1_CONTAINER_PART_END

}
