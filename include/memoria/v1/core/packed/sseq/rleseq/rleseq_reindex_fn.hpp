
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
namespace rleseq {

namespace {

template <typename Seq>
	class SymbolsIteratorBase {
	protected:
		using Codec 	= typename Seq::Codec;
		using Symbols 	= typename Codec::BufferType;

		static constexpr size_t ValuesPerBlock = Seq::ValuesPerBranch;

		Codec codec_;
		const Symbols* symbols_;
		size_t data_size_;

		size_t blocks_;

		size_t data_pos_ = 0;
		size_t block_ = 0;
		size_t limit_;



	public:
		SymbolsIteratorBase(const Symbols* symbols, size_t data_size, size_t blocks):
			symbols_(symbols), data_size_(data_size), blocks_(blocks)
		{
			limit_ = data_size < ValuesPerBlock ? data_size : ValuesPerBlock;
		}

		bool has_next() const {
			return block_ < blocks_;
		}

		size_t data_pos() const {return data_pos_;}
		size_t limit() const {return limit_;}
	};


	template <typename Seq>
	class SymbolsSizesIterator: public SymbolsIteratorBase<Seq> {
		using Base = SymbolsIteratorBase<Seq>;

		using typename Base::Symbols;

		using Base::ValuesPerBlock;

		using Base::codec_;
		using Base::symbols_;
		using Base::data_size_;
		using Base::blocks_;

		using Base::data_pos_;
		using Base::block_;
		using Base::limit_;

		UBigInt block_stat_   = 0;
		UBigInt block_offset_ = 0;

	public:
		SymbolsSizesIterator(const Symbols* symbols, size_t data_size, size_t blocks):
			Base(symbols, data_size, blocks)
		{}

		bool has_next() const {
			return block_ < blocks_;
		}

		size_t block_offset() const {
			return block_offset_;
		}

		void next()
		{
			block_stat_ = 0;

			block_offset_ = data_pos_ % ValuesPerBlock;

			while (data_pos_ < limit_)
			{
				UBigInt run_value = 0;
				auto len 	 = codec_.decode(symbols_, run_value, data_pos_);
				auto sym_run = Seq::decode_run(run_value);

				block_stat_ += sym_run.length();

				data_pos_ += len;
			}

			if (limit_ + ValuesPerBlock < data_size_) {
				limit_ += ValuesPerBlock;
			}
			else {
				limit_ = data_size_;
			}

			block_++;
		}

		auto value(Int block) const
		{
			return block_stat_;
		}
	};


	template <typename Seq>
	class SymbolsSumIterator: public SymbolsIteratorBase<Seq> {
		using Base = SymbolsIteratorBase<Seq>;

		using typename Base::Symbols;

		using Base::ValuesPerBlock;

		using Base::codec_;
		using Base::symbols_;
		using Base::data_size_;
		using Base::blocks_;

		using Base::data_pos_;
		using Base::block_;
		using Base::limit_;

		core::StaticVector<UBigInt, Seq::Symbols> block_stat_;

	public:
		SymbolsSumIterator(const Symbols* symbols, size_t data_size, size_t blocks):
			Base(symbols, data_size, blocks)
		{}


		void next()
		{
			block_stat_.clear();
			while (data_pos_ < limit_)
			{
				UBigInt run_value = 0;
				auto len 	 = codec_.decode(symbols_, run_value, data_pos_);
				auto sym_run = Seq::decode_run(run_value);

				block_stat_[sym_run.symbol()] += sym_run.length();

				data_pos_ += len;
			}

			if (limit_ + ValuesPerBlock < data_size_) {
				limit_ += ValuesPerBlock;
			}
			else {
				limit_ = data_size_;
			}

			block_++;
		}

		auto value(Int block) const
		{
			return block_stat_[block];
		}
	};


}


template <typename Seq>
class ReindexFn {
    using SizeIndex = typename Seq::SizeIndex;
    using SumIndex  = typename Seq::SumIndex;


    static const Int Symbols                                              		= Seq::Symbols;
    static const Int ValuesPerBranch                                            = Seq::ValuesPerBranch;
    static const Int Indxes                                                     = Seq::Indexes;





public:
    void reindex(Seq& seq)
    {
        auto meta = seq.metadata();

    	auto symbols_block_size = seq.element_size(Seq::SYMBOLS);
    	auto symbols_blocks 	= seq.number_of_offsets(symbols_block_size);

    	seq.resizeBlock(Seq::OFFSETS, symbols_blocks);
    	seq.clear(Seq::OFFSETS);

        if (symbols_block_size > ValuesPerBranch)
        {
            size_t index_size = symbols_blocks;

            seq.createIndex(index_size);

            auto size_index = seq.size_index();
            auto sum_index  = seq.sum_index();
            auto symbols 	= seq.symbols();
            auto data_size 	= meta->data_size();

            SymbolsSizesIterator<Seq> size_iterator(symbols, data_size, index_size);

            auto offsets = seq.offsets();

            for (size_t c = 0; c < index_size; c++)
            {
            	size_iterator.next();

            	offsets[c] = size_iterator.block_offset();

            	typename Seq::SizeIndex::Values sizes(size_iterator.value(0));

            	size_index->append(sizes);
            }

            size_index->reindex();

            SymbolsSumIterator<Seq> sum_iterator(symbols, data_size, index_size);

            sum_index->populate_from_iterator(0, index_size, sum_iterator);
            sum_index->reindex();
        }
        else {
            seq.removeIndex();
        }
    }



    void check(const Seq& seq)
    {
        auto meta = seq.metadata();

    	auto symbols_block_size = seq.element_size(Seq::SYMBOLS);

        if (symbols_block_size > ValuesPerBranch)
        {
        	auto symbols_blocks 	= seq.number_of_offsets(symbols_block_size);
        	auto offsets_block_size = seq.element_size(Seq::OFFSETS);

        	MEMORIA_V1_ASSERT(offsets_block_size, ==, seq.offsets_segment_size(symbols_block_size));

        	MEMORIA_V1_ASSERT_TRUE(seq.has_index());

            size_t index_size = symbols_blocks;

            auto size_index = seq.size_index();
            auto sum_index  = seq.sum_index();

            MEMORIA_V1_ASSERT(size_index->size(), ==, index_size);
            MEMORIA_V1_ASSERT(sum_index->size(),  ==, index_size);

            auto symbols   = seq.symbols();
            auto offsets   = seq.offsets();
            auto data_size = meta->size();

            auto size_index_iterator = size_index->iterator(0);

            SymbolsSizesIterator<Seq> size_iterator(symbols, data_size, index_size);

            size_t total_size = 0;

            for (size_t c = 0; c < index_size; c++)
            {
            	size_index_iterator.next();
            	size_iterator.next();

            	MEMORIA_V1_ASSERT(size_index_iterator.value(0), ==, size_iterator.value(0));
            	MEMORIA_V1_ASSERT(offsets[c], ==, size_iterator.block_offset());

                total_size += size_iterator.value(0);
            }

            MEMORIA_V1_ASSERT(total_size, ==, (size_t)meta->size());
            MEMORIA_V1_ASSERT(size_iterator.data_pos(), ==, size_iterator.limit());

            SymbolsSumIterator<Seq> sum_iterator(symbols, data_size, index_size);
            auto sum_index_iterator = sum_index->iterator(0);

            for (size_t c = 0; c < index_size; c++)
            {
            	sum_index_iterator.next();
            	sum_iterator.next();

            	for (Int s = 0; s < Seq::Symbols; s++)
            	{
            		MEMORIA_V1_ASSERT(sum_index_iterator.value(s), ==, sum_iterator.value(s));
            	}
            }

            MEMORIA_V1_ASSERT(sum_iterator.data_pos(), ==, sum_iterator.limit());
        }
        else {
            MEMORIA_V1_ASSERT_FALSE(seq.has_index());
            MEMORIA_V1_ASSERT(seq.element_size(Seq::OFFSETS), ==, (Int)PackedAllocator::AlignmentBlock);
        }
    }
};


}}}
