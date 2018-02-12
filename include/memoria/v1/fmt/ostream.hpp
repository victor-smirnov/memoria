/*
 Formatting library for C++ - std::ostream support

 Copyright (c) 2012 - 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#pragma once

#include <memoria/v1/fmt/format.hpp>
#include <ostream>

namespace memoria {
namespace v1 {
namespace fmt {

namespace _ {

template <class Char>
class FormatBuf : public std::basic_streambuf<Char> {
private:
    typedef typename std::basic_streambuf<Char>::int_type int_type;
    typedef typename std::basic_streambuf<Char>::traits_type traits_type;

    Buffer<Char> &buffer_;

public:
    FormatBuf(Buffer<Char> &buffer) : buffer_(buffer) {}

protected:
    // The put-area is actually always empty. This makes the implementation
    // simpler and has the advantage that the streambuf and the buffer are always
    // in sync and sputc never writes into uninitialized memory. The obvious
    // disadvantage is that each call to sputc always results in a (virtual) call
    // to overflow. There is no disadvantage here for sputn since this always
    // results in a call to xsputn.

    int_type overflow(int_type ch = traits_type::eof()) override
    {
        if (!traits_type::eq_int_type(ch, traits_type::eof()))
            buffer_.push_back(static_cast<Char>(ch));
        return ch;
    }

    std::streamsize xsputn(const Char *s, std::streamsize count) override
    {
        buffer_.append(s, s + count);
        return count;
    }
};

Yes &convert(std::ostream &);

struct DummyStream : std::ostream {
    DummyStream();  // Suppress a bogus warning in MSVC.
    // Hide all operator<< overloads from std::ostream.
    void operator<<(_::Null<>);
};

No &operator<<(std::ostream &, int);

template<typename T>
struct ConvertToIntImpl<T, true>
{
    // Convert to int only if T doesn't have an overloaded operator<<.
    enum {
        value = sizeof(convert(get<DummyStream>() << get<T>())) == sizeof(No)
    };
};

// Write the content of w to os.
MMA1_FMT_API void write(std::ostream &os, Writer &w);
}  // namespace _

// Formats a value.
template <typename Char, typename ArgFormatter_, typename T>
void format_arg(BasicFormatter<Char, ArgFormatter_> &f,
                const Char *&format_str,
                const T &value)
{
    _::MemoryBuffer<Char, _::INLINE_BUFFER_SIZE> buffer;

    _::FormatBuf<Char> format_buf(buffer);
    std::basic_ostream<Char> output(&format_buf);
    output << value;

    BasicStringRef<Char> str(&buffer[0], buffer.size());
    using MakeArg = _::MakeArg< BasicFormatter<Char>>;
    format_str = f.format(format_str, MakeArg(str));
}

/**
  \rst
  Prints formatted data to the stream *os*.

  **Example**::

    print(cerr, "Don't {}!", "panic");
  \endrst
 */
MMA1_FMT_API void print(std::ostream &os, CStringRef format_str, ArgList args);
MMA1_FMT_VARIADIC(void, print, std::ostream &, CStringRef)
}  // namespace fmt
}}

