
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

#include <memoria/core/container/builder.hpp>
#include <memoria/core/container/names.hpp>

#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/types/typelist.hpp>

#include <memoria/core/exceptions/exceptions.hpp>

namespace memoria {


template <typename Profile, typename BlockType, typename List>
class BlockDispatcher {

    typedef typename List::Head                                                 ContainerType;
    typedef typename ContainerType::Types::Blocks::NodeDispatcher               NodeDispatcher;
    typedef typename ContainerType::Types::TreeNodePtr::BlockType                 NodeBase;

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
        throw DispatchException(MMA_SRC, SBuf()<<"Invalid model hash code"<<block->model_hash());
    }
};

}
