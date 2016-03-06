
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

	template <typename V>
	size_t max_width(Int count, bool hex, function<V(Int)> fn)
	{
		if (sizeof(V) == 1) {
			return 2;
		}
		else if (sizeof(V) == 2)
		{
			return 4;
		}
		else
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
	}
}

template <typename V>
void dumpArray(std::ostream& out, Int count, function<V (Int)> fn)
{
	bool is_hex = std::is_same<V, UByte>::value || std::is_same<V, Byte>::value || std::is_same<V, Char>::value;

	auto width = max_width(count, is_hex, fn) + 1;

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
        out.flush();
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

        out << (is_hex ? hex : dec);

        for (Int d = 0; d < columns && c + d < count; d++)
        {
            out.width(width);

            stringstream ss;

            OutputHelepr<V>::out(ss, fn(c + d));

            out<<ss.str();
        }

        out << dec << endl;
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
