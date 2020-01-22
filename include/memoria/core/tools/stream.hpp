
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


#pragma once

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/strings/string.hpp>

#include <memoria/filesystem/path.hpp>

#include <fstream>
#include <limits>
#include <memory>

namespace memoria {

struct InputStreamHandler {

    virtual int32_t available()                             = 0;
    virtual void close()                                = 0;
    virtual int32_t bufferSize()                            = 0;
    virtual size_t read(void* mem, size_t offset, size_t length) = 0;


    virtual int8_t readByte()             = 0;
    virtual uint8_t readUByte()           = 0;
    virtual int16_t readShort()           = 0;
    virtual uint16_t readUShort()         = 0;
    virtual int32_t readInt()               = 0;
    virtual uint32_t readUInt32()             = 0;
    virtual int64_t readInt64()         = 0;
    virtual uint64_t readUInt64()       = 0;
    virtual bool readBool()             = 0;
    virtual float readFloat()           = 0;
    virtual double readDouble()         = 0;

    virtual ~InputStreamHandler() noexcept {}

    template <typename T>
    bool read(T &value) {
        return read(&value, sizeof(T));
    }


    virtual bool read(void* mem, int32_t size)
    {
        int32_t size0 = size;
        int32_t ptr = 0;
        while (size > 0)
        {
            int32_t r = read(mem, ptr, size);
            if (r < 0) {
                if (size != size0) {
                    MMA_THROW(Exception()) << WhatCInfo("End Of File");
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



struct OutputStreamHandler {

    virtual int32_t bufferSize() = 0;
    virtual void flush() = 0;
    virtual void close() = 0;
    virtual void write(const void* mem, size_t offset, size_t lenght) = 0;

    virtual void write(int8_t value)      = 0;
    virtual void write(uint8_t value)     = 0;
    virtual void write(int16_t value)     = 0;
    virtual void write(uint16_t value)    = 0;
    virtual void write(int32_t value)       = 0;
    virtual void write(uint32_t value)      = 0;
    virtual void write(int64_t value)    = 0;
    virtual void write(uint64_t value)   = 0;
    virtual void write(bool value)      = 0;
    virtual void write(float value)     = 0;
    virtual void write(double value)    = 0;

    virtual ~OutputStreamHandler() noexcept {}
};

class FileOutputStreamHandler: public OutputStreamHandler {
public:
    static std::unique_ptr<FileOutputStreamHandler> create(const char* file);
    
    static std::unique_ptr<FileOutputStreamHandler> create_buffered(const filesystem::path& file);

    virtual ~FileOutputStreamHandler() noexcept {}
};

class FileInputStreamHandler: public InputStreamHandler {
public:
    static std::unique_ptr<FileInputStreamHandler> create(const char* file);
    
    static std::unique_ptr<FileInputStreamHandler> create_buffered(const filesystem::path& file);
    
    virtual ~FileInputStreamHandler() noexcept {}
};




inline InputStreamHandler& operator>>(InputStreamHandler& in, int8_t& value) {
    value = in.readByte();
    return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, uint8_t& value) {
    value = in.readUByte();
    return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, int16_t& value) {
    value = in.readShort();
    return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, uint16_t& value) {
    value = in.readUShort();
    return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, int32_t& value) {
    value = in.readInt();
    return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, uint32_t& value) {
    value = in.readUInt32();
    return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, int64_t& value) {
    value = in.readInt64();
    return in;
}

inline InputStreamHandler& operator>>(InputStreamHandler& in, uint64_t& value) {
    value = in.readUInt64();
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

inline InputStreamHandler& operator>>(InputStreamHandler& in, U8String& value)
{
    int64_t size = in.readInt64();

    value.to_std_string().clear();
    value.to_std_string().insert(0, size, 0);

    in.read(&value[0], size);

    return in;
}


inline InputStreamHandler& operator>>(InputStreamHandler& in, U16String& value)
{
    int64_t size = in.readInt64();

    U8String u8_value(size, ' ');

    //value.to_std_string().clear();
    //value.to_std_string().insert(0, size, 0);

    in.read(u8_value.data(), size);

    value = u8_value.to_u16();

    return in;
}



template <typename T>
OutputStreamHandler& operator<<(OutputStreamHandler& out, const T& value) {
    out.write(value);
    return out;
}


inline OutputStreamHandler& operator<<(OutputStreamHandler& out, const U8String& value)
{
    out << (int64_t)value.length();

    out.write(value.data(), 0, value.length());
    return out;
}

inline OutputStreamHandler& operator<<(OutputStreamHandler& out, const U16String& value)
{
    U8String u8_value = value.to_u8();

    out << (int64_t)u8_value.length();

    out.write(u8_value.data(), 0, u8_value.length());
    return out;
}

}
