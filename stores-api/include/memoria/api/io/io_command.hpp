
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

#include <memoria/api/io/block_ptr.hpp>
#include <memoria/profiles/common/block.hpp>

#include <memory>
#include <vector>

namespace memoria::io {

enum class CommandType {
    READ, WRITE, MOVE, DISCARD, SEQENTIAL_GROUP, PARALLEL_GROUP
};

using BasicBlockPtr = BlockPtr<BasicBlockHeader>;

using DevSizeT = uint64_t;

struct IOCommand {
    virtual ~IOCommand() = default;
    virtual CommandType type() const = 0;
};

using IOCmdPtr = std::shared_ptr<IOCommand>;

class IOReadCommand: public IOCommand {
    DevSizeT block_address_;
    BlkSizeT block_size_;
    BasicBlockPtr block_;
public:
    IOReadCommand(DevSizeT block_address, BlkSizeT block_size, BasicBlockPtr block = BasicBlockPtr{}):
        block_address_(block_address), block_size_(block_size), block_(block)
    {}

    CommandType type() const override {
        return CommandType::READ;
    }

    DevSizeT block_address() const {return block_address_;}
    BlkSizeT block_size() const {return block_size_;}
    const BasicBlockPtr& block() const {return block_;}

    void set_block(BasicBlockPtr block) {
        block_ = block;
    }
};

class IOWriteCommand: public IOCommand {
    DevSizeT block_address_;
    BlkSizeT block_size_;
    BasicBlockPtr block_;
public:
    IOWriteCommand(DevSizeT block_address, BlkSizeT block_size, BasicBlockPtr block):
        block_address_(block_address), block_size_(block_size), block_(block)
    {}

    CommandType type() const override {
        return CommandType::WRITE;
    }

    DevSizeT block_address() const {return block_address_;}
    BlkSizeT block_size() const {return block_size_;}
    const BasicBlockPtr& block() const {return block_;}
};



class IOMoveCommand: public IOCommand {
    DevSizeT source_block_address_;
    DevSizeT target_block_address_;
    BlkSizeT block_size_;
    BasicBlockPtr block_;
public:
    IOMoveCommand(
            DevSizeT source_block_address,
            DevSizeT target_block_address,
            BlkSizeT block_size,
            BasicBlockPtr block = BasicBlockPtr{}
    ):
        source_block_address_(source_block_address),
        target_block_address_(target_block_address),
        block_size_(block_size), block_(block)
    {}

    CommandType type() const override {
        return CommandType::MOVE;
    }

    DevSizeT source_block_address() const {return source_block_address_;}
    DevSizeT target_block_address() const {return target_block_address_;}
    BlkSizeT block_size() const {return block_size_;}
    const BasicBlockPtr& block() const {return block_;}
};


class IOSeqExecutionGroup: public IOCommand {
    std::vector<IOCmdPtr> commands_;
public:
    IOSeqExecutionGroup(std::initializer_list<IOCmdPtr> commands):
        commands_(commands)
    {}

    IOSeqExecutionGroup(std::vector<IOCmdPtr> commands):
        commands_(std::move(commands))
    {}

    CommandType type() const override {
        return CommandType::SEQENTIAL_GROUP;
    }

    const std::vector<IOCmdPtr>& commands() {
        return commands_;
    }
};


class IOParExecutionGroup: public IOCommand {
    std::vector<IOCmdPtr> commands_;
public:
    IOParExecutionGroup(std::initializer_list<IOCmdPtr> commands):
        commands_(commands)
    {}

    IOParExecutionGroup(std::vector<IOCmdPtr> commands):
        commands_(std::move(commands))
    {}

    CommandType type() const override {
        return CommandType::PARALLEL_GROUP;
    }

    const std::vector<IOCmdPtr>& commands() {
        return commands_;
    }
};


}
