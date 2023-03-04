
// Copyright 2017 Victor Smirnov
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

#include <memoria/core/exceptions/exceptions.hpp>

#include "../message/message.hpp"
#include "../../core/tools/bzero_struct.hpp"
#include "../../core/tools/perror.hpp"


#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <stdlib.h>

#include <iostream>
#include <vector>

namespace memoria {
namespace reactor {
    

struct IOCB {
    enum {READ, WRITE};
    
    int32_t command;
    int32_t fd;
    int64_t offset;
    int64_t size;
    void* data;
    
    Message* message;
    
    int64_t processed;
    uint64_t status;
    int errno_;
    
    void configure(void* data, int64_t offset, int64_t size, int32_t command) 
    {
        this->data = data;
        this->offset = offset;
        this->size = size;
        this->command = command;
    }
};

class IOBatchBase {
public:
    virtual ~IOBatchBase() {}
    
    virtual size_t nblocks() const = 0;
    
    virtual IOCB& block(size_t idx) = 0;
    virtual const IOCB& block(size_t idx) const = 0;
    
    virtual void configure(int fd, int event_fd, Message* message) = 0;
    
    virtual void set_submited(size_t num) = 0;
    virtual size_t submited() const = 0;
    
    virtual void dump() const = 0;
};





class FileIOBatch: public IOBatchBase {
    std::vector<IOCB> blocks_;
    
    size_t submited_{};
    
public:
    
    void add_read(void* data, int64_t offset, int64_t size) 
    {
        IOCB iocb = tools::make_zeroed<IOCB>();
        
        iocb.configure(data, offset, size, IOCB::READ);
        
        blocks_.push_back(iocb);
    }
    
    void add_write(const void* data, int64_t offset, int64_t size)
    {
        IOCB iocb = tools::make_zeroed<IOCB>();
        
        iocb.configure(const_cast<void*>(data), offset, size, IOCB::WRITE);
        
        blocks_.push_back(iocb);
    }
    
    
    
    virtual IOCB& block(size_t idx) {return blocks_[idx];}
    virtual const IOCB& block(size_t idx) const {return blocks_[idx];}
    
    int64_t processed(size_t idx) const {
        return blocks_[idx].processed;
    }
    
    
    uint64_t status(size_t idx) const {
        return blocks_[idx].status;
    }
    
    void check_status(int idx) const 
    {
        auto& block = blocks_[idx];
        
        if (block.processed < 0) 
        {
            MMA_THROW(SystemException(-block.processed)) << format_ex(
                "AIO {} operation failed",
                (block.command == IOCB::READ ? "read" : "write")
            );
        }
    }
    
    void check_status() const 
    {
        for (size_t c = 0; c < blocks_.size(); c++){
            check_status(c);
        }
    }
    
    virtual size_t nblocks() const {
        return blocks_.size();
    }
    
    virtual void set_submited(size_t num) {
        submited_ = num;
    }
    
    virtual size_t submited() const {return submited_;}
    
    virtual void configure(int fd, int event_fd, Message* message)
    {
        for (auto& block: blocks_)
        {
            block.message =  message;
            block.fd = fd;
        }
    }
    
    virtual void dump() const 
    {
    }
};


    
}}
