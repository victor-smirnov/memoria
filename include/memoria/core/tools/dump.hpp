
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_TOOLS_DUMP_HPP_
#define MEMORIA_CORE_TOOLS_DUMP_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/types.hpp>

#include <iostream>
#include <functional>

namespace memoria {

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

}

template <typename V>
void dumpArray(std::ostream& out, Int count, function<V(Int)> fn)
{
    Int columns;

    switch (sizeof(V)) {
    case 1: columns = 32; break;
    case 2: columns = 16; break;
    case 4: columns = 16; break;
    default: columns = 8;
    }

    Int width = sizeof(V) * 2 + 1;

    out<<endl;
    Expand(out, 19 + width);
    for (int c = 0; c < columns; c++)
    {
        out.width(width);
        out<<hex<<c;
    }
    out<<dec<<endl;

    for (Int c = 0; c < count; c+= columns)
    {
        Expand(out, 12);
        out<<" ";
        out.width(6);
        out<<dec<<c<<" "<<hex;
        out.width(6);
        out<<c<<": ";

        for (Int d = 0; d < columns && c + d < count; d++)
        {
            out<<hex;
            out.width(width);

            OutputHelepr<V>::out(out, fn(c + d));
        }

        out<<dec<<endl;
    }
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
void dumpArray(std::ostream& out_, const T* data, Int count)
{
    Int columns;

    switch (sizeof(T)) {
    case 1: columns = 32; break;
    case 2: columns = 16; break;
    case 4: columns = 16; break;
    default: columns = 8;
    }

    Int width = sizeof(T) * 2 + 1;

    out_<<endl;
    Expand(out_, 19 + width);
    for (int c = 0; c < columns; c++)
    {
        out_.width(width);
        out_<<hex<<c;
    }
    out_<<endl;

    for (Int c = 0; c < count; c+= columns)
    {
        Expand(out_, 12);
        out_<<" ";
        out_.width(6);
        out_<<dec<<c<<" "<<hex;
        out_.width(6);
        out_<<c<<": ";

        for (Int d = 0; d < columns && c + d < count; d++)
        {
            out_<<hex;
            out_.width(width);

            OutputHelepr<T>::out(out_, data[c + d]);
        }

        out_<<dec<<endl;
    }
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



}


#endif /* DUMP_HPP_ */
