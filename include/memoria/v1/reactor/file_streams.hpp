
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

#include <exception>

namespace memoria {
namespace v1 {
namespace reactor {
       
template <typename BufferT = DefaultIOBuffer>    
class BufferedIS: public IBinaryInputStream {
    BufferT buffer_;
    File file_;
    
    uint64_t position_;
    
public:
    BufferedIS(size_t buffer_size, const File& file, uint64_t start):
        buffer_(buffer_size), file_(file), position_(start)
    {}
    
    virtual ssize_t read(uint8_t* data, size_t size) 
    {
        ssize_t len = file_.read(data, position_, size);
        position_ += len;
        return len;
    }
    
    BufferT& buffer() {return buffer_;}
    const BufferT& buffer() const {return buffer_;}
};


template <typename BufferT = DefaultIOBuffer>    
class BufferedOS: public IBinaryOutputStream {
    BufferT buffer_;
    File file_;
    
    uint64_t position_;
    
public:
    BufferedOS(size_t buffer_size, const File& file, uint64_t start):
        buffer_(buffer_size), file_(file), position_(start)
    {}
    
    virtual ssize_t write(const uint8_t* data, size_t size) 
    {
        ssize_t len = file_.write(data, position_, size);
        
        position_ += len;
        
        return len;
    }
    
    virtual void flush() {
        file_.fsync();
    }

    BufferT& buffer() {return buffer_;}
    const BufferT& buffer() const {return buffer_;}
};

enum class FilePos {BEGIN, END};

template <typename Char = char> 
class FileStrembuf: public std::basic_streambuf<Char> {
    using Base = std::basic_streambuf<Char>;
    
    File file_; 
    std::unique_ptr<Char[]> buffer_;
    size_t buffer_size_;
    
    uint64_t processed_;
    bool flushed_{false};
    
public:
    using typename Base::int_type;
    using typename Base::traits_type;
    
    FileStrembuf(File file, FilePos pos = FilePos::BEGIN, size_t buffer_size = 4096): 
        file_(file),
        buffer_(std::make_unique<Char[]>(buffer_size)),
        buffer_size_(buffer_size)
    {
        reset();
        
        if (pos == FilePos::BEGIN) {
            processed_ = 0;
        }
        else {
            processed_ = file_.size();
        }
    }
    
    ~FileStrembuf() noexcept {
        try {
            bflush();
        }
        catch(std::exception& ex) {
            std::cerr << "Exception flushing file stream buffer :" << ex.what() << std::endl; 
        }
        catch(...) {
            std::cerr << "Unknown exception flushing file stream buffer" << std::endl; 
        }
    }
    
    virtual int_type
    overflow(int_type c = traits_type::eof())
    { 
        *this->pptr() = c;
        
        bflush(1);
        
        return traits_type::not_eof(c); 
    }
    
    void fsync() {
        sync();
        flush();
        return file_.fdsync();
    }
    
    virtual int sync() {
        flushed_ = false;
        return 0;
    }
    
    void flush() {
        bflush();
    }
        
private:
    void bflush(size_t add = 0) 
    {
        ptrdiff_t size = (this->pptr() - this->pbase()) + add;
        
        if (size > 0) 
        {
            processed_ += file_.write(tools::ptr_cast<uint8_t>(this->pbase()), processed_, size * sizeof(Char));
            reset();
        }
    }
    
    void reset() {
        Base::setp(buffer_.get(), buffer_.get() + buffer_size_);
    }
};


template <typename Char = char> 
class BufferedFileOStream: public std::basic_ostream<Char> {
    using Base = std::basic_ostream<Char>;

    FileStrembuf<Char> buffer_;
public:
    BufferedFileOStream(File file, FilePos pos = FilePos::BEGIN, size_t buffer_size = 4096): 
		Base(nullptr),
        buffer_(file, pos, buffer_size)
    {
        this->init(&buffer_);
    }
    
    void flush() {
        buffer_.flush();
    }
    
    void fsync() {
        buffer_.fsync();
    }
};

using bfstream = BufferedFileOStream<char>;



}}}

