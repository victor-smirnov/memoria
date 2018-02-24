
// Copyright 2018 Victor Smirnov
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

#include <memoria/v1/core/tools/iostreams.hpp>

#include <iterator>

namespace memoria {
namespace v1 {
namespace reactor {


class ConsoleInputStream: public IBinaryInputStream {
    int32_t fd_;
public:
    ConsoleInputStream(int32_t fd) noexcept : fd_(fd) {}

    virtual ~ConsoleInputStream() noexcept {}
    virtual size_t read(uint8_t* data, size_t size);
    virtual void close() {}
    virtual bool is_closed() const {return false;}
};


class ConsoleOutputStream: public IBinaryOutputStream {
    int32_t fd_;
public:
    ConsoleOutputStream(int32_t fd) noexcept : fd_(fd){}

    virtual ~ConsoleOutputStream() noexcept {}
    virtual size_t write(const uint8_t* data, size_t size);
    virtual void flush();
    virtual void close() {}
    virtual bool is_closed() const {return false;}
};

template<class CharT = char>
class BinaryOStreamBuf;


template <>
class BinaryOStreamBuf<char>: public std::basic_streambuf<char> {

    using CharT         = char;

    using Base          = std::basic_streambuf<CharT>;
    using char_type     = typename Base::char_type;
    using int_type      = typename Base::int_type;
    using Traits        = typename Base::traits_type;

    size_t size_;
    UniquePtr<CharT> buffer_;

    IBinaryOutputStream* output_stream_{};

public:
    BinaryOStreamBuf(IBinaryOutputStream* output_stream, size_t size = 1024):
        size_(size),
        buffer_(allocate_system_zeroed<CharT>(size)),
        output_stream_(output_stream)
    {
        Base::setp(buffer_.get(), buffer_.get() + size_);
    }

protected:
    int_type overflow(int_type ch)
    {
        sync();

        if (ch != Traits::eof())
        {
            *buffer_.get() = ch;
            Base::pbump(1);
        }

        return ch;
    }

    int sync()
    {
        auto size = std::distance(Base::pbase(), Base::pptr());
        output_stream_->write(T2T<uint8_t*>(buffer_.get()), size);

        Base::pbump(-(int)size);

        return 0;
    }
};


}}}
