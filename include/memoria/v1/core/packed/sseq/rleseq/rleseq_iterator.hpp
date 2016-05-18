
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

#include <memoria/v1/core/tools/bitmap_select.hpp>
#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/tools/assert.hpp>

namespace memoria {
namespace v1 {

class RLESymbolsRun {
	Int symbol_;
	UBigInt length_;
public:
	constexpr RLESymbolsRun(): symbol_(0), length_(0) {}
	constexpr RLESymbolsRun(Int symbol, UBigInt length): symbol_(symbol), length_(length) {}

	Int symbol() const {return symbol_;}
	UBigInt length() const {return length_;}
};


namespace rleseq {

template <typename Seq>
class RLESeqIterator {
	using Codec = typename Seq::Codec;

	const UByte* symbols_;

	size_t data_pos_;
	size_t data_size_;

	size_t symbol_idx_start_;
	size_t symbol_idx_;

	Codec codec_;

	RLESymbolsRun run_;

	Int run_idx_backup_ 		= 0;
	size_t datapos_backup_ 		= 0;
	size_t symbol_idx_backup_ 	= 0;

public:
	RLESeqIterator(): symbols_(), data_pos_(), data_size_(), symbol_idx_start_(), symbol_idx_() {}
	RLESeqIterator(const UByte* symbols, size_t data_pos, size_t data_size, size_t symbol_idx, RLESymbolsRun run):
		symbols_(symbols), data_pos_(data_pos), data_size_(data_size), symbol_idx_start_(symbol_idx), run_(run)
	{
		symbol_idx_ = run.length();
	}

	bool has_next_run() const {
		return data_pos_ < data_size_;
	}

	bool has_next_symbol_in_run() const {
		return symbol_idx_ < run_.length();
	}

	bool has_next_symbol() const {
		return has_next_symbol_in_run() || has_next_run();
	}

	size_t symbol_idx() const {
		return symbol_idx_ - 1;
	}

	const auto& run() const {
		return run_;
	}

	size_t data_pos() const {return data_pos_;}
	size_t data_size() const {return data_size_;}

	void next_run()
	{
		if (data_pos_ < data_size_)
		{
			symbol_idx_ = symbol_idx_start_;

			UBigInt value = 0;
			auto len = codec_.decode(symbols_, value, data_pos_);

			run_ = Seq::decode_run(value);

			data_pos_ += len;
			symbol_idx_start_ = 0;
		}
		else {
			throw Exception(MA_SRC, SBuf() << "RLE Sequence Iterator is out of symbol runs");
		}
	}

	Int next_symbol_in_run()
	{
		if (symbol_idx_ < run_.length())
		{
			Int sym = run_.symbol();
			symbol_idx_++;

			return sym;
		}
		else {
			throw Exception(MA_SRC, SBuf() << "RLE Sequence Iterator is out of symbols run");
		}
	}

	Int next_symbol()
	{
		if (symbol_idx_ < run_.length())
		{
			Int sym = run_.symbol();
			symbol_idx_++;

			return sym;
		}
		else {
			next_run();
			return next_symbol_in_run();
		}
	}

	void mark()
	{
		symbol_idx_backup_	= symbol_idx_;
		datapos_backup_ 	= data_pos_;
	}

	void restore() {
		symbol_idx_	 = symbol_idx_backup_;
		data_pos_ 	 = datapos_backup_;
	}
};

}}}
