
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

#include <memoria/v1/metadata/metadata.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/tools/strings/string.hpp>
#include <fstream>
#include <limits>
#include <memory>

namespace memoria {
namespace v1 {

struct InputStreamHandler {

    virtual Int available()                             = 0;
    virtual void close()                                = 0;
    virtual Int bufferSize()                            = 0;
    virtual size_t read(void* mem, size_t offset, size_t length) = 0;


    virtual Byte readByte()             = 0;
    virtual UByte readUByte()           = 0;
    virtual Short readShort()           = 0;
    virtual UShort readUShort()         = 0;
    virtual Int readInt()               = 0;
    virtual UInt readUInt()             = 0;
    virtual BigInt readBigInt()         = 0;
    virtual UBigInt readUBigInt()       = 0;
    virtual bool readBool()             = 0;
    virtual float readFloat()           = 0;
    virtual double readDouble()         = 0;

    virtual ~InputStreamHandler() noexcept {}

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



struct OutputStreamHandler {

    virtual Int bufferSize() = 0;
    virtual void flush() = 0;
    virtual void close() = 0;
    virtual void write(const void* mem, size_t offset, size_t lenght) = 0;

    virtual void write(Byte value)      = 0;
    virtual void write(UByte value)     = 0;
    virtual void write(Short value)     = 0;
    virtual void write(UShort value)    = 0;
    virtual void write(Int value)       = 0;
    virtual void write(UInt value)      = 0;
    virtual void write(BigInt value)    = 0;
    virtual void write(UBigInt value)   = 0;
    virtual void write(bool value)      = 0;
    virtual void write(float value)     = 0;
    virtual void write(double value)    = 0;

    virtual ~OutputStreamHandler() noexcept {}
};

class FileOutputStreamHandler: public OutputStreamHandler {
public:
    static std::unique_ptr<FileOutputStreamHandler> create(const char* file);

    virtual ~FileOutputStreamHandler() noexcept {}
};

class FileInputStreamHandler: public InputStreamHandler {
public:
    static std::unique_ptr<FileInputStreamHandler> create(const char* file);
    virtual ~FileInputStreamHandler() noexcept {}
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

}}
