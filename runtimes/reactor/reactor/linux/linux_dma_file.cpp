
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


#include <memoria/reactor/linux/linux_file.hpp>
#include <memoria/reactor/linux/linux_io_poller.hpp>
#include <memoria/reactor/reactor.hpp>
#include <memoria/reactor/message/fiber_io_message.hpp>
#include <memoria/reactor/file_streams.hpp>

#include <memoria/core/memory/ptr_cast.hpp>
#include <memoria/core/tools/bzero_struct.hpp>
#include <memoria/core/tools/perror.hpp>

#include "linux_file_impl.hpp"
#include "linux_io_messages.hpp"


#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <exception>

namespace memoria {
namespace reactor {

class FileSingleIOMessage: public FileIOMessage {
    
    iocb block_;
    
    int64_t size_{};
    int64_t status_{};
    
    FiberContext* fiber_context_;
    
public:
    FileSingleIOMessage(int cpu, int fd, int eventfd, uint8_t* buffer, uint64_t offset, size_t size, int command):
        FileIOMessage(cpu),
        block_(tools::make_zeroed<iocb>()),
        fiber_context_(boost::fibers::context::active())
    {
        block_.aio_fildes = fd;
        block_.aio_lio_opcode = command;
        block_.aio_reqprio = 0;
        
        block_.aio_buf = (__u64) buffer;
        block_.aio_nbytes = size;
        block_.aio_offset = offset;
        block_.aio_flags = IOCB_FLAG_RESFD;
        block_.aio_resfd = eventfd;
        
        block_.aio_data = (__u64) this;
    }
    
    virtual ~FileSingleIOMessage() {}
    
    virtual uint64_t alignment() {return 512;}
    
    virtual void report(io_event* status) 
    {
        size_ = status->res;
        status_ = status->res2;
    }
    
    FiberContext* fiber_context() {return fiber_context_;}
    const FiberContext* fiber_context() const {return fiber_context_;}
    
    virtual void process() noexcept {
        return_ = true;
    }
    
    virtual void finish() 
    {
        engine().scheduler()->resume(fiber_context_);
    }
    
    virtual std::string describe() {
        return "FileSingleIOMessage";
    }
   
    void wait_for() {
        engine().scheduler()->suspend(fiber_context_);
    }
    
    int64_t size() const {return size_;}
    int64_t status() const {return status_;}
    
    iocb* block() {return &block_;}
}; 
    

DMAFileImpl::DMAFileImpl(boost::filesystem::path file_path, FileFlags flags, FileMode mode):
    FileImplBase(file_path)
{
    fd_ = engine().run_in_thread_pool([&]{
        return ::open(file_path.c_str(), (int)flags | O_DIRECT, (mode_t)mode);
    });
    
    if (fd_ < 0) {
        MMA_THROW(SystemException()) << format_ex("Can't open file {}", file_path.string());
    }
}

DMAFileImpl::~DMAFileImpl() noexcept
{
    if (!closed_) {
        ::close(fd_);
    }
}
    
void DMAFileImpl::close()
{
    if ((!closed_) && (::close(fd_) < 0))
    {
        MMA_THROW(SystemException()) << format_ex("Can't close file ", path_.string());
    }
    closed_ = true;
}




size_t DMAFileImpl::process_single_io(uint8_t* buffer, uint64_t offset, size_t size, int command, const char* opname)
{
    Reactor& r = engine();
    
    FileSingleIOMessage message(r.cpu(), fd_, r.io_poller().event_fd(), buffer, offset, size, command);
    
    iocb* pblock = message.block();
    
    while (true) 
    {
        int res = io_submit(r.io_poller().aio_context(), 1, &pblock);
    
        if (res > 0) 
        {
            message.wait_for();
            
            if (message.size() < 0) {
                MMA_THROW(SystemException(errno)) << format_ex("AIO {} operation failed for file {}", opname, path_.string());
            }
            
            return message.size();
        }
        else if (res < 0)
        {
            MMA_THROW(SystemException(errno)) << format_ex("Can't submit AIO  {} operation for file {}", opname, path_.string());
        }
    }
}


size_t DMAFileImpl::read(uint8_t* buffer, uint64_t offset, size_t size)
{    
    return process_single_io(buffer, offset, size, IOCB_CMD_PREAD, "read");
}




size_t DMAFileImpl::write(const uint8_t* buffer, uint64_t offset, size_t size)
{    
    return process_single_io(const_cast<uint8_t*>(buffer), offset, size, IOCB_CMD_PWRITE, "write");
}








class FileMultiIOMessage: public FileIOMessage {
    
    IOBatchBase& batch_;
    size_t remaining_{};
    
    FiberContext* fiber_context_;
    
public:
    FileMultiIOMessage(int cpu, int fd, int event_fd, IOBatchBase& batch):
        FileIOMessage(cpu),
        batch_(batch),
        remaining_(batch.nblocks()),
        fiber_context_(boost::fibers::context::active())
    {
        batch.configure(fd, event_fd, this);
        return_ = true;
    }
    
    virtual ~FileMultiIOMessage() {}
    
    virtual void report(io_event* status) 
    {
        ExtendedIOCB* eiocb = ptr_cast<ExtendedIOCB>((char*)status->obj);
        eiocb->processed    = status->res;
        eiocb->status       = status->res2;
    }
    
    FiberContext* fiber_context() {return fiber_context_;}
    const FiberContext* fiber_context() const {return fiber_context_;}
    
    virtual void process() noexcept {}
    
    virtual void finish() 
    {
        if (--remaining_ == 0) 
        {
            engine().scheduler()->resume(fiber_context_);
        }
    }
    
    virtual std::string describe() {
        return "FileMultiIOMessage";
    }
   
    void wait_for() {
        engine().scheduler()->suspend(fiber_context_);
    }
    
    void add_submited(size_t num) 
    {
        remaining_ = num;
        batch_.set_submited(batch_.submited() + num);
    }
}; 






size_t DMAFileImpl::process_batch(IOBatchBase& batch, bool rise_ex_on_error)
{
    Reactor& r = engine();
    
    FileMultiIOMessage message(r.cpu(), fd_, r.io_poller().event_fd(), batch);
    
    size_t total = batch.nblocks();
    size_t done = 0;
    
    while (done < total)
    {
        iocb** pblock = batch.blocks(done);
        int to_submit = total - done;
        int res = io_submit(r.io_poller().aio_context(), to_submit, pblock);
    
        if (res > 0)
        {
            message.add_submited(res);
            
            message.wait_for();
            
            done += res;
        } 
        else if (res < 0)
        {
            MMA_THROW(SystemException()) << format_ex("Can't submit AIO batch operations for file {}", path_.string());
        }
        else {
            return 0;
        }
    }
    
    return done;
}




DMABuffer allocate_dma_buffer(size_t size) 
{
	if (size != 0) 
	{
		void* ptr = aligned_alloc(512, size);

		if (ptr) {
			DMABuffer buf(ptr_cast<uint8_t>(ptr));
			return buf;
		}
		else {
            MMA_THROW(OOMException()) << format_ex("Cant allocate dma buffer of {} bytes", size);
		}
	}
	else {
        MMA_THROW(OOMException()) << WhatCInfo("Can't allocate dma buffer of 0 bytes");
	}
}

DMAFile open_dma_file(boost::filesystem::path file_path, FileFlags flags, FileMode mode)
{
    return MakeLocalShared<DMAFileImpl>(file_path, flags, mode);
}
    
}}
