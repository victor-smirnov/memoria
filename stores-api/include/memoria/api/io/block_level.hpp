
// Copyright 2023 Victor Smirnov
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
#include <memoria/profiles/common/block.hpp>
#include <memoria/api/io/block_ptr.hpp>
#include <memoria/api/io/io_command.hpp>

#include <atomic>

namespace memoria::io {


struct BlockIOProvider {

    virtual ~BlockIOProvider() noexcept = default;

    virtual void execute(const IOCmdPtr& graph) = 0;

    // in bytes
    virtual DevSizeT extent_size() = 0;

    // in bytes
    virtual DevSizeT resize(DevSizeT new_size) = 0;

    // in bytes
    virtual DevSizeT size() = 0;

    virtual BlockPtr<BasicBlockHeader> make_block(BlkSizeT size) = 0;

    virtual void close() = 0;
};

}
