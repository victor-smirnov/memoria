
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {


MEMORIA_V1_CONTAINER_PART_BEGIN(bt::UpdateName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher    = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher    = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher  = typename Types::Pages::BranchDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;


    static const int32_t Streams                                                = Types::Streams;


    using CtrSizeT = typename Types::CtrSizeT;


    template <int32_t Stream, typename SubstreamsList, typename Buffer>
    void update_stream_entry(Iterator& iter, int32_t stream, int32_t idx, const Buffer& entry)
    {
        auto& self = this->self();

        auto result = self.template try_update_stream_entry<Stream, SubstreamsList>(iter, idx, entry);

        if (!std::get<0>(result))
        {
            iter.split(stream, idx);

            result = self.template try_update_stream_entry<Stream, SubstreamsList>(iter, idx, entry);

            if (!std::get<0>(result))
            {
                MMA1_THROW(Exception()) << WhatCInfo("Second insertion attempt failed");
            }
        }

        self.update_path(iter.leaf());
    }



MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::UpdateName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}}
