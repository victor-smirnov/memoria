
// Copyright 2013 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/types/types.hpp>

#include <iostream>
#include <functional>

namespace memoria {
namespace v1 {

void Expand(std::ostream& os, Int level);

namespace {

    template <typename T>
    struct OutputHelepr {
        static std::ostream& out(std::ostream& o, const T& value)
        {
            o<<value;
            return o;
        }
    };

    template <>
    struct OutputHelepr<Byte> {
        static std::ostream& out(std::ostream& o, const Byte& value)
        {
            o<<(Int)(UByte)value;
            return o;
        }
    };

    template <>
    struct OutputHelepr<UByte> {
        static std::ostream& out(std::ostream& o, const UByte& value)
        {
            o<<(Int)value;
            return o;
        }
    };

    template <typename V>
    size_t max_width(Int count, bool hex, function<V(Int)> fn)
    {
        size_t max = 0;

        for (Int c = 0; c < count; c++)
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


    inline void dump_as_char(std::ostream& out, const UByte& val) {
        out << mask_controls(val);
    }


    inline void dump_as_char(std::ostream& out, const Byte& val) {
        out << mask_controls(val);
    }


    inline void dump_as_char(std::ostream& out, const Char& val) {
        out << mask_controls(val);
    }
}

template <typename V>
void dumpArray(std::ostream& out, Int count, function<V (Int)> fn)
{
    bool is_char = std::is_same<V, UByte>::value || std::is_same<V, Byte>::value || std::is_same<V, Char>::value;

    auto width = max_width(count, is_char, fn) + 1;

    if (width < 3) width = 3;

    Int columns;

    switch (sizeof(V))
    {
        case 1: columns = 32; break;
        case 2: columns = 16; break;
        default: columns = (80 / width > 0 ? 80 / width : 1);
    }

    out << endl;
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

    out << dec << endl;

    for (Int c = 0; c < count; c+= columns)
    {
        Expand(out, 12);
        out << " ";
        out.width(6);
        out << dec << c << " " << hex;
        out.width(6);
        out << c << ": ";

        Int d;
        for (d = 0; d < columns && c + d < count; d++)
        {
            stringstream ss;

            ss << (is_char ? hex : dec);

            OutputHelepr<V>::out(ss, fn(c + d));

            out.width(width);
            out<<ss.str();
        }

        out << dec;

        if (is_char)
        {
            for (; d < columns; d++)
            {
                out.width(width);
                out << "";
            }

            out.width(16);
            out << "";

            for (Int d = 0; d < columns && c + d < count; d++)
            {
                out.width(1);

                dump_as_char(out, fn(c + d));
            }
        }

        out << endl;
    }
}

template <typename V>
void dumpVector(std::ostream& out, const std::vector<V>& data)
{
    dumpArray<V>(out, data.size(), [&](Int idx) {return data[idx];});
}


template <typename V>
void dumpSymbols(ostream& out_, Int size_, Int bits_per_symbol, function<V(Int)> fn)
{
    Int columns;

    switch (bits_per_symbol)
    {
    case 1: columns = 100; break;
    case 2: columns = 100; break;
    case 4: columns = 100; break;
    default: columns = 50;
    }

    Int width = bits_per_symbol <= 4 ? 1 : 3;

    Int c = 0;

    do
    {
        out_<<endl;
        Expand(out_, 31 - width * 5 - (bits_per_symbol <= 4 ? 2 : 0));
        for (int c = 0; c < columns; c += 5)
        {
            out_.width(width*5);
            out_<<dec<<c;
        }
        out_<<endl;

        Int rows = 0;
        for (; c < size_ && rows < 10; c += columns, rows++)
        {
            Expand(out_, 12);
            out_<<" ";
            out_.width(6);
            out_<<dec<<c<<" "<<hex;
            out_.width(6);
            out_<<c<<": ";

            for (Int d = 0; d < columns && c + d < size_; d++)
            {
                out_<<hex;
                out_.width(width);
                OutputHelepr<V>::out(out_, fn(c + d));
            }

            out_<<dec<<endl;
        }
    } while (c < size_);
}



template <typename T>
void dumpSymbols(ostream& out_, T* symbols, Int size_, Int bits_per_symbol)
{
    Int columns;

    switch (bits_per_symbol)
    {
        case 1: columns = 100; break;
        case 2: columns = 100; break;
        case 4: columns = 100; break;
        default: columns = 50;
    }

    Int width = bits_per_symbol <= 4 ? 1 : 3;

    Int c = 0;

    do
    {
        out_<<endl;
        Expand(out_, 31 - width * 5 - (bits_per_symbol <= 4 ? 2 : 0));
        for (int c = 0; c < columns; c += 5)
        {
            out_.width(width*5);
            out_<<dec<<c;
        }
        out_<<endl;

        Int rows = 0;
        for (; c < size_ && rows < 10; c += columns, rows++)
        {
            Expand(out_, 12);
            out_<<" ";
            out_.width(6);
            out_<<dec<<c<<" "<<hex;
            out_.width(6);
            out_<<c<<": ";

            for (Int d = 0; d < columns && c + d < size_; d++)
            {
                out_<<hex;
                out_.width(width);

                Int idx = (c + d) * bits_per_symbol;

                if (sizeof(T) > 1) {
                    out_<<GetBits(symbols, idx, bits_per_symbol);
                }
                else {
                    out_<<(Int)GetBits(symbols, idx, bits_per_symbol);
                }
            }

            out_<<dec<<endl;
        }
    } while (c < size_);
}



}}