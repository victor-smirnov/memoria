
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

//#define WIN32_LEAN_AND_MEAN

#include "../message/message.hpp"
#include "../../tools/bzero_struct.hpp"
#include "../../tools/perror.hpp"

#include "msvc_io_poller.hpp"

#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdlib.h>



#include <iostream>
#include <vector>

namespace memoria {
namespace v1 {
namespace reactor {

	
	
class IOBatchBase {
public:
    virtual ~IOBatchBase() {}
    
    virtual OVERLAPPEDMsg* block(size_t idx) = 0;
    virtual size_t nblocks() const = 0;

    virtual void set_submited(size_t num) = 0;
    virtual size_t submited() const = 0;
   
	virtual void configure(AIOMessage* msg) = 0;
};





class FileIOBatch: public IOBatchBase {
    std::vector<OVERLAPPEDMsg> blocks_;

    size_t submited_{};
    
public:
    
    void add_read(void* data, int64_t offset, int64_t size) 
    {
		auto iocb = tools::make_zeroed<OVERLAPPEDMsg>();
		
        iocb.configure(OVERLAPPEDMsg::READ, data, offset, size);
        
		blocks_.push_back(iocb);
    }
    
    void add_write(const void* data, int64_t offset, int64_t size)
    {
		auto iocb = tools::make_zeroed<OVERLAPPEDMsg>();

		iocb.configure(OVERLAPPEDMsg::WRITE, const_cast<void*>(data), offset, size);

		blocks_.push_back(iocb);
    }
    
    
    virtual OVERLAPPEDMsg* block(size_t idx)
    {
        return &blocks_[idx];
    }
    
    virtual size_t nblocks() const {
        return blocks_.size();
    }
    
    virtual void set_submited(size_t num) {
        submited_ = num;
    }
    
    virtual size_t submited() const {return submited_;}

	virtual void configure(AIOMessage* msg) {
		for (auto& ovl : blocks_) ovl.msg_ = msg;
	}
};


    
}}}
