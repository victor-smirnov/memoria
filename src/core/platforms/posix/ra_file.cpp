
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
	delete pimpl_;
}

void RAFile::open(const char* name, Int mode)
{
	Int flags = 0;

	if ((mode & READ) && (mode & WRITE))
	{
		flags = O_RDWR;
	}
	else if (mode & READ)
	{
		flags = O_RDONLY;

		if (mode & CREATE)
		{
			throw Exception(MA_SRC, "CREATE with READ without WRITE flags is meaningless");
		}
	}
	else if (mode & WRITE)
	{
		flags = O_WRONLY;
	}
	else {
		throw Exception(MA_SRC, "At least one READ or WRITE flag must be specified");
	}

	if (mode & CREATE)
	{
		flags |= O_CREAT;
	}

	pimpl_->fd_ = ::open(name, flags, S_IRUSR | S_IWUSR);

	if (pimpl_->fd_ == -1)
	{
		throw Exception(MA_SRC, SBuf()<<String(strerror(errno)));
	}
}

void RAFile::close()
{
	if (::close(pimpl_->fd_) == -1)
	{
		throw Exception(MA_SRC, SBuf()<<String(strerror(errno)));
	}
}

UBigInt RAFile::seek(UBigInt pos, Int mode)
{
	Int whence = 0;

	if (mode & CUR) {
		whence = SEEK_CUR;
	}
	else if (mode & SET) {
		whence = SEEK_SET;
	}
	else if (mode & END) {
		whence = SEEK_END;
	}

	off_t offset = lseek(pimpl_->fd_, pos, whence);

	if (offset == (off_t)-1)
	{
		throw Exception(MA_SRC, SBuf()<<"Can't lseek to the specified position: "
									  <<toString(pos)<<" "<<String(strerror(errno)));
	}
	else if ((whence == SEEK_SET) && (offset != pos))
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


}
