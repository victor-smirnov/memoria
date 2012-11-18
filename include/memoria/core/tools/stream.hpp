
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_STREAM_HPP
#define _MEMORIA_CORE_TOOLS_STREAM_HPP

#include <memoria/metadata/metadata.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/strings.hpp>

#include <fstream>

namespace memoria    {
namespace vapi       {

struct MEMORIA_API InputStreamHandler {

    virtual Int available()                             = 0;
    virtual void close()                                = 0;
    virtual Int bufferSize()                            = 0;
    virtual BigInt pos()                                = 0;
    virtual Int read(void* mem, Int offset, Int length) = 0;

    virtual ~InputStreamHandler() throw() {}

    template <typename T>
    bool read(T &value) {
        return read(&value, sizeof(T));
    }


    virtual bool read(void* mem, Int size)
    {
        Int size0 = size;
        Int ptr = 0;
        while (size > 0) {
            Int r = read(mem, ptr, size);
            if (r < 0) {
                if (size != size0) {
                    throw Exception(MEMORIA_SOURCE, "End Of File");
                }
                else {
                    return false;
                }
            }
            else {
                size -= r;
                ptr += r;
            }
        }
        return true;
    }
};



struct MEMORIA_API OutputStreamHandler {

    virtual Int bufferSize() = 0;
    virtual void flush() = 0;
    virtual void close() = 0;
    virtual BigInt pos() = 0;
    virtual void write(const void* mem, int offset, int lenght) = 0;

    virtual ~OutputStreamHandler() throw() {}

    template <typename T>
    void write(const T& value) {
        write(&value, 0, sizeof(T));
    }
};

class MEMORIA_API FileOutputStreamHandler: public OutputStreamHandler {
public:
    static FileOutputStreamHandler* create(const char* file);

    virtual ~FileOutputStreamHandler() throw() {}
};

class MEMORIA_API FileInputStreamHandler: public InputStreamHandler {
public:
    static FileInputStreamHandler* create(const char* file);
    virtual ~FileInputStreamHandler() throw() {}
};



class FileOutputStreamHandlerImpl: public FileOutputStreamHandler {
    FILE* fd_;
    bool closed_;
public:
    FileOutputStreamHandlerImpl(const char* file)
    {
        closed_ = false;
        fd_ = fopen(file, "wb");
        if (fd_ == NULL)
        {
            throw Exception(MEMORIA_SOURCE, SBuf()<<"Can't open file "<<file);
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

    virtual Int bufferSize() {return 0;}

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
            throw Exception(MEMORIA_SOURCE, SBuf()<<"Can't write "<<length<<" bytes to file");
        }
    }
};



class FileInputStreamHandlerImpl: public FileInputStreamHandler {
    FILE* fd_;
    bool closed_;
public:
    FileInputStreamHandlerImpl(const char* file)
    {
        closed_ = false;
        fd_ = fopen(file, "rb");
        if (fd_ == NULL)
        {
            throw Exception(MEMORIA_SOURCE, SBuf()<<"Can't open file "<<file);
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
    virtual Int bufferSize() {return 0;}

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





}
}

#endif
