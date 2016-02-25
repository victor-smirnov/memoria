
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_STREAM_HPP
#define _MEMORIA_CORE_TOOLS_STREAM_HPP

#include <memoria/metadata/metadata.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/strings/string.hpp>
#include <fstream>
#include <limits>
#include <memory>

namespace memoria    {

struct MEMORIA_API InputStreamHandler {

    virtual Int available()                             = 0;
    virtual void close()                                = 0;
    virtual Int bufferSize()                            = 0;
    virtual BigInt pos() const                          = 0;
    virtual BigInt size() const                         = 0;
    virtual size_t read(void* mem, size_t offset, size_t length) = 0;


    virtual Byte readByte()				= 0;
    virtual UByte readUByte()			= 0;
    virtual Short readShort()			= 0;
    virtual UShort readUShort()			= 0;
    virtual Int readInt()				= 0;
    virtual UInt readUInt()				= 0;
    virtual BigInt readBigInt()			= 0;
    virtual UBigInt readUBigInt()		= 0;
    virtual bool readBool()				= 0;
    virtual float readFloat()			= 0;
    virtual double readDouble()			= 0;

    virtual ~InputStreamHandler() throw() {}

    template <typename T>
    bool read(T &value) {
        return read(&value, sizeof(T));
    }


    virtual bool read(void* mem, Int size)
    {
        Int size0 = size;
        Int ptr = 0;
        while (size > 0)
        {
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
    virtual void write(const void* mem, size_t offset, size_t lenght) = 0;

    virtual void write(Byte value) 		= 0;
    virtual void write(UByte value) 	= 0;
    virtual void write(Short value) 	= 0;
    virtual void write(UShort value) 	= 0;
    virtual void write(Int value) 		= 0;
    virtual void write(UInt value) 		= 0;
    virtual void write(BigInt value) 	= 0;
    virtual void write(UBigInt value) 	= 0;
    virtual void write(bool value) 		= 0;
    virtual void write(float value) 	= 0;
    virtual void write(double value) 	= 0;

    virtual ~OutputStreamHandler() throw() {}
};

class MEMORIA_API FileOutputStreamHandler: public OutputStreamHandler {
public:
    static std::unique_ptr<FileOutputStreamHandler> create(const char* file);

    virtual ~FileOutputStreamHandler() throw() {}
};

class MEMORIA_API FileInputStreamHandler: public InputStreamHandler {
public:
    static std::unique_ptr<FileInputStreamHandler> create(const char* file);
    virtual ~FileInputStreamHandler() throw() {}
};



class FileOutputStreamHandlerImpl: public FileOutputStreamHandler {
    FILE* fd_;
    bool closed_;


public:
    FileOutputStreamHandlerImpl(const char* file)
    {
        closed_ = false;
        fd_ = fopen64(file, "wb");
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
    	fflush(fd_);
    }

    virtual void close()
    {
        if (!closed_)
        {
            ::fclose(fd_);
            closed_ = true;
        }
    }

    virtual void write(const void* mem, size_t offset, size_t length)
    {
        const char* data = static_cast<const char*>(mem) + offset;
        size_t total_size = fwrite(data, 1, length, fd_);

        if (total_size != length)
        {
            throw Exception(MEMORIA_SOURCE, SBuf()<<"Can't write "<<length<<" bytes to file");
        }
    }

    virtual void write(Byte value) {
    	writeT(value);
    }

    virtual void write(UByte value) {
    	writeT(value);
    }

    virtual void write(Short value) {
    	writeT(value);
    }

    virtual void write(UShort value) {
    	writeT(value);
    }

    virtual void write(Int value) {
    	writeT(value);
    }

    virtual void write(UInt value) {
    	writeT(value);
    }

    virtual void write(BigInt value) {
    	writeT(value);
    }

    virtual void write(UBigInt value) {
    	writeT(value);
    }

    virtual void write(bool value) {
    	writeT((Byte)value);
    }

    virtual void write(float value) {
    	writeT(value);
    }

    virtual void write(double value) {
    	writeT(value);
    }


private:
    template <typename T>
    void writeT(const T& value) {
    	write(&value, 0, sizeof(T));
    }
};



class FileInputStreamHandlerImpl: public FileInputStreamHandler {
    FILE* fd_;
    bool closed_;

    BigInt size_;

public:
    FileInputStreamHandlerImpl(const char* file)
    {
        closed_ = false;
        fd_ = fopen64(file, "rb");
        if (fd_ == NULL)
        {
            throw Exception(MEMORIA_SOURCE, SBuf()<<"Can't open file "<<file);
        }

        if (fseeko(fd_, 0, SEEK_END) < 0)
        {
        	throw Exception(MEMORIA_SOURCE, SBuf()<<"Can't seek to the end for file "<<file);
        }

        size_ = ftello64(fd_);

        if (size_ < 0)
        {
        	throw Exception(MEMORIA_SOURCE, SBuf()<<"Can't read file position for file "<<file);
        }

        if (fseeko64(fd_, 0, SEEK_SET) < 0)
        {
        	throw Exception(MEMORIA_SOURCE, SBuf()<<"Can't seek to the start for file "<<file);
        }
    }

    virtual BigInt size() const {
    	return size_;
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

    virtual BigInt pos() const {
        return ftello64(fd_);
    }



    virtual size_t read(void* mem, size_t offset, size_t length)
    {
        char* data = static_cast<char*>(mem) + offset;
        size_t size = ::fread(data, 1, length, fd_);
        return size == length ? size : std::numeric_limits<size_t>::max();
    }

    virtual Byte readByte() {
    	return readT<Byte>();
    }

    virtual UByte readUByte() {
    	return readT<UByte>();
    }

    virtual Short readShort() {
    	return readT<Short>();
    }

    virtual UShort readUShort() {
    	return readT<UShort>();
    }

    virtual Int readInt() {
    	return readT<Int>();
    }

    virtual UInt readUInt() {
    	return readT<UInt>();
    }

    virtual BigInt readBigInt() {
    	return readT<BigInt>();
    }

    virtual UBigInt readUBigInt() {
    	return readT<UBigInt>();
    }

    virtual bool readBool() {
    	return readT<Byte>();
    }

    virtual float readFloat() {
    	return readT<float>();
    }

    virtual double readDouble() {
    	return readT<double>();
    }

private:
    template <typename T>
    T readT()
    {
    	T value;

    	auto len = read(&value, 0, sizeof(T));

    	if (len == sizeof(T)) {
    		return value;
    	}
    	else {
    		throw Exception(MA_SRC, "Can't read value from InputStreamHandler");
    	}
    }
};


inline InputStreamHandler& operator>>(InputStreamHandler& in, Byte& value) {
	value = in.readByte();
	return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, UByte& value) {
	value = in.readUByte();
	return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, Short& value) {
	value = in.readShort();
	return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, UShort& value) {
	value = in.readUShort();
	return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, Int& value) {
	value = in.readInt();
	return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, UInt& value) {
	value = in.readUInt();
	return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, BigInt& value) {
	value = in.readBigInt();
	return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, UBigInt& value) {
	value = in.readUBigInt();
	return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, bool& value) {
	value = in.readBool();
	return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, float& value) {
	value = in.readFloat();
	return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, double& value) {
	value = in.readDouble();
	return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, String& value)
{
	BigInt size = in.readBigInt();

	value.clear();
	value.insert(0, size, 0);

	in.read(&value[0], size);

	return in;
}


template <typename T>
OutputStreamHandler& operator<<(OutputStreamHandler& out, const T& value) {
	out.write(value);
	return out;
}


inline OutputStreamHandler& operator<<(OutputStreamHandler& out, const String& value)
{
	out << (BigInt)value.length();

	out.write(value.c_str(), 0, value.length());
	return out;
}

}


#endif
