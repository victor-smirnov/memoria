
// Copyright 2015 Victor Smirnov
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


#include <memoria/v1/containers/map/map_names.hpp>
#include <memoria/v1/containers/map/map_tools.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::bt::RemoveName)

    typedef typename Base::Types                                                Types;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher    = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher    = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher  = typename Types::Pages::BranchDispatcher;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

protected:
    template <int32_t Stream>
    void remove_stream_entry(Iterator& iter, int32_t stream, int32_t idx)
    {
        auto& self = this->self();

        auto result = self.template try_remove_stream_entry<Stream>(iter, idx);

        if (!std::get<0>(result))
        {
            iter.split(stream, idx);

            result = self.template try_remove_stream_entry<Stream>(iter, idx);

            if (!std::get<0>(result))
            {
                MMA1_THROW(Exception()) << WhatCInfo("Second removal attempt failed");
            }

            self.update_path(iter.leaf());
        }
        else {
            self.update_path(iter.leaf());

            auto next = self.getNextNodeP(iter.leaf());

            if (next.isSet())
            {
                self.mergeLeafNodes(iter.leaf(), next, [](const Position&){});
            }

            auto prev = self.getPrevNodeP(iter.leaf());

            if (prev.isSet())
            {
                self.mergeLeafNodes(prev, iter.leaf(), [&](const Position& sizes){
                    iter.idx() += sizes[0];
                    iter.leaf() = prev;
                });
            }
        }
    }



MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::bt::RemoveName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}
