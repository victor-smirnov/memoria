
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/tools/assert.hpp>

namespace memoria {
namespace v1 {
namespace rleseq {


class RLESymbolsRun {
	Int symbol_;
	UBigInt length_;
public:
	constexpr RLESymbolsRun(): symbol_(0), length_(0) {}
	constexpr RLESymbolsRun(Int symbol, UBigInt length): symbol_(symbol), length_(length) {}

	Int symbol() const {return symbol_;}
	UBigInt length() const {return length_;}
};


template <Int Symbols>
static constexpr RLESymbolsRun DecodeRun(UBigInt value)
{
	constexpr UBigInt BitsPerSymbol = NumberOfBits(Symbols - 1);
	constexpr Int SymbolMask = (1 << BitsPerSymbol) - 1;

	return RLESymbolsRun(value & SymbolMask, value >> BitsPerSymbol);
}

template <Int Symbols, Int MaxRunLength>
static constexpr UBigInt EncodeRun(Int symbol, UBigInt length)
{
	constexpr UBigInt BitsPerSymbol = NumberOfBits(Symbols - 1);
	constexpr Int SymbolMask = (1 << BitsPerSymbol) - 1;

	if (length <= MaxRunLength)
	{
		if (length > 0)
		{
			return (symbol & SymbolMask) | (length << BitsPerSymbol);
		}
		else {
			throw Exception(MA_SRC, "Symbols run length must be positive");
		}
	}
	else {
		throw Exception(MA_SRC, SBuf() << "Symbols run length of " << length << " exceeds limit (" << (size_t)MaxRunLength << ")");
	}
}


}}}
