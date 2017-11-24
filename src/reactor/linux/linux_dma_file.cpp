
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


#include <memoria/v1/reactor/linux/linux_file.hpp>
#include <memoria/v1/reactor/linux/linux_io_poller.hpp>
#include <memoria/v1/reactor/reactor.hpp>
#include <memoria/v1/reactor/message/fiber_io_message.hpp>
#include <memoria/v1/reactor/file_streams.hpp>

#include <memoria/v1/core/tools/ptr_cast.hpp>
#include <memoria/v1/core/tools/bzero_struct.hpp>
#include <memoria/v1/core/tools/perror.hpp>

#include "linux_file_impl.hpp"


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



/*
    
class DMAFile: public FileImpl, public std::enable_shared_from_this<DMAFile> {
    int fd_{};
    bool closed_{true};
public:
    DMAFile (filesystem::path file_path, FileFlags flags, FileMode mode = FileMode::IDEFLT);
    virtual ~DMAFile() noexcept;
    
    virtual uint64_t alignment() {return 512;}
        
    virtual void close();
    virtual bool is_closed() const {return closed_;}
    
    virtual size_t read(uint8_t* buffer, uint64_t offset, size_t size);
    virtual size_t write(const uint8_t* buffer, uint64_t offset, size_t size);
    
    virtual size_t process_batch(IOBatchBase& batch, bool rise_ex_on_error = true);
    
    virtual void fsync() {}
    virtual void fdsync() {}
    
    
    
    
    virtual IDataInputStream istream(uint64_t position = 0, size_t buffer_size = 4096) {
        tools::rise_error(SBuf() << "Streams are not supported for DMA files");
    }
    
    virtual IDataOutputStream ostream(uint64_t position = 0, size_t buffer_size = 4096) 
    {
        //auto buffered_os = std::make_shared<DmaOS<>>(4096, std::static_pointer_cast<File>(shared_from_this()), position);
        //return DataOutputStream(buffered_os.get(), &buffered_os->buffer(), buffered_os);
        tools::rise_error(SBuf() << "Streams are not supported for DMA files");
    }
    
private:
    uint64_t process_single_io(uint8_t* buffer, uint64_t offset, uint64_t size, int command, const char* opname);
};    
    */

class FileSingleIOMessage: public FileIOMessage {
    
    iocb block_;
    
    int64_t size_{};
    int64_t status_{};
    
    FiberContext* fiber_context_;
    
public:
    FileSingleIOMessage(int cpu, int fd, int eventfd, uint8_t* buffer, uint64_t offset, size_t size, int command):
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
    

DMAFileImpl::DMAFileImpl(filesystem::path file_path, FileFlags flags, FileMode mode):
    FileImplBase(file_path)
{
    fd_ = engine().run_in_thread_pool([&]{
        return ::open(file_path.c_str(), (int)flags | O_DIRECT, (mode_t)mode);
    });
    
    if (fd_ < 0) {
        tools::rise_perror(SBuf() << "Can't open file " << file_path);
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
    if ((!closed_) && (::close(fd_) < 0)) {
        tools::rise_perror(SBuf() << "Can't close file " << path_);
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
            tools::rise_perror(SBuf() << "Can't submit AIO batch operations for file " << path_);
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

DMAFile open_dma_file(filesystem::path file_path, FileFlags flags, FileMode mode)
{
    return std::make_shared<DMAFileImpl>(file_path, flags, mode);
}
    
}}}
