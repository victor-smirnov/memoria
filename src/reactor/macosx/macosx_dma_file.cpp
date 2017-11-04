
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


#include <memoria/v1/reactor/macosx/macosx_file.hpp>
#include <memoria/v1/reactor/macosx/macosx_io_poller.hpp>
#include <memoria/v1/reactor/reactor.hpp>
#include <memoria/v1/reactor/message/fiber_io_message.hpp>
#include <memoria/v1/reactor/file_streams.hpp>

#include <memoria/v1/core/tools/ptr_cast.hpp>
#include <memoria/v1/core/tools/bzero_struct.hpp>
#include <memoria/v1/core/tools/perror.hpp>

#include "../file_impl.hpp"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <boost/align/aligned_alloc.hpp>


#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <exception>
#include <stdlib.h>


namespace memoria {
namespace v1 {
namespace reactor {


class DMAFile: public FileImpl, public std::enable_shared_from_this<DMAFile> {
    int fd_{};
    bool closed_{true};
public:
    DMAFile(filesystem::path file_path, FileFlags flags, FileMode mode = FileMode::IDEFLT): 
        FileImpl(file_path)
    {
        int errno0 = 0;
        std::tie(fd_, errno0) = engine().run_in_thread_pool([&]{
            auto fd = ::open(file_path.c_str(), (int)flags, (mode_t)mode);
            if (fd >= 0) {
                ::fcntl(fd, F_NOCACHE, 1);
            }
            
            return std::make_tuple(fd, errno);
        });
        
        if (fd_ < 0) {
            tools::rise_perror(errno0, SBuf() << "Can't open file " << file_path);
        }
    }
    
    virtual ~DMAFile() noexcept 
    {
        if (!closed_) {
            ::close(fd_);
        }
    }
    
    virtual uint64_t alignment() {return 512;}
    
    virtual void close() 
    {   
        if ((!closed_) && (::close(fd_) < 0)) 
        {
            tools::rise_perror(SBuf() << "Can't close file " << path_);
        }
        
        closed_ = true;
    }
    
    virtual size_t read(uint8_t* buffer, uint64_t offset, size_t size) 
    {
        off_t res;
        int errno0;
        
        std::tie(res, errno0) = engine().run_in_thread_pool([&]{
            off_t r = ::lseek(fd_, offset, SEEK_SET);
            
            if (r >= 0) {
                r = ::read(fd_, buffer, size);
            }
            
            return std::make_tuple(r, errno);
        });
        
        if (res >= 0) {
            return res;
        }
        else {
            tools::rise_perror(errno0, SBuf() << "Can't read from file " << path_);
        }
    }
    
    virtual size_t write(const uint8_t* buffer, uint64_t offset, size_t size) 
    {
        off_t res;
        int errno0 = 0;
        
        std::tie(res, errno0) = engine().run_in_thread_pool([&]{
            off_t r = ::lseek(fd_, offset, SEEK_SET);
            
            if (r >= 0) {
                r = ::write(fd_, buffer, size);
            }
            
            return std::make_tuple(r, errno);
        });
        
        if (res >= 0) {
            return res;
        }
        else {
            tools::rise_perror(errno0, SBuf() << "Can't write to file " << path_);
        }
    }
    
    virtual size_t process_batch(IOBatchBase& batch, bool rise_ex_on_error = true) 
    {
        return engine().run_in_thread_pool([&]{
            size_t c;
            for (c = 0; c < batch.nblocks(); c++) 
            {
                IOCB& iocb = batch.block(c);
                
                if (iocb.command == IOCB::READ)
                {
                    if (::lseek(fd_, iocb.offset, SEEK_SET) >= 0)
                    {
                        iocb.processed = ::read(fd_, iocb.data, iocb.size);
                        if (iocb.processed < 0) 
                        {
                            iocb.errno_ = errno;
                            break;
                        }
                    }
                    else {
                        iocb.errno_ = errno;
                        break;
                    }
                }
                else if (iocb.command == IOCB::WRITE) 
                {
                    if (::lseek(fd_, iocb.offset, SEEK_SET)) 
                    {
                        iocb.processed = ::write(fd_, iocb.data, iocb.size);
                        if (iocb.processed < 0) 
                        {
                            iocb.errno_ = errno;
                            break;
                        }
                    }
                    else {
                        iocb.errno_ = errno;
                        break;
                    }
                }
                else {
                    iocb.errno_ = EINVAL;
                    break;
                }
            }
            
            return c;
        });
    }
    
    virtual void fsync() 
    {
        int res;
        int errno0 = 0;
        
        std::tie(res, errno) = engine().run_in_thread_pool([&]{
            int r = ::fsync(fd_);
            return std::make_tuple(r, errno);
        });
        
        if (res < 0) {
            tools::rise_perror(errno0, SBuf() << "Can't fsync file " << path_);
        }
    }
    
    virtual void fdsync() {
        int res;
        int errno0 = 0;
        
        std::tie(res, errno) = engine().run_in_thread_pool([&]{
            int r = ::fcntl(fd_, F_FULLFSYNC);
            return std::make_tuple(r, errno);
        });
        
        if (res < 0) {
            tools::rise_perror(errno0, SBuf() << "Can't fsync file " << path_);
        }
    }
    
    virtual DataInputStream istream(uint64_t position = 0, size_t buffer_size = 4096) {
        tools::rise_error(SBuf() << "Streams are not supported for DMA files");
    }
    
    virtual DataOutputStream ostream(uint64_t position = 0, size_t buffer_size = 4096) 
    {
        tools::rise_error(SBuf() << "Streams are not supported for DMA files");
    }
};      
    


DMABuffer allocate_dma_buffer(size_t size) 
{
	if (size != 0) 
	{
		void* ptr = boost::alignment::aligned_alloc(512, size);

		if (ptr) {
			DMABuffer buf(tools::ptr_cast<uint8_t>(ptr));
			return buf;
		}
		else {
			tools::rise_perror(SBuf() << "Cant allocate dma buffer of " << size << " bytes");
		}
	}
	else {
		tools::rise_error(SBuf() << "Cant allocate dma buffer of 0 bytes");
	}
}

File open_dma_file(filesystem::path file_path, FileFlags flags, FileMode mode) 
{
    return std::static_pointer_cast<FileImpl>(std::make_shared<DMAFile>(file_path, flags, mode));
}
    
}}}
