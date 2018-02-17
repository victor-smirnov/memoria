
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




#include <memoria/v1/core/tools/stream.hpp>
#include <memoria/v1/core/strings/string.hpp>

#include <memoria/v1/reactor/file.hpp>


#include <fstream>

#include <stdlib.h>
#include <stdint.h>

namespace memoria {
namespace v1 {
namespace reactor {

class BufferedFileOutputStreamHandlerImpl: public FileOutputStreamHandler {
    File file_;
    uint64_t position_{};
    
public:
	BufferedFileOutputStreamHandlerImpl(const filesystem::path& file_name);
	virtual ~BufferedFileOutputStreamHandlerImpl() noexcept;


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



class BufferedFileInputStreamHandlerImpl: public FileInputStreamHandler {
    File file_;
    uint64_t position_{};
    int64_t size_;

public:
	BufferedFileInputStreamHandlerImpl(const filesystem::path& file_name);

	virtual ~BufferedFileInputStreamHandlerImpl() noexcept;

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
            MMA1_THROW(Exception()) << WhatCInfo("Can't read value from InputStreamHandler");
        }
    }
};
    

BufferedFileOutputStreamHandlerImpl::BufferedFileOutputStreamHandlerImpl(const filesystem::path& file_name):
    file_(open_buffered_file(file_name, FileFlags::WRONLY | FileFlags::CREATE)) //
{
}

BufferedFileOutputStreamHandlerImpl::~BufferedFileOutputStreamHandlerImpl() noexcept
{
}



void BufferedFileOutputStreamHandlerImpl::flush() {
    file_.fsync();
}

void BufferedFileOutputStreamHandlerImpl::close()
{
    file_.close();
}

void BufferedFileOutputStreamHandlerImpl::write(const void* mem, size_t offset, size_t length)
{
    const uint8_t* data = static_cast<const uint8_t*>(mem) + offset;
    size_t total_size = file_.write(data, position_, length);

    position_ += total_size;

    if (total_size != length)
    {
        MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Can't write {} bytes to file", length));
    }
}





BufferedFileInputStreamHandlerImpl::BufferedFileInputStreamHandlerImpl(const filesystem::path& file_name):
    file_(open_buffered_file(file_name, FileFlags::RDONLY))
{
    size_ = memoria::v1::filesystem::file_size(file_name);
}

BufferedFileInputStreamHandlerImpl::~BufferedFileInputStreamHandlerImpl() noexcept
{
}


void BufferedFileInputStreamHandlerImpl::close()
{
    file_.close();
}


size_t BufferedFileInputStreamHandlerImpl::read(void* mem, size_t offset, size_t length)
{
    uint8_t* data = static_cast<uint8_t*>(mem) + offset;
    size_t size = file_.read(data, position_, length);
    
    position_ += size;
    
    return size;
}


}

std::unique_ptr<FileOutputStreamHandler> FileOutputStreamHandler::create_buffered(const filesystem::path& file) {
    return std::make_unique<reactor::BufferedFileOutputStreamHandlerImpl>(file);
}

std::unique_ptr<FileInputStreamHandler> FileInputStreamHandler::create_buffered(const filesystem::path& file) {
    return std::make_unique<reactor::BufferedFileInputStreamHandlerImpl>(file);
}
    
    
}}
