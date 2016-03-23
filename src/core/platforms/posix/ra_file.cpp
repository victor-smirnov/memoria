
// Copyright 2013 Victor Smirnov
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



#include <memoria/v1/core/tools/file.hpp>
#include <memoria/v1/core/exceptions/memoria.hpp>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <memoria/v1/core/tools/strings/string.hpp>

namespace memoria {
namespace v1 {

struct RAFileImpl {
    Int fd_ = 0;
};

RAFile::RAFile(): pimpl_(new RAFileImpl)
{}

RAFile::~RAFile()
{
    if (pimpl_->fd_)
    {
        if(::close(pimpl_->fd_) == -1)
        {
            // log this error
        }
    }

    delete pimpl_;
}

void RAFile::open(const char* name, OpenMode mode)
{
    Int flags = 0;

    if (to_bool(mode & OpenMode::READ) && to_bool(mode & OpenMode::WRITE))
    {
        flags = O_RDWR;
    }
    else if (to_bool(mode & OpenMode::READ))
    {
        flags = O_RDONLY;

        if (to_bool(mode & OpenMode::CREATE))
        {
            throw Exception(MA_SRC, "CREATE with READ without WRITE flags is meaningless");
        }
    }
    else if (to_bool(mode & OpenMode::WRITE))
    {
        flags = O_WRONLY;
    }
    else {
        throw Exception(MA_SRC, "At least one READ or WRITE flag must be specified");
    }

    if (to_bool(mode & OpenMode::CREATE))
    {
        flags |= O_CREAT;
    }

    if (to_bool(mode & OpenMode::TRUNC))
    {
        flags |= O_TRUNC;
    }

    pimpl_->fd_ = ::open(name, flags, S_IRUSR | S_IWUSR);

    if (pimpl_->fd_ == -1)
    {
        throw Exception(MA_SRC, SBuf()<<String(strerror(errno)));
    }

    closed_ = false;
}

void RAFile::close()
{
    if ((!closed_) && (::close(pimpl_->fd_) == -1))
    {
        throw Exception(MA_SRC, SBuf()<<String(strerror(errno)));
    }

    closed_ = true;
}

UBigInt RAFile::seek(UBigInt pos, SeekType whence)
{
    Int seek_whence = 0;

    if (whence == SeekType::CUR) {
        seek_whence = SEEK_CUR;
    }
    else if (whence == SeekType::SET) {
        seek_whence = SEEK_SET;
    }
    else if (whence == SeekType::END) {
        seek_whence = SEEK_END;
    }

    off_t offset = lseek(pimpl_->fd_, pos, seek_whence);

    if (offset == (off_t)-1)
    {
        throw Exception(MA_SRC, SBuf()<<"Can't lseek to the specified position: "
                                      <<toString(pos)<<" "<<String(strerror(errno)));
    }
    else if ((seek_whence == SEEK_SET) && (offset != (off_t)pos))
    {
        throw Exception(MA_SRC, SBuf()<<"Failed lseek to the specified position: "<<toString(pos));
    }

    return offset;
}

UBigInt RAFile::read(void* buf, UBigInt size)
{
    ssize_t result = ::read(pimpl_->fd_, buf, size);

    if (result == (ssize_t)-1)
    {
        throw Exception(MA_SRC, SBuf()<<String(strerror(errno)));
    }

    return result;
}

void RAFile::readAll(void* buf, UBigInt size)
{
    ssize_t result = ::read(pimpl_->fd_, buf, size);

    if (result == (ssize_t)-1)
    {
        throw Exception(MA_SRC, SBuf()<<String(strerror(errno)));
    }
    else if (result < (ssize_t)size)
    {
        throw Exception(MA_SRC, SBuf()<<"Failed to read "<<toString(size)<<" bytes");
    }
}

void RAFile::write(const void* buf, UBigInt size)
{
    ssize_t result = ::write(pimpl_->fd_, buf, size);

    if (result == (ssize_t)-1)
    {
        throw Exception(MA_SRC, SBuf()<<String(strerror(errno)));
    }
    else if (result < size)
    {
        throw Exception(MA_SRC, SBuf()<<"Failed to write "<<toString(size)<<" bytes");
    }
}

void RAFile::sync()
{
    if (::fsync(pimpl_->fd_) == -1)
    {
        throw Exception(MA_SRC, SBuf()<<String(strerror(errno)));
    }
}

void RAFile::truncate(UBigInt size)
{
    if (::ftruncate(pimpl_->fd_, size) == -1)
    {
        throw Exception(MA_SRC, SBuf()<<String(strerror(errno)));
    }
}


}}