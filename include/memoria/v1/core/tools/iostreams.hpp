

// Copyright 2017 Victor Smirnov
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

#include <memoria/v1/core/iobuffer/io_buffer.hpp>
#include <memoria/v1/core/tools/perror.hpp>
#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/tools/ptr_cast.hpp>
#include <memoria/v1/core/tools/referenceable.hpp>
#include <memoria/v1/core/tools/pimpl_base.hpp>

#include <limits>
#include <memory>
#include <utility>
#include <type_traits>

#include <stdint.h>

namespace memoria {
namespace v1 {

class IBinaryInputStream: public Referenceable {
public:
    virtual ~IBinaryInputStream() noexcept {}
    virtual size_t read(uint8_t* data, size_t size) = 0;
    virtual void close() = 0;
    virtual bool is_closed() const = 0;
};


class IBinaryOutputStream: public Referenceable {
public:
    virtual ~IBinaryOutputStream() noexcept {}
    virtual size_t write(const uint8_t* data, size_t size) = 0;
    virtual void flush() = 0;
    virtual void close() = 0;
    virtual bool is_closed() const = 0;
};

class IBinaryIOStream: public IBinaryInputStream, public IBinaryOutputStream {
public:
    virtual ~IBinaryIOStream() noexcept {}
};


struct BinaryInputStream final: PimplBase<IBinaryInputStream> {

    using Base = PimplBase<IBinaryInputStream>;

    MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS(BinaryInputStream)


    size_t read(uint8_t* data, size_t size) {
        return ptr_->read(data, size);
    }

    bool is_closed() const {
        return this->ptr_->is_closed();
    }

    void close() {
        return this->ptr_->close();
    }

};


struct BinaryOutputStream final: PimplBase<IBinaryOutputStream>{

    using Base = PimplBase<IBinaryOutputStream>;

    MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS(BinaryOutputStream)
    
    size_t write(const uint8_t* data, size_t size) {
        return ptr_->write(data, size);
    }
    
    void flush() {
        return ptr_->flush();
    }

    void close() {
        ptr_->close();
    }


    bool is_closed() const {
        return this->ptr_->is_closed();
    }
};


class IDataOutputStream;
class IDataInputStream;




class IDataInputStream {
    using BufferT = DefaultIOBuffer;
    IBinaryInputStream* stream_;
    BufferT* buffer_;
    std::shared_ptr<Referenceable> holder_;
public:
    IDataInputStream(IBinaryInputStream* stream, BufferT* buffer_, std::shared_ptr<Referenceable> holder):
        stream_(stream), buffer_(buffer_), holder_(holder)
    {
        buffer_->limit(0);
        pull();
    }
    
    template <typename T>
    static constexpr size_t type_size() 
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return BufferT::ValueSize<T>::value;
    }
    
    IDataInputStream(const IDataInputStream&) = delete;
    IDataInputStream& operator=(const IDataInputStream&) = delete;
    
    IDataInputStream(IDataInputStream&& other): 
        stream_(other.stream_), 
        buffer_(other.buffer_),
        holder_(std::move(other.holder_))
    {
        other.stream_ = nullptr;
        other.buffer_ = nullptr;
    }
    
    IDataInputStream& operator=(IDataInputStream&& other) 
    {
        if (this != &other) 
        {
            stream_ = other.stream_; 
            buffer_ = other.buffer_;
            holder_ = std::move(other.holder_);
        }
        
        return *this;
    }
    
    void swap(IDataInputStream& other) 
    {
        std::swap(stream_, other.stream_);
        std::swap(buffer_, other.buffer_);
        std::swap(holder_, other.holder_);
    }
    
    char readChar()
    {
        if (MMA1_UNLIKELY(!buffer_->has_capacity(type_size<char>())))
        {
            pull_and_ensure(type_size<char>());
        }
        
        return (char)buffer_->getByte();
    }
    
    void read(char& value) 
    {
        value = readChar();
    }
    
    int8_t readInt8()
    {
        if (MMA1_UNLIKELY(!buffer_->has_capacity(type_size<int8_t>())))
        {
            pull_and_ensure(sizeof(int8_t));
        }
        
        return buffer_->getByte();
    }
    
    void read(int8_t& value) 
    {
        value = readUInt328();
    }
    
    
    uint8_t readUInt328()
    {
        if (MMA1_UNLIKELY(!buffer_->has_capacity(type_size<uint8_t>())))
        {
            pull_and_ensure(type_size<uint8_t>());
        }
        
        return buffer_->getUByte();
    }
    
    void read(uint8_t& value) 
    {
        value = readUInt328();
    }
    
    
    int16_t readInt16()
    {
        if (MMA1_UNLIKELY(!buffer_->has_capacity(type_size<int16_t>())))
        {
            pull_and_ensure(type_size<int16_t>());
        }
        
        return buffer_->getShort();
    }
    
    void read(int16_t& value) 
    {
        value = readInt16();
    }

    
    uint16_t readUInt3216()
    {
        if (MMA1_UNLIKELY(!buffer_->has_capacity(type_size<uint16_t>())))
        {
            pull_and_ensure(type_size<uint16_t>());
        }
        
        return buffer_->getShort();
    }
    
    void read(uint16_t& value) 
    {
        value = readUInt3216();
    }
    
    
    int32_t readInt32()
    {
        if (MMA1_UNLIKELY(!buffer_->has_capacity(type_size<int32_t>())))
        {
            pull_and_ensure(type_size<int32_t>());
        }
        
        return buffer_->getUInt32();
    }
    
    void read(int32_t& value) 
    {
        value = readInt32();
    }

    
    uint32_t readUInt32()
    {
        if (MMA1_UNLIKELY(!buffer_->has_capacity(type_size<uint32_t>())))
        {
            pull_and_ensure(type_size<uint32_t>());
        }
        
        return buffer_->getUInt32();
    }
    
    void read(uint32_t& value) 
    {
        value = readUInt32();
    }
    
    
    int64_t readInt64()
    {
        if (MMA1_UNLIKELY(!buffer_->has_capacity(type_size<int64_t>())))
        {
            pull_and_ensure(type_size<int64_t>());
        }
        
        return buffer_->getInt64();
    }
    
    void read(int64_t& value) 
    {
        value = readInt64();
    }
    
    
    uint64_t readUInt64()
    {
        if (MMA1_UNLIKELY(!buffer_->has_capacity(type_size<uint64_t>()))) 
        {
            pull_and_ensure(type_size<uint64_t>());
        }
        
        return buffer_->getUInt64();
    }
    
    void read(uint64_t& value) 
    {
        value = readUInt64();
    }
    
    
    float readFloat()
    {
        if (MMA1_UNLIKELY(!buffer_->has_capacity(type_size<float>()))) 
        {
            pull_and_ensure(type_size<float>());
        }
        
        return buffer_->getFloat();
    }
    
    void read(float& value) 
    {
        value = readFloat();
    }

    void read_exactly(uint8_t* data, size_t size)
    {
        size_t len = read(data, size);
        
        if (MMA1_UNLIKELY(len < size)) 
        {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Unexpected end of stream");
        }
    }
    

    size_t read(uint8_t* data, size_t size)
    {
        size_t pos = 0;
        while (pos < size)
        {
            size_t available = buffer_->capacity();
            size_t limit = MMA1_UNLIKELY(pos + available <= size) ? available : (size - pos);
            
            buffer_->get(data + pos, limit);
            
            pos += limit;
            
            if (pos < size && pull() == 0) {
                break;
            }
        }
            
        return pos;
    }

    

    double readDouble()
    {
        if (MMA1_UNLIKELY(!buffer_->has_capacity(type_size<double>()))) 
        {
            pull_and_ensure(type_size<double>());
        }
        
        return buffer_->getDouble();
    }
    
    void read(double& value) 
    {
        value = readDouble();
    }
    
    
    bool readBool()
    {
        if (MMA1_UNLIKELY(!buffer_->has_capacity(type_size<bool>())))
        {
            pull_and_ensure(type_size<bool>());
        }
        
        return buffer_->getBool();
    }
    
    void read(bool& value) 
    {
        value = readBool();
    }
    
    
    size_t pull() 
    {
        buffer_->moveRemainingToStart0();
        size_t len = stream_->read(buffer_->array() + buffer_->limit(), buffer_->size() - buffer_->limit());
        
        buffer_->limit(buffer_->limit() + len);
        
        return len;
    }
    
    void pull_and_ensure(size_t size)
    {
        auto len = pull();
        if (MMA1_UNLIKELY(len < size)) 
        {
            MMA1_THROW(RuntimeException()) << fmt::format_ex(u"Unexpected end of stream. Requested {}, read {}", size, len);
        }
    }
};



class IDataOutputStream {
    using BufferT = DefaultIOBuffer;
    IBinaryOutputStream* stream_;
    BufferT* buffer_;
    std::shared_ptr<Referenceable> holder_;
public:
    IDataOutputStream(IBinaryOutputStream* stream, BufferT* buffer_, std::shared_ptr<Referenceable> holder):
        stream_(stream), buffer_(buffer_), holder_(holder)
    {}
    
    template <typename T>
    static constexpr size_t type_size() 
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return BufferT::ValueSize<T>::value;
    }
    
    IDataOutputStream(const IDataOutputStream&) = delete;
    IDataOutputStream& operator=(const IDataOutputStream&) = delete;
    
    IDataOutputStream(IDataOutputStream&& other): 
        stream_(other.stream_), 
        buffer_(other.buffer_),
        holder_(std::move(other.holder_))
    {
        other.stream_ = nullptr;
        other.buffer_ = nullptr;
    }
    
    IDataOutputStream& operator=(IDataOutputStream&& other)
    {
        if (this != &other) 
        {
            stream_ = other.stream_; 
            buffer_ = other.buffer_;
            holder_ = std::move(other.holder_);
        }
        
        return *this;
    }
    
    void swap(IDataOutputStream& other) 
    {
        std::swap(stream_, other.stream_);
        std::swap(buffer_, other.buffer_);
        std::swap(holder_, other.holder_);
    }
    
    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, void>::type write(T value)
    {
        if (MMA1_UNLIKELEY(buffer_->has_capacity(type_size<T>())))
        {
            flush();
        }
        
        buffer_->put(value);
    }
    
    
    void write(const uint8_t* data, size_t size)
    {
        size_t room_size = buffer_->capacity();
        
        if (MMA1_LIKELY(size <= room_size))
        {
            buffer_->put_(data, size);
        }
        else {
            size_t buffer_size = buffer_->size();
            
            buffer_->put_(data, room_size);
            flush_buffer();
            
            for (size_t pos = room_size; pos < size; ) 
            {
                if (pos + buffer_size <= size)
                {
                    buffer_->put_(data + pos, buffer_size);
                    flush_buffer();
                    
                    pos += buffer_size;
                }
                else {
                    buffer_->put_(data + pos, size - pos);
                    pos = size;
                }
            }
        }
    }
    
    void flush_buffer()
    {
        size_t size = buffer_->pos();
        
        size_t len = stream_->write(buffer_->array(), size);
        
        if (len < size) {
            MMA1_THROW(RuntimeException()) << fmt::format_ex(u"Can't write all data to the stream. Data size {}, written {}", size, len);
        }
        
        buffer_->pos(0);
    }
    
    void flush() 
    {
        flush_buffer();
        stream_->flush();
    }
};

    
}}
