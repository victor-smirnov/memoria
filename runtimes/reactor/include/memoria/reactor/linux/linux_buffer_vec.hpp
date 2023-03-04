
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

#include <memoria/reactor/message/message.hpp>
#include <memoria/core/tools/bzero_struct.hpp>
#include <memoria//core/tools/perror.hpp>

#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <stdlib.h>

#include <linux/aio_abi.h> 

#include <iostream>
#include <vector>

namespace memoria {
namespace reactor {

struct ExtendedIOCB: iocb {
    int64_t processed;
    uint64_t status;
    int errno_;
    
    void dump(int cnt) const {
        std::cout << cnt << ": " << aio_data << "," << aio_resfd << ", " << aio_offset << " " << std::endl;
    }
    
    static void configure(iocb& block, void* data, int64_t offset, int64_t size, int command)
    {
        block.aio_lio_opcode = command;
        block.aio_reqprio = 0;
        
        block.aio_buf = (__u64) data;
        block.aio_nbytes = size;
        block.aio_offset = offset;
        block.aio_flags = IOCB_FLAG_RESFD;
    }
};

class IOBatchBase {
public:
    virtual ~IOBatchBase() {}
    
    virtual iocb** blocks(size_t from = 0) = 0;
    virtual size_t nblocks() const = 0;
    
    virtual ExtendedIOCB& block(size_t idx) = 0;
    virtual const ExtendedIOCB& block(size_t idx) const = 0;
    
    virtual void configure(int fd, int event_fd, Message* message) = 0;
    
    virtual void set_submited(size_t num) = 0;
    virtual size_t submited() const = 0;
    
    virtual void dump() const = 0;
};





class FileIOBatch: public IOBatchBase {
    std::vector<ExtendedIOCB> blocks_;
    std::vector<iocb*> pblocks_;
    
    
    size_t submited_{};
    
public:
    
    void add_read(void* data, int64_t offset, int64_t size) 
    {
        ExtendedIOCB iocb = tools::make_zeroed<ExtendedIOCB>();
        
        iocb.configure(iocb, data, offset, size, IOCB_CMD_PREAD);
        
        blocks_.push_back(iocb);
        pblocks_.push_back(nullptr);
    }
    
    void add_write(const void* data, int64_t offset, int64_t size)
    {
        ExtendedIOCB iocb = tools::make_zeroed<ExtendedIOCB>();
        
        iocb.configure(iocb, const_cast<void*>(data), offset, size, IOCB_CMD_PWRITE);
        
        blocks_.push_back(iocb);
        pblocks_.push_back(nullptr);
    }
    
    
    
    virtual ExtendedIOCB& block(size_t idx) {return blocks_[idx];}
    virtual const ExtendedIOCB& block(size_t idx) const {return blocks_[idx];}
    
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
            MMA_THROW(SystemException(-block.processed))
                    << format_ex(
                           "AIO {} operation failed", (block.aio_lio_opcode == IOCB_CMD_PREAD ? "read" : "write")
                       );
        }
    }
    
    void check_status() const 
    {
        for (size_t c = 0; c < blocks_.size(); c++){
            check_status(c);
        }
    }
    
    
    
    virtual iocb** blocks(size_t from = 0) 
    {
        for (size_t c = 0; c < blocks_.size(); c++){
            pblocks_[c] = &blocks_[c];
        }
        
        return &pblocks_[from];
    }
    
    virtual size_t nblocks() const {
        return pblocks_.size();
    }
    
    virtual void set_submited(size_t num) {
        submited_ = num;
    }
    
    virtual size_t submited() const {return submited_;}
    
    virtual void configure(int fd, int event_fd, Message* message)
    {
        for (auto& block: blocks_)
        {
            block.aio_data = (__u64) message;
            block.aio_fildes = fd;
            block.aio_resfd = event_fd;
        }
    }
    
    virtual void dump() const 
    {
        int cnt = 0;
        for (auto& block: blocks_)
        {
            block.dump(cnt);
            cnt++;
        }
    }
};


    
}}
