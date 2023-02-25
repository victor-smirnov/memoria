
// Copyright 2013-2022 Victor Smirnov
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

#include <memoria/core/types.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <iostream>
#include <functional>
#include <string>

namespace memoria {

void Expand(std::ostream& os, size_t level);

namespace detail {

    template <typename T>
    struct OutputHelepr {
        static std::ostream& out(std::ostream& o, const T& value)
        {
            o << value;
            return o;
        }
    };

#ifdef MMA_HAS_INT128
    template <>
    struct OutputHelepr<UInt128T> {
        static std::ostream& out(std::ostream& o, const UInt128T& value)
        {
            uint64_t v0 = value;
            o << v0;
            return o;
        }
    };

    template <>
    struct OutputHelepr<Int128T> {
        static std::ostream& out(std::ostream& o, const Int128T& value)
        {
            o << static_cast<int64_t>(value);
            return o;
        }
    };
#endif

    template <>
    struct OutputHelepr<int8_t> {
        static std::ostream& out(std::ostream& o, const int8_t& value)
        {
            o << (size_t)(uint8_t)value;
            return o;
        }
    };

    template <>
    struct OutputHelepr<uint8_t> {
        static std::ostream& out(std::ostream& o, const uint8_t& value)
        {
            o << (size_t)value;
            return o;
        }
    };

    template <typename V>
    size_t max_width(size_t count, bool hex, std::function<V(size_t)> fn)
    {
        size_t max = 0;

        for (size_t c = 0; c < count; c++)
        {
            V v = fn(c);

            auto str = toString(v, hex);

            auto len = str.length();

            if (len > max)
            {
                max = len;
            }
        }

        return max;
    }

    template <typename T>
    T mask_controls(T ch) {
        return (ch >= 32 && ch < 127) ? ch : (T)32;
    }

    template <typename T>
    inline void dump_as_char(std::ostream& out, const T& val) {}


    inline void dump_as_char(std::ostream& out, const uint8_t& val) {
        out << mask_controls(val);
    }


    inline void dump_as_char(std::ostream& out, const int8_t& val) {
        out << mask_controls(val);
    }


    inline void dump_as_char(std::ostream& out, const char& val) {
        out << mask_controls(val);
    }
}

template <typename V>
void dumpArray(std::ostream& out, size_t count, std::function<V (size_t)> fn)
{
    bool is_char = std::is_same<V, uint8_t>::value || std::is_same<V, int8_t>::value || std::is_same<V, char>::value;

    auto width = is_char ? 3 : 5;//max_width(count, is_char, fn) + 1;

    if (width < 2) width = 2;

    size_t columns;

    switch (sizeof(V))
    {
        case 1: columns = 16; break;
        case 2: columns = 8; break;
        default: columns = (80 / width > 0 ? 80 / width : 1);
    }

    out << std::endl;
    Expand(out, 28);
    for (int c = 0; c < columns; c++)
    {
        out.width(width);
        out << c;
    }

    if (is_char)
    {
        out.width(16);
        out << "";

        for (int c = 0; c < columns; c++)
        {
            out.width(1);
            out << c % 10;
        }
    }

    out << std::dec << std::endl;

    for (size_t c = 0; c < count; c+= columns)
    {
        Expand(out, 12);
        out << " ";
        out.width(6);
        out << std::dec << c << " " << std::hex;
        out.width(6);
        out << c << ": ";

        size_t d;
        for (d = 0; d < columns && c + d < count; d++)
        {
            std::stringstream ss;

            ss << (is_char ? std::hex : std::dec);

            detail::OutputHelepr<V>::out(ss, fn(c + d));

            out.width(width);
            out<<ss.str();
        }

        out << std::dec;

        if (is_char)
        {
            for (; d < columns; d++)
            {
                out.width(width);
                out << "";
            }

            out.width(16);
            out << "";

            for (size_t d = 0; d < columns && c + d < count; d++)
            {
                out.width(1);

                detail::dump_as_char(out, fn(c + d));
            }
        }

        out << std::endl;
    }
}





template <typename V>
void dumpVector(std::ostream& out, const std::vector<V>& data)
{
    dumpArray<V>(out, data.size(), [&](size_t idx) {return data[idx];});
}


template <typename V>
void dumpSymbols(std::ostream& out_, size_t size_, size_t bits_per_symbol, std::function<V(size_t)> fn)
{
    size_t columns;

    switch (bits_per_symbol)
    {
    case 1: columns = 100; break;
    case 2: columns = 100; break;
    case 4: columns = 100; break;
    default: columns = 50;
    }

    size_t width = bits_per_symbol <= 4 ? 1 : 3;

    size_t c = 0;

    do
    {
        out_ << std::endl;
        Expand(out_, 31 - width * 5 - (bits_per_symbol <= 4 ? 2 : 0));
        for (int c = 0; c < columns; c += 5)
        {
            out_.width(width*5);
            out_ << std::dec << c;
        }
        out_ << std::endl;

        size_t rows = 0;
        for (; c < size_ && rows < 10; c += columns, rows++)
        {
            Expand(out_, 12);
            out_ << " ";
            out_.width(6);
            out_ << std::dec << c << " " << std::hex;
            out_.width(6);
            out_ << c << ": ";

            for (size_t d = 0; d < columns && c + d < size_; d++)
            {
                out_ << std::hex;
                out_.width(width);
                detail::OutputHelepr<V>::out(out_, fn(c + d));
            }

            out_ << std::dec << std::endl;
        }
    } while (c < size_);
}



template <typename T>
void dumpSymbols(std::ostream& out_, T* symbols, size_t size_, size_t bits_per_symbol)
{
    size_t columns;

    switch (bits_per_symbol)
    {
        case 1: columns = 100; break;
        case 2: columns = 100; break;
        case 4: columns = 100; break;
        default: columns = 50;
    }

    size_t width = bits_per_symbol <= 4 ? 1 : 3;

    size_t c = 0;

    do
    {
        out_ << std::endl;
        Expand(out_, 31 - width * 5 - (bits_per_symbol <= 4 ? 2 : 0));
        for (int c = 0; c < columns; c += 5)
        {
            out_.width(width*5);
            out_ << std::dec << c;
        }
        out_ << std::endl;

        size_t rows = 0;
        for (; c < size_ && rows < 10; c += columns, rows++)
        {
            Expand(out_, 12);
            out_ << " ";
            out_.width(6);
            out_ << std::dec << c << " " << std::hex;
            out_.width(6);
            out_<<c<<": ";

            for (size_t d = 0; d < columns && c + d < size_; d++)
            {
                out_ << std::hex;
                out_.width(width);

                size_t idx = (c + d) * bits_per_symbol;

                if (sizeof(T) > 1) {
                    out_ << GetBits(symbols, idx, bits_per_symbol);
                }
                else {
                    out_ << (size_t)GetBits(symbols, idx, bits_per_symbol);
                }
            }

            out_ << std::dec << std::endl;
        }
    } while (c < size_);
}



}
