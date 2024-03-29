
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
#include <memoria/reactor/file_streams.hpp>

#include <memoria/reactor/message/fiber_io_message.hpp>

#include <memoria/core/memory/ptr_cast.hpp>
#include <memoria/core/tools/bzero_struct.hpp>
#include <memoria/core/tools/perror.hpp>

#include <memoria/core/tools/time.hpp>

#include <boost/filesystem/operations.hpp>

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
#include <tuple>
#include <memory>


namespace memoria {
namespace reactor {
 

BufferedFileImpl::BufferedFileImpl(boost::filesystem::path file_path, FileFlags flags, FileMode mode):
    FileImplBase(file_path)
{
    int errno0 = 0;
    std::tie(fd_, errno0) = engine().run_in_thread_pool([&]{
        auto fd = ::open(file_path.c_str(), (int)flags, (mode_t)mode);
        return std::make_tuple(fd, errno);
    });

    if (fd_ < 0) {
        MMA_THROW(SystemException(errno0)) << format_ex("Can't open file {}", file_path.string());
    }
}

BufferedFileImpl::~BufferedFileImpl() noexcept
{
    if (!closed_) {
        ::close(fd_);
    }
}


    
void BufferedFileImpl::close()
{
    if ((!closed_) && (::close(fd_) < 0))
    {
        MMA_THROW(SystemException()) << format_ex("Can't close file {}", path_.string());
    }

    closed_ = true;
}

uint64_t BufferedFileImpl::seek(uint64_t position)
{
    off64_t res;
    int errno0;

    std::tie(res, errno0) = engine().run_in_thread_pool([&]{
        off64_t r = lseek64(fd_, position, SEEK_SET);
        return std::make_tuple(r, errno);
    });

    if (res >= 0) {
        return res;
    }
    else {
        MMA_THROW(SystemException(errno0)) << format_ex("Can't seek into the file  {}", path_.string());
    }
}


uint64_t BufferedFileImpl::fpos()
{
    off64_t res;
    int errno0;

    std::tie(res, errno0) = engine().run_in_thread_pool([&]{
        off64_t r = lseek64(fd_, 0, SEEK_CUR);
        return std::make_tuple(r, errno);
    });

    if (res >= 0) {
        return res;
    }
    else {
        MMA_THROW(SystemException(errno0)) << format_ex("Can't seek into the file  {}", path_.string());
    }
}


size_t BufferedFileImpl::read(uint8_t* buffer, uint64_t offset, size_t size)
{
    off64_t res;
    int errno0;

    std::tie(res, errno0) = engine().run_in_thread_pool([&]{
        off64_t r = lseek64(fd_, offset, SEEK_SET);

        if (r >= 0) {
            r = ::read(fd_, buffer, size);
        }

        return std::make_tuple(r, errno);
    });

    if (res >= 0) {
        return res;
    }
    else {
        MMA_THROW(SystemException(errno0)) << format_ex("Can't read from file {}", path_.string());
    }
}


size_t BufferedFileImpl::read(uint8_t* buffer, size_t size)
{
    off64_t res;
    int errno0;

    std::tie(res, errno0) = engine().run_in_thread_pool([&]{
        off64_t r = ::read(fd_, buffer, size);
        return std::make_tuple(r, errno);
    });

    if (res >= 0) {
        return res;
    }
    else {
        MMA_THROW(SystemException(errno0)) << format_ex("Can't read from file {}", path_.string());
    }
}


size_t BufferedFileImpl::write(const uint8_t* buffer, uint64_t offset, size_t size)
{
    off64_t res;
    int errno0 = 0;

    std::tie(res, errno0) = engine().run_in_thread_pool([&]{
        off64_t r = lseek64(fd_, offset, SEEK_SET);

        if (r >= 0) {
            r = ::write(fd_, buffer, size);
        }

        return std::make_tuple(r, errno);
    });

    if (res >= 0) {
        return res;
    }
    else {
        MMA_THROW(SystemException(errno0)) << format_ex("Can't write to file {}", path_.string());
    }
}

size_t BufferedFileImpl::write(const uint8_t* buffer, size_t size)
{
    off64_t res;
    int errno0 = 0;

    std::tie(res, errno0) = engine().run_in_thread_pool([&]{
        off64_t r = ::write(fd_, buffer, size);
        return std::make_tuple(r, errno);
    });

    if (res >= 0) {
        return res;
    }
    else {
        MMA_THROW(SystemException(errno0)) << format_ex("Can't write to file {}", path_.string());
    }
}


void BufferedFileImpl::fsync()
{
    int res;
    int errno0 = 0;

    std::tie(res, errno) = engine().run_in_thread_pool([&]{
        int r = ::fsync(fd_);
        return std::make_tuple(r, errno);
    });

    if (res < 0)
    {
        MMA_THROW(SystemException(errno0)) << format_ex("Can't fsync file {}", path_.string());
    }
}

void BufferedFileImpl::fdsync() {
    int res;
    int errno0 = 0;

    std::tie(res, errno) = engine().run_in_thread_pool([&]{
        int r = ::fdatasync(fd_);
        return std::make_tuple(r, errno);
    });

    if (res < 0) {
        MMA_THROW(SystemException(errno0)) << format_ex("Can't fsync file {}", path_.string());
    }
}

File open_buffered_file(boost::filesystem::path file_path, FileFlags flags, FileMode mode)
{
    return MakeLocalShared<BufferedFileImpl>(file_path, flags, mode);
}

}}
