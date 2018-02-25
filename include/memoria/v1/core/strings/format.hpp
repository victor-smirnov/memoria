
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/types/type2type.hpp>
#include <memoria/v1/core/strings/string.hpp>
#include <memoria/v1/core/exceptions/core.hpp>

#include <string>
#include <sstream>
#include <memory>
#include <ostream>
#include <functional>


namespace memoria {
namespace v1 {
namespace fmt {


namespace _ {



template <typename T, typename Alloc = std::allocator<T>>
class IndexableBuffer {
    T* array_;
    size_t size_;
    size_t max_size_;
    Alloc alloc_;
public:
    IndexableBuffer(size_t capacity, Alloc&& alloc = Alloc()):
        array_(nullptr), size_(0), max_size_(capacity),
        alloc_(std::move(alloc))
    {
        array_ = alloc_.allocate(capacity);
    }

    IndexableBuffer(IndexableBuffer&&) = delete;
    IndexableBuffer(const IndexableBuffer&) = delete;

    ~IndexableBuffer() noexcept
    {
        alloc_.deallocate(array_, max_size_);
    }

    T& operator[](size_t idx) {return array_[idx];}
    const T& operator[](size_t idx) const {return array_[idx];}

    size_t size() const {return size_;}
    size_t capacity() const {return max_size_ - size_;}

    void push_back(const T& item)
    {
        ensure_capacity(1);

        new (array_ + size_) T(item);
        size_++;
    }

    void push_back(T&& item)
    {
        ensure_capacity(1);

        new (array_ + size_) T(std::move(item));
        size_++;
    }

    template <typename... Args>
    void emplace_back(Args&&... args)
    {
        ensure_capacity(1);

        new (array_ + size_) T(std::forward(args)...);
        size_++;
    }

private:
    void ensure_capacity(size_t n)
    {
        if (size_ + n <= max_size_)
        {
            return;
        }
        else {
            size_t new_max_size = max_size_ * 2;
            T* ptr = alloc_.allocate(new_max_size);

            for (size_t c = 0; c < size_; c++)
            {
                new (ptr + c) T(std::move(array_[c]));
                (array_ + c)->~T();
            }

            std::swap(array_, ptr);

            alloc_.deallocate(ptr, max_size_);
            max_size_ = new_max_size;
        }
    }
};



template <typename T>
class InplaceAllocator: protected std::allocator<T> {

    using Base = std::allocator<T>;

    char* inplace_buffer_;
    size_t buffer_size_;

    size_t allocs_{};

public:
    InplaceAllocator(char* buffer, size_t buffer_size):
        inplace_buffer_(buffer), buffer_size_(buffer_size)
    {}

    T* allocate(size_t n, const T* hint = 0 )
    {
        allocs_++;
        if (allocs_ == 1) {
            if (n <= buffer_size_) {
                return static_cast<T*>(inplace_buffer_);
            }
            else {
                return Base::allocate(n, hint);
            }
        }
        else {
            return Base::allocate(n, hint);
        }
    }

    void deallocate( T* p, std::size_t n )
    {
        if (p != inplace_buffer_)
        {
            Base::deallocate(p, n);
        }
    }

};


}



class FormatSpecifier {
    size_t fmt_num_;
    const char16_t* text_;
    size_t length_;
public:
    FormatSpecifier(size_t fmt_num, const char16_t* text, size_t length):
        fmt_num_(fmt_num), text_(text), length_(length)
    {}

    size_t fmt_num() const {return fmt_num_;}
    const char16_t* text() const {return text_;}
    size_t length() const {return length_;}
};


template <typename TextFn, typename FormatFn>
void parseFormat(const char16_t* format, TextFn&& text_handler, FormatFn&& format_handler)
{
    auto parse_format_specifier = [](size_t fmt_num, const char16_t* format_str, size_t& pos)
    {
        pos++;
        size_t start = pos;
        while (*(format_str + pos) != 0)
        {
            if (*(format_str + pos) == u'}')
            {
                return FormatSpecifier(fmt_num, format_str + start, (pos++) - start);
            }

            pos++;
        }

        throw std::runtime_error((SBuf() << "Unclosed format specifier in format string: " << U16String(format_str)).str());
    };

    size_t pos{};
    bool masked = false;
    size_t fmt_num {};
    size_t last_pos {};

    while (*(format + pos) != 0)
    {
        auto ch = *(format + pos);
        switch (ch)
        {
        case u'\\':
        {
            char16_t next_ch = *(format + pos + 1);
            if (next_ch == 0)
            {
                if (pos - last_pos) {
                    text_handler(format + last_pos, pos - last_pos + 1);
                }
                return;
            }
            else if (next_ch == u'{')
            {
                if (pos - last_pos > 0) {
                    text_handler(format + last_pos, pos - last_pos);
                }

                last_pos = pos + 1;
                masked = true;
            }

            pos++;

            break;
        }

        case u'{':
            if (!masked)
            {
                if (pos - last_pos > 0) {
                    text_handler(format + last_pos, pos - last_pos);
                }

                FormatSpecifier fmt = parse_format_specifier(fmt_num++, format, pos);
                format_handler(std::move(fmt));

                last_pos = pos;
                masked = false;
            }
            else {
                pos++;
            }

            break;
        default: pos += 1;
        }
    }

    if (pos - last_pos) {
        text_handler(format + last_pos, pos - last_pos);
    }
}


struct CodeunitConsumer {
    virtual ~CodeunitConsumer() noexcept {}
    virtual void consume(const char16_t* code_units, size_t size) = 0;
};

class U16StringCodeunitConsumer: public CodeunitConsumer {
    U16String buffer_;
public:
    U16StringCodeunitConsumer() {}

    virtual void consume(const char16_t* code_units, size_t size)
    {
        buffer_.to_std_string().append(code_units, 0, size);
    }

    U16String& buffer() {return buffer_;}
    const U16String& buffer() const {return buffer_;}
};


template <typename... Args>
U16String format(const char16_t* format, Args&&... args)
{
    U16StringCodeunitConsumer consumer;

    constexpr size_t args_size = sizeof...(Args);

    using ArgFn = std::function<void (const FormatSpecifier&)>;

    ArgFn handlers[] = {
        ArgFn([&](const FormatSpecifier& fmt){
            auto str = U8String((SBuf() << args).str()).to_u16();
            consumer.consume(str.data(), str.length());
        }) ...
    };

    parseFormat(
        format,
        [&](const char16_t* text, size_t length) {
            consumer.consume(text, length);
        },
        [&](const FormatSpecifier& fmt) {
            if (fmt.fmt_num() < args_size) {
                handlers[fmt.fmt_num()](fmt);
            }
            else {
                consumer.consume(u"{", 1);
                consumer.consume(fmt.text(), fmt.length());
                consumer.consume(u"}", 1);
            }
        }
    );

    return std::move(consumer.buffer());
}


template <typename... Args>
U8String format8(const char16_t* fmt, Args&&... args) {
    return format(fmt, std::forward<Args>(args)...).to_u8();
}

template <typename... Args>
WhatInfo format_ex(const char16_t* fmt, Args&&... args) {
    return WhatInfo(format8(fmt, std::forward<Args>(args)...));
}


}}}
