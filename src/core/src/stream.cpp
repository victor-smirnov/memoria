
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/vapi/models/allocator.hpp>
#include <memoria/core/tools/strings.hpp>
#include <memoria/core/vapi/api.hpp>

#ifdef __GNUC__
#include <unistd.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>



namespace memoria { namespace vapi {

using namespace std;

class FileOutputStreamHandlerImpl: public OutputStreamHandlerImplT<FileOutputStreamHandler> {
	FILE* fd_;
	bool closed_;
public:
	FileOutputStreamHandlerImpl(const char* file)
	{
		closed_ = false;
		fd_ = fopen(file, "wb");
		if (fd_ == NULL)
		{
			throw MemoriaException(MEMORIA_SOURCE, "Can't open file "+String(file));
		}
	}

	virtual ~FileOutputStreamHandlerImpl() throw()
	{
		if (!closed_)
		{
			::fclose(fd_);
		}
	}

	virtual BigInt pos() {
		return ftell(fd_);
	}

	virtual Int buffer_size() {return 0;}

	virtual void flush() {

	}

	virtual void close()
	{
		if (!closed_)
		{
			::fclose(fd_);
			closed_ = true;
		}
	}

	virtual void write(const void* mem, int offset, int length)
	{
		const char* data = static_cast<const char*>(mem) + offset;
		size_t total_size = fwrite(data, 1, length, fd_);

		if (total_size != (size_t)length)
		{
			throw MemoriaException(MEMORIA_SOURCE, "Can't write "+ToString(length)+" bytes to file");
		}
	}
};



class FileInputStreamHandlerImpl: public InputStreamHandlerImplT<FileInputStreamHandler> {
	FILE* fd_;
	bool closed_;
public:
	FileInputStreamHandlerImpl(const char* file)
	{
		closed_ = false;
		fd_ = fopen(file, "rb");
		if (fd_ == NULL)
		{
			throw MemoriaException(MEMORIA_SOURCE, "Can't open file "+String(file));
		}
	}

	virtual ~FileInputStreamHandlerImpl() throw()
	{
		if (!closed_)
		{
			::fclose(fd_);
		}
	}

	virtual Int available() {return 0;}
	virtual Int buffer_size() {return 0;}

	virtual void close()
	{
		if (!closed_)
		{
			::fclose(fd_);
			closed_ = true;
		}
	}

	virtual BigInt pos() {
		return ftell(fd_);
	}

	virtual Int read(void* mem, int offset, int length)
	{
		char* data = static_cast<char*>(mem) + offset;
		size_t size = ::fread(data, 1, length, fd_);
		return size == (size_t)length ? size : -1;
	}
};



FileOutputStreamHandler* FileOutputStreamHandler::create(const char* file) {
	return new FileOutputStreamHandlerImpl(file);
}

FileInputStreamHandler* FileInputStreamHandler::create(const char* file) {
	return new FileInputStreamHandlerImpl(file);
}



}}


