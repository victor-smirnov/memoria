
// Copyright 2011 Victor Smirnov
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

#include <memoria/core/types.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/vector/vctr_names.hpp>
#include <memoria/containers/vector/vctr_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

#include <iostream>


namespace memoria {

MEMORIA_V1_ITERATOR_PART_BEGIN(mvector::ItrApiName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Types::ValueDataType                                       ValueDataType;
    typedef typename Container::BranchNodeEntry                                 BranchNodeEntry;

    typedef typename Container::Position                                        Position;

    using Profile  = typename Types::Profile;

    using CtrSizeT = ProfileCtrSizeT<Profile>;

    using CtrApiTypes = ICtrApiTypes<typename Types::ContainerTypeName, Profile>;

    using IOVSchema = Linearize<typename CtrApiTypes::IOVSchema>;

    using ValueView = typename DataTypeTraits<ValueDataType>::ViewType;

public:

    Datum<ValueDataType> value() const
    {
        auto& self = this->self();

        psize_t local_pos = self.iter_local_pos();

        if (local_pos < self.iter_leaf_size(0))
        {
            auto& iovv = self.iovector_view();

            using Adapter = IOSubstreamAdapter<Select<0, IOVSchema>>;

            ValueView value_view;

            Adapter::read_one(iovv.substream(0), 0, local_pos, value_view);

            return value_view;
        }
        else {
            MMA_THROW(BoundsException()
                       << format_ex(
                           "Requested index {} is outside of bounds [0, {})",
                           local_pos,
                           self.iter_leaf_size(0)
                       )
            );
        }
    }


    CtrSizeT seek(CtrSizeT pos)
    {
        auto& self = this->self();

        CtrSizeT current_pos = self.pos();
        self.skip(pos - current_pos);
    }

    struct EntryAdapter {
        ValueView view;
        template <int32_t StreamIdx, int32_t SubstreamIdx>
        ValueView get(bt::StreamTag<StreamIdx>, bt::StreamTag<SubstreamIdx>, int32_t block) const {
            return view;
        }
    };

    VoidResult set(ValueView v) noexcept
    {
        auto& self = this->self();
        psize_t local_pos = self.iter_local_pos();

        if (local_pos < self.iter_leaf_size(0))
        {
            return self.ctr().template ctr_update_entry<IntList<1>>(self, EntryAdapter{v});
        }
        else {
            return VoidResult::make_error(
                "Requested index {} is outside of bounds [0, {})",
                local_pos,
                self.iter_leaf_size(0)
            );
        }
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(mvector::ItrApiName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}
