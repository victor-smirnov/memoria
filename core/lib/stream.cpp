
// Copyright 2011 Victor Smirnov
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


#include <memoria/core/tools/stream.hpp>
#include <memoria/core/strings/string.hpp>
#include <memoria/core/strings/format.hpp>

#include <boost/filesystem/operations.hpp>

#include <fstream>

#include <stdlib.h>
#include <stdint.h>

namespace memoria {

class FileOutputStreamHandlerImpl: public FileOutputStreamHandler {
    std::filebuf file_;
    bool closed_;


public:
	FileOutputStreamHandlerImpl(const char* file_name);
	virtual ~FileOutputStreamHandlerImpl() noexcept;


    virtual int32_t bufferSize() {return 0;}

	virtual void flush();

	virtual void close();

	virtual void write(const void* mem, size_t offset, size_t length);

    virtual void write(int8_t value) {
        writeT(value);
    }

    virtual void write(uint8_t value) {
        writeT(value);
    }

    virtual void write(int16_t value) {
        writeT(value);
    }

    virtual void write(uint16_t value) {
        writeT(value);
    }

    virtual void write(int32_t value) {
        writeT(value);
    }

    virtual void write(uint32_t value) {
        writeT(value);
    }

    virtual void write(int64_t value) {
        writeT(value);
    }

    virtual void write(uint64_t value) {
        writeT(value);
    }

    virtual void write(bool value) {
        writeT((int8_t)value);
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
    std::filebuf file_;
    bool closed_;

    int64_t size_;

public:
	FileInputStreamHandlerImpl(const char* file_name);

	virtual ~FileInputStreamHandlerImpl() noexcept;

    virtual int32_t available() {return 0;}
    virtual int32_t bufferSize() {return 0;}

    virtual void close();

	virtual size_t read(void* mem, size_t offset, size_t length);

    virtual int8_t readByte() {
        return readT<int8_t>();
    }

    virtual uint8_t readUByte() {
        return readT<uint8_t>();
    }

    virtual int16_t readShort() {
        return readT<int16_t>();
    }

    virtual uint16_t readUShort() {
        return readT<uint16_t>();
    }

    virtual int32_t readInt() {
        return readT<int32_t>();
    }

    virtual uint32_t readUInt32() {
        return readT<uint32_t>();
    }

    virtual int64_t readInt64() {
        return readT<int64_t>();
    }

    virtual uint64_t readUInt64() {
        return readT<uint64_t>();
    }

    virtual bool readBool() {
        return readT<int8_t>();
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
            MMA_THROW(Exception()) << WhatCInfo("Can't read value from InputStreamHandler");
        }
    }
};
    

FileOutputStreamHandlerImpl::FileOutputStreamHandlerImpl(const char* file_name)
{
    file_.open(file_name, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    
    if (!file_.is_open()) {
        MMA_THROW(Exception()) << WhatInfo(format_u8("Can't open file {}", file_name));
    }
    
    closed_ = false;
}

FileOutputStreamHandlerImpl::~FileOutputStreamHandlerImpl() noexcept
{
    if (!closed_)
    {
        file_.close();
    }
}



void FileOutputStreamHandlerImpl::flush() {
    file_.pubsync();
}

void FileOutputStreamHandlerImpl::close()
{
    if (!closed_)
    {
        file_.close();
        closed_ = true;
    }
}

void FileOutputStreamHandlerImpl::write(const void* mem, size_t offset, size_t length)
{
    const char* data = static_cast<const char*>(mem) + offset;
    size_t total_size = file_.sputn(data, length);

    if (total_size != length)
    {
        MMA_THROW(Exception()) << WhatInfo(format_u8("Can't write {} bytes to file", length));
    }
}





FileInputStreamHandlerImpl::FileInputStreamHandlerImpl(const char* file_name)
{
    file_.open(file_name, std::ios_base::binary | std::ios_base::in);
    
    if (!file_.is_open()) {
        MMA_THROW(Exception()) << WhatInfo(format_u8("Can't open file {}", file_name));
    }
    
    size_ = boost::filesystem::file_size(file_name);
    
    closed_ = false;
}

FileInputStreamHandlerImpl::~FileInputStreamHandlerImpl() noexcept
{
    if (!closed_)
    {
        file_.close();
    }
}


void FileInputStreamHandlerImpl::close()
{
    if (!closed_)
    {
        file_.close();
        closed_ = true;
    }
}


size_t FileInputStreamHandlerImpl::read(void* mem, size_t offset, size_t length)
{
    char* data = static_cast<char*>(mem) + offset;
    size_t size = file_.sgetn (data, length);
    
    return size == length ? size : std::numeric_limits<size_t>::max();
}



std::unique_ptr<FileOutputStreamHandler> FileOutputStreamHandler::create(const char* file) {
    return std::make_unique<FileOutputStreamHandlerImpl>(file);
}

std::unique_ptr<FileInputStreamHandler> FileInputStreamHandler::create(const char* file) {
    return std::make_unique<FileInputStreamHandlerImpl>(file);
}

}
