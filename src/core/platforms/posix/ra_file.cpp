
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/core/tools/file.hpp>
#include <memoria/core/tools/strings.hpp>
#include <memoria/core/exceptions/memoria.hpp>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

namespace memoria {

using namespace memoria::vapi;

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
	else if ((seek_whence == SEEK_SET) && (offset != pos))
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
	else if (result < size)
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


}
