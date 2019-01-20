
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

#include <memoria/v1/metadata/group.hpp>
#include <memoria/v1/metadata/block.hpp>
#include <memoria/v1/core/container/blocks.hpp>


#include <iostream>

namespace memoria {
namespace v1 {

template <typename Profile>
void dumpBlock(BlockMetadata<Profile>* meta, const Block* block, std::ostream& out)
{
    TextBlockDumper dumper(out);
    meta->getBlockOperations()->generateDataEvents(T2T<const ProfileBlockType<Profile>*>(block->Ptr()), DataEventsParams(), &dumper);
}

template <typename Profile>
void dumpBlockData(BlockMetadata<Profile>* meta, const ProfileBlockType<Profile>* block, std::ostream& out)
{
    TextBlockDumper dumper(out);

    meta->getBlockOperations()->generateDataEvents(block, DataEventsParams(), &dumper);
}

}}
