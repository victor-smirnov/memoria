
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

#include "file.hpp"
#include "dma_buffer.hpp"

#include <memoria/v1/core/tools/ptr_cast.hpp>

namespace memoria {
namespace v1 {
namespace reactor {
       
template <typename BufferT = DefaultIOBuffer>    
class BufferedIS: public BinaryInputStream {
    BufferT buffer_;
    std::shared_ptr<File> file_;
    
    uint64_t position_;
    
public:
    BufferedIS(size_t buffer_size, const std::shared_ptr<File>& file, uint64_t start):
        buffer_(buffer_size), file_(file), position_(start)
    {}
    
    virtual size_t read(uint8_t* data, size_t size) 
    {
        size_t len = file_->read(data, position_, size);
        position_ += len;
        return len;
    }
    
    BufferT& buffer() {return buffer_;}
    const BufferT& buffer() const {return buffer_;}
};


template <typename BufferT = DefaultIOBuffer>    
class BufferedOS: public BinaryOutputStream {
    BufferT buffer_;
    std::shared_ptr<File> file_;
    
    uint64_t position_;
    
public:
    BufferedOS(size_t buffer_size, const std::shared_ptr<File>& file, uint64_t start):
        buffer_(buffer_size), file_(file), position_(start)
    {}
    
    virtual size_t write(const uint8_t* data, size_t size) 
    {
        size_t len = file_->write(data, position_, size);
        
        position_ += len;
        
        return len;
    }
    
    virtual void flush() {
        file_->fsync();
    }

    BufferT& buffer() {return buffer_;}
    const BufferT& buffer() const {return buffer_;}
};


/*
template <typename BufferT = DefaultIOBuffer>    
class DmaOS: public BinaryOutputStream {
    BufferT buffer_;
    std::shared_ptr<File> file_;
    
    uint64_t position_;
    
    DMABuffer dma_buffer_;
    
    size_t dma_buffer_size_{512*1024};
    size_t dma_buffer_pos_{};
    
public:
    DmaOS(size_t buffer_size, const std::shared_ptr<File>& file, uint64_t start):
        buffer_(buffer_size), file_(file), position_(start), dma_buffer_{allocate_dma_buffer(512*1024)}
    {}
    
    virtual size_t write(const uint8_t* data, size_t size) 
    {
        size_t room_size = dma_buffer_size_ - dma_buffer_pos_;
        
        if (MMA1_LIKELY(size <= room_size))
        {
            std::memcpy(dma_buffer_.get() + dma_buffer_pos_, data, size);
            dma_buffer_pos_ += size;
        }
        else if (room_size == 0) 
        {
            flush_dma(dma_buffer_size_);
            return write(data, size);
        }
        else {
            std::memcpy(dma_buffer_.get() + dma_buffer_pos_, data, room_size);
            flush_dma(dma_buffer_size_);
            
            for (size_t pos = room_size; pos < size; ) 
            {
                if (pos + dma_buffer_size_ <= size)
                {
                    std::memcpy(dma_buffer_.get(), data, dma_buffer_size_);
                    flush_dma(dma_buffer_size_);
                    
                    pos += dma_buffer_size_;
                }
                else {
                    std::memcpy(dma_buffer_.get(), data, dma_buffer_size_ - dma_buffer_pos_);
                    pos = size;
                }
            }
        }
        
        return size;
    }
    
    void flush_dma(size_t size = 0) 
    {
        size_t len = file_->write(dma_buffer_.get(), position_, size);
        
        std::memset(dma_buffer_.get(), 0, dma_buffer_size_);
        
        if (len < size) 
        {
            tools::rise_error(SBuf() << "Write error: " << size << " " << len);
        }
        
        position_ += size;
        dma_buffer_pos_ = 0;
    }
    
    virtual void flush() {
        flush_dma(align_up(dma_buffer_pos_));
    }
    
    size_t align_up(size_t size) 
    {
        size_t alignment = file_->alignment();
        
        if (size % alignment == 0) 
        {
            return size;
        }
        else {
            return (size / alignment + 1) * alignment;
        }
    }

    BufferT& buffer() {return buffer_;}
    const BufferT& buffer() const {return buffer_;}
};





template <typename BufferT = DefaultIOBuffer>    
class DmaIS: public BinaryInputStream {
    BufferT buffer_;
    std::shared_ptr<File> file_;
    
    uint64_t position_;
    
    DMABuffer dma_buffer_;
    
    size_t dma_buffer_size_;
    size_t dma_buffer_limit_{};
    size_t dma_buffer_pos_{};
    
public:
    DmaIS(size_t buffer_size, const std::shared_ptr<File>& file, uint64_t start, size_t dma_buf_size = 512*1024):
        buffer_(buffer_size), 
        file_(file), 
        position_(start), 
        dma_buffer_(allocate_dma_buffer(dma_buf_size)),
        dma_buffer_size_(dma_buf_size)
    {
        pull();
    }


    virtual size_t read(uint8_t* data, size_t size)
    {
        size_t availavle = dma_buffer_limit_ - dma_buffer_pos_;
        
        if (MMA1_LIKELY(size <= availavle)) 
        {
            std::memcpy(data, dma_buffer_.get() + dma_buffer_pos_, size);
            dma_buffer_pos_ += size;
            
            return size;
        }
        else {
            pull();
            
            availavle = dma_buffer_limit_ - dma_buffer_pos_;
            if (MMA1_LIKELY(size <= availavle))
            {
                std::memcpy(data, dma_buffer_.get() + dma_buffer_pos_, size);
                dma_buffer_pos_ += size;
            
                return size;
            }
            else if (MMA1_LIKELY(dma_buffer_limit_ == dma_buffer_size_))
            {
                for (size_t pos = 0; pos < size;)
                {
                    if (pos + dma_buffer_size_ <= size) 
                    {
                        std::memcpy(data, dma_buffer_.get() + dma_buffer_pos_, dma_buffer_size_ - dma_buffer_limit_);
                    }
                    
                    size_t limit = MMA1_UNLIKELY(pos + dma_buffer_size_ <= size) ? dma_buffer_size_ : (size - pos);
                    
                    std::memcpy(data, dma_buffer_.get() + dma_buffer_pos_, size);
                    
                    pull();
                    
                    pos += limit;
                    
                    if (MMA1_UNLIKELY(buffer_->limit() < limit)) {
                        tools::rise_error(SBuf() << "Premature end of stream");
                    }
                }
            }
            else {
                tools::rise_error(SBuf() << "Premature end of stream");
            }
        }
        
        
        size_t len = file_->read(data, position_, size);
        position_ += len;
        return len;
    }
    
    size_t pull() 
    {
        size_t pos = dma_buffer_pos_ / file_->alignment();
        size_t to_move = dma_buffer_limit_ - pos;
        
        std::memmove(dma_buffer_.get(), dma_buffer_.get() + pos, dma_buffer_size_ - pos);
        
        dma_buffer_pos_ -= pos;
        dma_buffer_limit_ -= pos;
        
        size_t len = file_->read(dma_buffer_.get() + dma_buffer_limit_, position_, dma_buffer_size_ - dma_buffer_limit_);
        
        //FIXME may be a problem if returned read length is not a multiple of alignment blocks
        dma_buffer_limit_ += len;
        position_ += len;
        
        return len;
    }
    
    void pull_and_ensure(size_t size)
    {
        auto len = pull();
        if (MMA1_UNLIKELY(len < size)) 
        {
            tools::rise_error(SBuf() << "Premature end of stream. Requested " << size << ", read " << len);
        }
    }
    
    
    BufferT& buffer() {return buffer_;}
    const BufferT& buffer() const {return buffer_;}
};
*/





}}}

