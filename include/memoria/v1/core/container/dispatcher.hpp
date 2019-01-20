
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

#include <memoria/v1/core/container/profile.hpp>
#include <memoria/v1/core/container/builder.hpp>
#include <memoria/v1/core/container/names.hpp>

#include <memoria/v1/core/tools/type_name.hpp>
#include <memoria/v1/core/types/typelist.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/metadata/metadata.hpp>

namespace memoria {
namespace v1 {


template <typename Profile, typename BlockType, typename List>
class BlockDispatcher {

    //typedef typename BlockType::ID                                              BlockId;

    typedef typename List::Head                                                 ContainerType;
    typedef typename ContainerType::Types::Blocks::NodeDispatcher               NodeDispatcher;
    typedef typename ContainerType::Types::NodeBase                             NodeBase;

public:
    void dispatch(BlockType *block) {
        if (block->model_hash() == ContainerType::hash()) {
            NodeDispatcher::dispatch(static_cast<NodeBase*>(block));
        }
        else {
            BlockDispatcher<Profile, BlockType, typename ListTail<List>::Type>::dispatch(block);
        }
    }
};

template <typename Profile, typename BlockType>
class BlockDispatcher<Profile, BlockType, TypeList<> > {

public:
    void dispatch(BlockType *block) {
        throw DispatchException(MMA1_SRC, SBuf()<<"Invalid model hash code"<<block->model_hash());
    }
};

}}
