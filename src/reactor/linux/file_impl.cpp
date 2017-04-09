
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


#include <memoria/v1/reactor/linux/file_impl.hpp>
#include <memoria/v1/reactor/linux/io_poller.hpp>
#include <memoria/v1/reactor/reactor.hpp>
#include <memoria/v1/reactor/message/fiber_io_message.hpp>

#include <memoria/v1/core/tools/ptr_cast.hpp>
#include <memoria/v1/core/tools/bzero_struct.hpp>
#include <memoria/v1/core/tools/perror.hpp>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


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
namespace v1 {
namespace reactor {
    
class FileSingleIOMessage: public FileIOMessage {
    
    iocb block_;
    
    int64_t size_{};
    uint64_t status_{};
    
    FiberContext* fiber_context_;
    
public:
    FileSingleIOMessage(int cpu, int fd, int eventfd, char* buffer, int64_t offset, int64_t size, int command):
        FileIOMessage(cpu),
        block_(tools::make_zeroed<iocb>()),
        fiber_context_(fibers::context::active())
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
    uint64_t status() const {return status_;}
    
    iocb* block() {return &block_;}
}; 
    

File::File(std::string path, FileFlags flags, FileMode mode): 
    path_(path) 
{
    fd_ = ::open(path_.c_str(), (int)flags | O_DIRECT, (mode_t)mode);
    if (fd_ < 0)
    {
        tools::rise_perror(SBuf() << "Can't open file " << path);
    }
}

File::~File() noexcept
{
    
}
    
void File::close()
{
    if (::close(fd_) < 0) {
        tools::rise_perror(SBuf() << "Can't close file " << path_);
    }
}

int64_t File::seek(int64_t pos, FileSeek whence) 
{
    off_t res = ::lseek(fd_, pos, (int)whence);
    if (res >= 0) 
    {
        return res;
    }
    else {
        tools::rise_perror(SBuf() << "Error seeking in file " << path_);
    }
}


int64_t File::process_single_io(char* buffer, int64_t offset, int64_t size, int command, const char* opname) 
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
                tools::rise_perror(-message.size(), SBuf() << "AIO " << opname << " operation failed for file " << path_);
            }
            
            return message.size();
        } 
        else if (res < 0)
        {
            tools::rise_perror(SBuf() << "Can't submit AIO " << opname << " operation for file " << path_);
        }
    }
}


int64_t File::read(char* buffer, int64_t offset, int64_t size) 
{    
    return process_single_io(buffer, offset, size, IOCB_CMD_PREAD, "read");
}




int64_t File::write(const char* buffer, int64_t offset, int64_t size)
{    
    return process_single_io(const_cast<char*>(buffer), offset, size, IOCB_CMD_PWRITE, "write");
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
        fiber_context_(fibers::context::active())
    {
        batch.configure(fd, event_fd, this);
        return_ = true;
    }
    
    virtual ~FileMultiIOMessage() {}
    
    virtual void report(io_event* status) 
    {
        ExtendedIOCB* eiocb = tools::ptr_cast<ExtendedIOCB>((char*)status->obj);
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






size_t File::process_batch(IOBatchBase& batch, bool rise_ex_on_error) 
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
            tools::rise_perror(SBuf() << "Can't submit AIO batch operations for file " << path_);
        }
        else {
            return 0;
        }
    }
    
    return done;
}








void File::fsync()
{
    // NOOP at the moment
}

void File::fdsync()
{
    // NOOP at the moment
}

DMABuffer allocate_dma_buffer(size_t size) 
{
	if (size != 0) 
	{
		void* ptr = aligned_alloc(512, size);

		if (ptr) {
			DMABuffer buf(tools::ptr_cast<char>(ptr));
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
    
}}}
