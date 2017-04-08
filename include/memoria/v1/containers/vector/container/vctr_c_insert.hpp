
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
#include <memoria/v1/containers/vector/vctr_tools.hpp>

#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {

using namespace v1::bt;

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::mvector::CtrInsertName)
public:
    using Types = typename Base::Types;
    using typename Base::Iterator;

protected:
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;


    using NodeDispatcher    = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher    = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher  = typename Types::Pages::BranchDispatcher;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;


    typedef typename Types::CtrSizeT                                            CtrSizeT;
    typedef typename Types::Value                                               Value;

    static const Int Streams                                                    = Types::Streams;


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::mvector::CtrInsertName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


#undef M_PARAMS
#undef M_TYPE

}}