
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

#include <memoria/core/tools/iostreams.hpp>
#include <memoria/reactor/file.hpp>
#include <memoria/core/types/type2type.hpp>

#include <memoria/core/memory/malloc.hpp>

#include <iterator>

namespace memoria {
namespace reactor {

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
            *buffer_.get() = Traits::to_char_type(ch);
            Base::pbump(1);
        }

        return ch;
    }

    int sync()
    {
        auto size = std::distance(Base::pbase(), Base::pptr());
        if (size > 0)
        {
            size_t written = output_stream_->write(T2T<uint8_t*>(buffer_.get()), size);
            Base::pbump(-(int32_t)written);
        }

        return 0;
    }
};


template<class CharT = char>
class BinaryIStreamBuf;

template <>
class BinaryIStreamBuf<char>: public std::basic_streambuf<char> {

    using CharT         = char;

    using Base          = std::basic_streambuf<CharT>;
    using char_type     = typename Base::char_type;
    using int_type      = typename Base::int_type;
    using Traits        = typename Base::traits_type;

    size_t size_;
    UniquePtr<CharT> buffer_;
    IBinaryInputStream* input_stream_{};

public:
    BinaryIStreamBuf(IBinaryInputStream* input_stream, size_t size = 1024):
        size_(size),
        buffer_(allocate_system_zeroed<CharT>(size)),
        input_stream_(input_stream)
    {
        Base::setg(buffer_.get(), buffer_.get() + size_, buffer_.get() + size_);
    }

protected:
    int_type underflow()
    {
        size_t read = input_stream_->read(T2T<uint8_t*>(buffer_.get()), size_);

        if (read != 0)
        {
            setg(buffer_.get(), buffer_.get(), buffer_.get() + read);
            return Traits::to_int_type(*gptr());
        }
        else {
            return Traits::eof();
        }
    }
};



}}
