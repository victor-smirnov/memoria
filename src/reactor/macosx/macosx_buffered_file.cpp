
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
#include <memoria/v1/reactor/file_streams.hpp>

#include <memoria/v1/reactor/message/fiber_io_message.hpp>

#include <memoria/v1/core/tools/ptr_cast.hpp>
#include <memoria/v1/core/tools/bzero_struct.hpp>
#include <memoria/v1/core/tools/perror.hpp>

#include <memoria/v1/core/tools/time.hpp>

#include "macosx_file_impl.hpp"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <exception>
#include <tuple>
#include <memory>


namespace memoria {
namespace v1 {
namespace reactor {
 


    

BufferedFileImpl::BufferedFileImpl(filesystem::path file_path, FileFlags flags, FileMode mode):
    FileImplBase(file_path)
{
    int errno0 = 0;
    std::tie(fd_, errno0) = engine().run_in_thread_pool([&]{
        auto fd = ::open(file_path.c_str(), (int)flags, (mode_t)mode);
        return std::make_tuple(
            fd,
            errno
        );
    });

    if (fd_ < 0) {
        MMA1_THROW(SystemException(errno0)) << fmt::format_ex(u"Can't open file {}", file_path);
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
        MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't close file {}", path_);
    }

    closed_ = true;
}



uint64_t BufferedFileImpl::seek(uint64_t position) {
    off_t res;
    int errno0;

    std::tie(res, errno0) = engine().run_in_thread_pool([&]{
        off_t r = lseek(fd_, position, SEEK_SET);
        return std::make_tuple(r, errno);
    });

    if (res >= 0) {
        return res;
    }
    else {
        MMA1_THROW(SystemException(errno0)) << fmt::format_ex(u"Can't read from file {}", path_);
    }
}


uint64_t BufferedFileImpl::fpos() {
    off_t res;
    int errno0;

    std::tie(res, errno0) = engine().run_in_thread_pool([&]{
        off_t r = lseek(fd_, 0, SEEK_CUR);
        return std::make_tuple(r, errno);
    });

    if (res >= 0) {
        return res;
    }
    else {
        MMA1_THROW(SystemException(errno0)) << fmt::format_ex(u"Can't read from file {}", path_);
    }
}


uint64_t BufferedFileImpl::size() {
    return filesystem::file_size(path_);
}

size_t BufferedFileImpl::read(uint8_t* buffer, size_t size)
{
    off_t res;
    int errno0;

    std::tie(res, errno0) = engine().run_in_thread_pool([&]{
        auto r = ::read(fd_, buffer, size);
        return std::make_tuple(r, errno);
    });

    if (res >= 0) {
        return res;
    }
    else {
        MMA1_THROW(SystemException(errno0)) << fmt::format_ex(u"Can't read from file {}", path_);
    }
}
    
size_t BufferedFileImpl::write(const uint8_t* buffer, size_t size)
{
    ssize_t res;
    int errno0;

    std::tie(res, errno0) = engine().run_in_thread_pool([&]{
        auto r = ::write(fd_, buffer, size);
        return std::make_tuple(r, errno);
    });

    if (res >= 0) {
        return res;
    }
    else {
        MMA1_THROW(SystemException(errno0)) << fmt::format_ex(u"Can't write to file {}", path_);
    }
}





size_t BufferedFileImpl::read(uint8_t* buffer, uint64_t offset, size_t size)
{
    off_t res;
    int errno0;

    std::tie(res, errno0) = engine().run_in_thread_pool([&]{
        off_t r = lseek(fd_, offset, SEEK_SET);

        if (r >= 0) {
            r = ::read(fd_, buffer, size);
        }

        return std::make_tuple(r, errno);
    });

    if (res >= 0) {
        return res;
    }
    else {
        MMA1_THROW(SystemException(errno0)) << fmt::format_ex(u"Can't read from file {}", path_);
    }
}



size_t BufferedFileImpl::write(const uint8_t* buffer, uint64_t offset, size_t size)
{
    off_t res;
    int errno0 = 0;

    std::tie(res, errno0) = engine().run_in_thread_pool([&]{
        off_t r = lseek(fd_, offset, SEEK_SET);

        if (r >= 0) {
            r = ::write(fd_, buffer, size);
        }

        return std::make_tuple(r, errno);
    });

    if (res >= 0) {
        return res;
    }
    else {
        MMA1_THROW(SystemException(errno0)) << fmt::format_ex(u"Can't write to file {}", path_);
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

    if (res < 0) {
        MMA1_THROW(SystemException(errno0)) << fmt::format_ex(u"Can't fsync file {}", path_);
    }
}

void BufferedFileImpl::fdsync() {
    int res;
    int errno0 = 0;

    std::tie(res, errno) = engine().run_in_thread_pool([&]{
        int r = ::fcntl(fd_, F_FULLFSYNC);
        return std::make_tuple(r, errno);
    });

    if (res < 0) {
        MMA1_THROW(SystemException(errno0)) << fmt::format_ex(u"Can't fsync file {}", path_);
    }
}


File open_buffered_file(filesystem::path file_path, FileFlags flags, FileMode mode) 
{
    return MakeLocalShared<BufferedFileImpl>(file_path, flags, mode);
}
    
}}}
