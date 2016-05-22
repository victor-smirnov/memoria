
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

#include <memoria/v1/core/packed/tools/packed_allocator.hpp>
#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree.hpp>

#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/tools/optional.hpp>
#include <memoria/v1/core/tools/bignum/primitive_codec.hpp>

#include <memoria/v1/metadata/page.hpp>

#include "rleseq/rleseq_reindex_fn.hpp"
#include "rleseq/rleseq_iterator.hpp"

#include <ostream>

namespace memoria {
namespace v1 {


namespace {
	static constexpr Int SymbolsRange(Int symbols) {
		return 0; // No ranges defined at the moment
	}

	template <Int Symbols, Int SymbolsRange = SymbolsRange(Symbols)>
	struct SumIndexFactory: HasType<PkdFQTreeT<BigInt, Symbols>> {};
}




template <
    Int Symbols_,
    Int BytesPerBlock
>
struct PkdRLSeqTypes {
    static const Int Blocks                 = Symbols_;
    static const Int ValuesPerBranch        = BytesPerBlock;
};

template <typename Types> class PkdRLESeq;

template <
    Int Symbols,
    Int BytesPerBlock = 128
>
using PkdRLESeqT = PkdRLESeq<PkdRLSeqTypes<Symbols, BytesPerBlock>>;

template <typename Types_>
class PkdRLESeq: public PackedAllocator {

    using Base = PackedAllocator;

public:
    static constexpr UInt VERSION = 1;

    using Types	 = Types_;
    using MyType = PkdRLESeq<Types_>;

    static constexpr PkdSearchType KeySearchType 	= PkdSearchType::SUM;
    static constexpr PkdSearchType SearchType 		= PkdSearchType::SUM;
    static const PackedSizeType SizeType 			= PackedSizeType::VARIABLE;

    static constexpr Int ValuesPerBranch        = Types::ValuesPerBranch;
    static constexpr Int ValuesPerBranchLog2    = NumberOfBits(Types::ValuesPerBranch);

    static constexpr Int Indexes                = Types::Blocks;
    static constexpr Int Symbols                = Types::Blocks;
    static constexpr Int BitsPerSymbol          = NumberOfBits(Symbols - 1);
    static constexpr Int SymbolMask          	= (1 << BitsPerSymbol) - 1;

    static constexpr size_t MaxRunLength        = MaxRLERunLength;

    enum {
        METADATA, SIZE_INDEX, OFFSETS, SUM_INDEX, SYMBOLS, TOTAL_SEGMENTS__
    };

    using Base::clear;

    using Value = UByte;

    using Codec = ValueCodec<UBigInt>;

    using Values = core::StaticVector<BigInt, Indexes>;

    using SumIndex 	= typename SumIndexFactory<Symbols>::Type;
    using SizeIndex = PkdFQTreeT<BigInt, 1>;

    using InputType = Value;
    using InputBuffer = MyType;

    using OffsetsType = UByte;

    class Metadata {
        Int size_;
        Int data_size_;
    public:
        Int& size()                 {return size_;}
        const Int& size() const     {return size_;}

        Int& data_size()                 {return data_size_;}
        const Int& data_size() const     {return data_size_;}
    };

    using SizesT = core::StaticVector<Int, 1>;

    struct Tools {};

    using Iterator = rleseq::RLESeqIterator<MyType>;

    Int number_of_offsets() const
    {
    	return number_of_offsets(this->element_size(SYMBOLS));
    }

    static constexpr Int number_of_offsets(Int values)
    {
        return values > 0 ? divUp(values, ValuesPerBranch) : 1;
    }

    static constexpr Int offsets_segment_size(Int values)
    {
        return PackedAllocator::roundUpBytesToAlignmentBlocks(number_of_offsets(values) * sizeof(OffsetsType));
    }

    static constexpr size_t divUp(size_t value, size_t divisor) {
        return (value / divisor) + ((value % divisor) ? 1 : 0);
    }

    struct Location {
    	size_t data_pos_;
    	size_t data_length_;
    	size_t local_idx_;
    	size_t block_base_;
    	size_t run_base_;

    	RLESymbolsRun run_;
    	bool out_of_range_;

    	Location(size_t data_pos, size_t data_length, size_t local_idx, size_t block_base, size_t run_base, RLESymbolsRun run, bool out_of_range = false):
    		data_pos_(data_pos), data_length_(data_length), local_idx_(local_idx), block_base_(block_base), run_base_(run_base), run_(run), out_of_range_(out_of_range)
    	{}

    	size_t run_suffix() const {return run_.length() - local_idx_;}
    	size_t run_prefix() const {return local_idx_;}

    	size_t local_idx() 	const {return local_idx_;}
    	auto symbol() 		const {return run_.symbol();}
    	auto length() 		const {return run_.length();}

    	auto data_pos() 	const {return data_pos_;}
    	auto data_length() 	const {return data_length_;}
    	auto data_end() 	const {return data_pos_ + data_length_;}
    	auto block() 		const {return data_pos_ / ValuesPerBranch;}
    	auto block_base()	const {return block_base_;}
    	auto run_base()		const {return run_base_;}

    	bool out_of_range() const {return out_of_range_;}

    	const RLESymbolsRun& run() const {return run_;}
    };


public:
    PkdRLESeq() = default;

    Int& size() {return metadata()->size();}
    const Int& size() const {return metadata()->size();}

    Int& data_size() {return metadata()->data_size();}
    const Int& data_size() const {return metadata()->data_size();}



    static constexpr RLESymbolsRun decode_run(UBigInt value)
    {
    	return RLESymbolsRun(value & SymbolMask, value >> BitsPerSymbol);
    }

    static constexpr UBigInt encode_run(Int symbol, UBigInt length)
    {
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


    // ====================================== Accessors ================================= //

    Metadata* metadata()
    {
        return Base::template get<Metadata>(METADATA);
    }

    const Metadata* metadata() const
    {
        return Base::template get<Metadata>(METADATA);
    }

    SumIndex* sum_index()
    {
        return Base::template get<SumIndex>(SUM_INDEX);
    }

    const SumIndex* sum_index() const
    {
        return Base::template get<SumIndex>(SUM_INDEX);
    }

    SizeIndex* size_index()
    {
        return Base::template get<SizeIndex>(SIZE_INDEX);
    }

    const SizeIndex* size_index() const
    {
        return Base::template get<SizeIndex>(SIZE_INDEX);
    }

    OffsetsType* offsets() {
        return this->template get<OffsetsType>(OFFSETS);
    }

    const OffsetsType* offsets() const {
        return this->template get<OffsetsType>(OFFSETS);
    }

    OffsetsType offset(Int idx) const
    {
        return offsets()[idx];
    }

    void set_offset(Int idx, OffsetsType value)
    {
        set_offsets(offsets(), idx, value);
    }

    void set_offset(OffsetsType* block, Int idx, Int value)
    {
        block[idx] = value;
    }

    Int symbols_block_capacity() const
    {
    	return symbols_block_capacity(metadata());
    }

    Int symbols_block_capacity(const Metadata* meta) const
    {
    	return this->element_size(SYMBOLS) - meta->data_size();
    }

    bool has_index() const {
        return Base::element_size(SIZE_INDEX) > 0;
    }

    Value* symbols()
    {
        return Base::template get<Value>(SYMBOLS);
    }

    const Value* symbols() const
    {
        return Base::template get<Value>(SYMBOLS);
    }

    Tools tools() const {
    	return Tools();
    }

    class SymbolAccessor {
        MyType& seq_;
        Int idx_;
    public:
        SymbolAccessor(MyType& seq, Int idx): seq_(seq), idx_(idx) {}

        Value operator=(Value val)
        {
            seq_.set_symbol(idx_, val);
            return val;
        }

        operator Value() const {
            return seq_.get_symbol(idx_);
        }

        Value value() const {
            return seq_.get_symbol(idx_);
        }
    };

    SymbolAccessor symbol(Int idx)
    {
        return SymbolAccessor(*this, idx);
    }

    Int get_symbol(Int idx) const
    {
    	auto result = this->find_run(idx);

    	if (!result.out_of_range())
    	{
    		return result.symbol();
    	}
    	else {
    		throw Exception(MA_SRC, SBuf() << "Symbol index " << idx << " is out of range " << this->size());
    	}
    }



    void set_symbol(Int idx, Int symbol)
    {
    	auto location = this->find_run(idx);

    	if (!location.out_of_range())
    	{
    		if (location.symbol() != symbol)
    		{
    			if (location.run_prefix() > 0)
    			{
    				if (location.run_suffix() > 1)
    				{
    					location = split_run(location, 1);
    					insert_run(location.data_pos(), symbol, 1);
    				}
    				else {
    					auto run_value_length = add_run_length(location, -1);
    					insert_run(location.data_pos() + run_value_length, symbol, 1);
    				}
    			}
    			else if (location.length() > 1)
    			{
    				add_run_length(location, -1);
    				insert_run(location.data_pos(), symbol, 1);
    			}
    			else {
    				remove_run(location);
    				insert_run(location.data_pos(), symbol, 1);
    				try_merge_two_adjustent_runs(location.data_pos());
    			}

    			reindex();
    		}
    	}
    	else {
    		throw Exception(MA_SRC, SBuf() << "Symbol index " << idx << " is out of range " << this->size());
    	}
    }


    Int value(Int symbol, Int idx) const {
        return this->symbol(idx) == symbol;
    }


    class ConstSymbolAccessor {
        const MyType& seq_;
        Int idx_;
    public:
        ConstSymbolAccessor(const MyType& seq, Int idx): seq_(seq), idx_(idx) {}

        operator Value() const {
            return seq_.get_symbol(idx_);
        }

        Value value() const {
            return seq_.get_symbol(idx_);
        }
    };

    ConstSymbolAccessor symbol(Int idx) const
    {
        return ConstSymbolAccessor(*this, idx);
    }


    void append(Int symbol, UBigInt length)
    {
    	MEMORIA_V1_ASSERT_TRUE(symbol >= 0 && symbol < Symbols);

    	auto meta = this->metadata();

    	auto run_value = encode_run(symbol, length);

    	Codec codec;
    	auto len = codec.length(run_value);

    	ensure_capacity(len);

    	meta->data_size() += codec.encode(this->symbols(), run_value, meta->data_size());
    	meta->size() += length;
    }

    void append_and_reindex(Int symbol, UBigInt length)
    {
    	append(symbol, length);
    	reindex();
    }


public:

    // ===================================== Allocation ================================= //

    void init(Int block_size)
    {
        MEMORIA_V1_ASSERT(block_size, >=, empty_size());

        init();
    }

    void init()
    {
        Base::init(empty_size(), TOTAL_SEGMENTS__);

        Metadata* meta  = Base::template allocate<Metadata>(METADATA);

        meta->size() = 0;
        meta->data_size() = 0;

        this->template allocateArrayBySize<OffsetsType>(OFFSETS, number_of_offsets(0));

        Base::setBlockType(SIZE_INDEX, PackedBlockType::ALLOCATABLE);
        Base::setBlockType(SUM_INDEX,  PackedBlockType::ALLOCATABLE);
        Base::setBlockType(SYMBOLS, PackedBlockType::RAW_MEMORY);

        // other sections are empty at this moment
    }


public:
    static Int empty_size()
    {
        Int metadata_length 	= Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
        Int size_index_length   = 0;
        Int offsets_length  	= offsets_segment_size(0);
        Int sum_index_length    = 0;
        Int values_length   	= 0;
        Int block_size      	= Base::block_size(metadata_length + size_index_length  + offsets_length + sum_index_length + values_length, TOTAL_SEGMENTS__);
        return block_size;
    }


    void removeIndex()
    {
        Base::free(SIZE_INDEX);
        Base::free(SUM_INDEX);
    }


    void createIndex(Int index_size)
    {
        Int size_index_block_size = SizeIndex::block_size(index_size);
        Base::resizeBlock(SIZE_INDEX, size_index_block_size);

        Int sum_index_block_size = SumIndex::block_size(index_size);
        Base::resizeBlock(SUM_INDEX, sum_index_block_size);

        auto size_index = this->size_index();
        size_index->setAllocatorOffset(this);
        size_index->init(index_size);

        auto sum_index = this->sum_index();
        sum_index->setAllocatorOffset(this);
        sum_index->init(index_size);
    }

    Iterator iterator(Int symbol_pos) const
    {
    	auto meta = this->metadata();

    	if (symbol_pos < meta->size())
    	{
    		if (!has_index())
    		{
    			auto result = locate_run(meta, 0, symbol_pos, 0);
    			return Iterator(symbols(), result.data_pos(), meta->data_size(), result.local_idx(), result.run_base(), result.run());
    		}
    		else
    		{
    			auto find_result = this->size_index()->find_gt(0, symbol_pos);

    			Int local_pos 		= symbol_pos - find_result.prefix();
    			size_t block_offset = find_result.idx() * ValuesPerBranch;
    			auto offset 		= offsets()[find_result.idx()];

    			block_offset += offset;

    			auto result = locate_run(meta, block_offset, local_pos, find_result.prefix());
    			return Iterator(symbols(), result.data_pos(), meta->data_size(), result.local_idx(), result.run_base(), result.run());
    		}
    	}
    	else {
    		return end();
    	}
    }

    Iterator end() const
    {
    	auto meta = this->metadata();
    	return Iterator(symbols(), meta->data_size(), meta->data_size(), meta->size(), 0, RLESymbolsRun());
    }

    Iterator begin() const
    {
    	return iterator(0);
    }


    // ========================================= Update ================================= //

    void reindex()
    {
    	rleseq::ReindexFn<MyType> reindex_fn;
        reindex_fn.reindex(*this);
    }

    void check() const
    {
        if (has_index())
        {
            size_index()->check();
            sum_index()->check();

            rleseq::ReindexFn<MyType> reindex_fn;
            reindex_fn.check(*this);
        }
    }

    void set(Int idx, Int symbol)
    {
        MEMORIA_V1_ASSERT(idx , <, size());

        //tools().set(symbols(), idx, symbol);
    }

    void clear()
    {
        Base::resizeBlock(SYMBOLS, 0);
        removeIndex();

        auto meta = this->metadata();

        meta->size() = 0;
        meta->data_size() = 0;
    }


    void ensure_capacity(Int capacity)
    {
    	Int current_capacity = this->symbols_block_capacity();

    	if (current_capacity < capacity)
    	{
    		enlargeData(capacity - current_capacity);
    	}
    }


protected:



    void enlargeData(Int length)
    {
        Int new_size = this->element_size(SYMBOLS) + length;
        Base::resizeBlock(SYMBOLS, new_size);
    }


    void insertDataRoom(Int pos, Int length)
    {
        enlargeData(length);

        auto symbols = this->symbols();

        Int rest = size() - pos;

        tools().move(symbols, pos, (pos + length), rest);

        size() += length;
    }

    void shrinkData(Int length)
    {
        Int current_size = this->element_size(SYMBOLS);

    	Int new_size = current_size - length;

        if (new_size >= 0)
        {
            Base::resizeBlock(SYMBOLS, new_size);
        }
    }

    void shrink_to_data()
    {
    	shrinkData(this->symbols_block_capacity());
    }

public:

    template <Int Offset, Int Size, typename AccessorFn, typename T2, template <typename, Int> class BranchNodeEntryItem>
    void _insert_b(Int idx, BranchNodeEntryItem<T2, Size>& accum, AccessorFn&& values)
    {
        insert(idx, values(0));
    }



    void insert(Int pos, Int symbol)
    {
        insert(pos, symbol, 1);
    }

    void remove(Int start, Int end)
    {
    	auto meta = this->metadata();

        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(end, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);
        MEMORIA_V1_ASSERT(end, <=, meta->size());

        auto location_start = find_run(start);
        auto location_end 	= find_run(end);

        if (location_end.local_idx() > 0)
        {
        	if (location_start.run_prefix() > 0)
        	{
        		remove_runs(location_start, location_end);

        		if (location_start.symbol() == location_end.symbol())
        		{
        			try_merge_two_adjustent_runs(location_start.data_pos());
        		}
        	}
        	else {
        		remove_runs_one_sided_left(location_start, location_end);
        		try_merge_two_adjustent_runs(location_start.data_pos());
        	}
        }
        else if (location_start.run_prefix() > 0)
        {
        	remove_runs_one_sided(location_start, location_end.data_pos());

        	try_merge_two_adjustent_runs(location_start.data_pos());
        }
        else {
        	remove_runs_two_sided(location_start.data_pos(), location_end.data_pos());
        }

        meta->size() -= end - start;

        shrink_to_data();

        reindex();
    }

    void insert(Int idx, Int symbol, UBigInt length)
    {
    	auto location = find_run(idx);

    	if (!location.out_of_range())
    	{
    		if (location.symbol() != symbol || location.length() + length > MaxRunLength)
    		{
    			if (location.run_prefix() > 0)
    			{
    				location = split_run(location);
    			}

    			insert_run(location.data_pos(), symbol, length);
    		}
    		else {
    			add_run_length(location, length);
    		}
    	}
    	else {
    		insert_run(location.data_pos(), symbol, length);
    	}

    	reindex();
    }







    void fill(Int start, Int end, std::function<Value ()> fn)
    {
        auto symbols = this->symbols();
        auto tools = this->tools();

        for (Int c = start; c < end; c++)
        {
            Value val = fn();
            tools.set(symbols, c, val);
        }
    }

    void insert_buffer(Int at, const InputBuffer* buffer, Int start, Int size)
    {
        insertDataRoom(at, size);
        tools().move(buffer->symbols(), this->symbols(), start, at, size);
        reindex();
    }


    void insert(Int start, Int length, std::function<Value ()> fn)
    {
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, size());

        MEMORIA_V1_ASSERT(length, >=, 0);

        insertDataRoom(start, length);
        fill(start, start + length, fn);
        reindex();
    }




    template <typename Adaptor>
    void fill_with_buf(Int start, Int length, Adaptor&& adaptor)
    {
        Int size = this->size();

        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, size);
        MEMORIA_V1_ASSERT(length, >=, 0);

        insertDataRoom(start, length);

        auto symbols = this->symbols();

        Int total = 0;

        while (total < length)
        {
            auto buf = adaptor(length - total);

            tools().move(buf.symbols(), symbols, 0, start + total, buf.size());

            total += buf.size();
        }

        reindex();
    }


    void update(Int start, Int end, std::function<Value ()> fn)
    {
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);
        MEMORIA_V1_ASSERT(end, <=, size());

        fill(start, end, fn);
        reindex();
    }


    using ReadState = SizesT;

    void read(Int start, Int end, std::function<void (Value)> fn) const
    {
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);
        MEMORIA_V1_ASSERT(end, <=, size());

        auto symbols    = this->symbols();
        auto tools      = this->tools();

        for (Int c = start; c < end; c++)
        {
            fn(tools.get(symbols, c));
        }
    }



    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void _insert(Int idx, Int symbol, BranchNodeEntryItem<T, Size>& accum)
    {
        insert(idx, symbol);

        sum<Offset>(idx, accum);
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void _update(Int idx, Int symbol, BranchNodeEntryItem<T, Size>& accum)
    {
        sub<Offset>(idx, accum);

        this->symbol(idx) = symbol;

        this->reindex();

        sum<Offset>(idx, accum);
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void _remove(Int idx, BranchNodeEntryItem<T, Size>& accum)
    {
        sub<Offset>(idx, accum);
        remove(idx, idx + 1);
    }


    // ========================================= Node ================================== //




    void splitTo(MyType* other, Int idx)
    {
    	auto meta = this->metadata();

    	if (idx < meta->size())
    	{
    		auto other_meta = other->metadata();

    		auto location 	= this->find_run(idx);

    		Codec codec;

    		size_t symbols_to_move 		= location.run_suffix();
    		UBigInt suffix_value 		= encode_run(location.symbol(), symbols_to_move);
    		size_t  suffix_value_length = codec.length(suffix_value);

    		Int current_run_data_length = location.data_length();

    		Int data_to_move_remainder 	= meta->data_size() - (location.data_pos() + current_run_data_length);

    		Int data_to_move = data_to_move_remainder + suffix_value_length;

    		other->ensure_capacity(data_to_move);

    		codec.move(other->symbols(), 0, data_to_move, other_meta->data_size());

    		codec.copy(symbols(), location.data_pos() + current_run_data_length, other->symbols(), suffix_value_length, data_to_move_remainder);
    		codec.encode(other->symbols(), suffix_value, 0);

    		other_meta->data_size() += data_to_move;
    		other_meta->size() 		+= meta->size() - idx;

    		other->try_merge_two_adjustent_runs(0);

    		other->reindex();

    		remove(idx, meta->size());
    	}
    }

    void mergeWith(MyType* other) const
    {
    	auto meta 		= this->metadata();
    	auto other_meta = other->metadata();

    	Int data_to_move = meta->data_size();

        other->ensure_capacity(data_to_move);

        Codec codec;

        codec.move(other->symbols(), 0, data_to_move, other_meta->data_size());
        codec.copy(symbols(), 0, other->symbols(), 0, data_to_move);

        other_meta->data_size() += data_to_move;
        other_meta->size() 		+= meta->size();

        compactify_runs();

        other->reindex();
    }


    // ========================================= Query ================================= //

    Values sums() const
    {
        if (has_index())
        {
            auto index = this->index();
            return index->sums();
        }
        else {
            return sums(size());
        }
    }


    Values sums(Int to) const
    {
        if (has_index())
        {
            auto index = this->index();

            Int index_block = to / ValuesPerBranch;

            auto isums = index->sums(0, index_block);

            auto vsums = tools().sum(index_block * ValuesPerBranch, to);

            vsums.sumUp(isums);

            return vsums;
        }
        else
        {
            auto vsums = tools().sum(0, to);
            return vsums;
        }
    }




    Values ranks(Int to) const
    {
        Values vals;

        for (Int symbol = 0; symbol < Indexes; symbol++)
        {
            vals[symbol] = rank(to, symbol);
        }

        return vals;
    }

    Values ranks() const
    {
        return this->ranks(this->size());
    }




    Values sumsAt(Int idx) const
    {
        Values values;
        values[symbol(idx)] = 1;

        return values;
    }

    Values sums(Int from, Int to) const
    {
        return sums(to) - sums(from);
    }


    void sums(Int from, Int to, Values& values) const
    {
        values += sums(from, to);
    }

    void sums(Values& values) const
    {
        values += sums();
    }

    Values sum_v(Int from, Int to) const {
        return sums(from, to);
    }


    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void max(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        for (Int block = 0; block < Indexes; block++)
        {
            accum[block + Offset] = rank(block);
        }
    }



    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void sum(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        for (Int block = 0; block < Indexes; block++)
        {
            accum[block + Offset] += rank(block);
        }
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void sum(Int start, Int end, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        for (Int block = 0; block < Indexes; block++)
        {
            accum[block + Offset] += rank(start, end, block);
        }
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void sum(Int idx, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        accum[symbol(idx) + Offset] ++;
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void sub(Int idx, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        accum[symbol(idx) + Offset]--;
    }


    template <Int Offset, Int From, Int To, typename T, template <typename, Int, Int> class BranchNodeEntryItem>
    void sum(Int start, Int end, BranchNodeEntryItem<T, From, To>& accum) const
    {
        for (Int block = 0; block < Indexes; block++)
        {
            accum[block + Offset] += rank(block, start, end);
        }
    }


    Int sum(Int symbol, Int start, Int end) const
    {
        return rank(start, end, symbol);
    }

    Int sum(Int symbol, Int end) const
    {
        return rank(end, symbol);
    }

    Int sum(Int symbol) const
    {
        return rank(symbol);
    }





    template <typename T>
    void _add(Int symbol, T& value) const
    {
        value += rank(symbol);
    }

    template <typename T>
    void _add(Int symbol, Int end, T& value) const
    {
        value += rank(end, symbol);
    }

    template <typename T>
    void _add(Int symbol, Int start, Int end, T& value) const
    {
        value += rank(start, end, symbol);
    }



    template <typename T>
    void _sub(Int symbol, T& value) const
    {
        value -= rank(symbol);
    }

    template <typename T>
    void _sub(Int symbol, Int end, T& value) const
    {
        value -= rank(end, symbol);
    }

    template <typename T>
    void _sub(Int symbol, Int start, Int end, T& value) const
    {
        value -= rank(start, end, symbol);
    }


//    Int get(Int idx) const
//    {
//        MEMORIA_V1_ASSERT(idx , <, size());
//        return tools().get(symbols(), idx);
//    }

    Int get_values(Int idx) const
    {
        MEMORIA_V1_ASSERT(idx , <, size());
        return tools().get(symbols(), idx);
    }

    bool test(Int idx, Int symbol) const
    {
        MEMORIA_V1_ASSERT(idx , <, size());
        return iterator(idx).symbol() == symbol;
    }

    Int rank(Int symbol) const
    {
        if (has_index())
        {
            const auto* index = this->sum_index();
            return index->sum(symbol);
        }
        else {
            return rank(size(), symbol);
        }
    }

    Int rank(Int start, Int end, Int symbol) const
    {
        Int rank_start  = rank(start, symbol);
        Int rank_end    = rank(end, symbol);

        return rank_end - rank_start;
    }

    UBigInt rank(Int end, Int symbol) const
    {
    	auto meta = this->metadata();
    	Int size = meta->size();

        MEMORIA_V1_ASSERT_TRUE(end >= 0);

        MEMORIA_V1_ASSERT_TRUE(symbol >= 0 && symbol < Symbols);

        if (has_index())
        {
        	const SumIndex* sum_index  = this->sum_index();

        	if (end < size)
        	{
        		auto location 		= find_run(end);
        		auto block 	  		= location.block();
        		auto block_start 	= block * ValuesPerBranch;

        		auto rank_base 	= sum_index->sum(symbol, block);

        		auto block_offset = offsets()[block];

        		auto local_rank = block_rank(meta, block_start + block_offset, end - location.block_base(), symbol);

        		return local_rank + rank_base;
        	}
        	else {
        		return sum_index->sum(symbol);
        	}
        }
        else {
        	return block_rank(meta, 0, end, symbol);
        }
    }

    UBigInt rank_s(Int end, Int symbol) const
    {
    	MEMORIA_V1_ASSERT(end, >=, 0);
    	MEMORIA_V1_ASSERT(end, <=, size());

    	return block_rank(metadata(), 0, end, symbol);
    }

    UBigInt rank_s(Int start, Int end, Int symbol) const
    {
    	return rank_s(end, symbol) - rank_s(start, symbol);
    }

    UBigInt rank_is(Int end, Int symbol) const
    {
    	auto iter = begin();

    	UBigInt rank = 0;

    	while (iter.has_data() && iter.idx() < end)
    	{
    		rank += iter.symbol() == symbol;
    		iter.next();
    	}

    	return rank;
    }

    UBigInt rank_is(Int start, Int end, Int symbol) const
    {
    	return rank_is(end, symbol) - rank_s(start, symbol);
    }

    auto select_s(Int symbol, UBigInt rank) const
    {
    	MEMORIA_V1_ASSERT(rank, >=, 1);
    	return block_select(metadata(), symbols(), 0, rank, 0, symbol);
    }

    auto select_is(Int symbol, UBigInt rank) const {
    	return select_is(begin(), symbol, rank);
    }

    auto selectFw_is(Int pos, UBigInt rank, Int symbol) const
    {
    	return select_fw_is(iterator(pos), symbol, rank);
    }


    UBigInt find_rank_iter(Int end, Int symbol) const
    {
    	auto iter = iterator(0);

    	UBigInt rank = 0;

    	while (iter.has_next())
    	{
    		iter.next();
    		if (iter.idx() < end)
    		{
    			rank += iter.symbol() == symbol;
    		}
    	}

    	return rank;
    }


    Iterator select(UBigInt rank, Int symbol) const
    {
    	auto meta 	 = this->metadata();
    	auto symbols = this->symbols();

        MEMORIA_V1_ASSERT_TRUE(symbol >= 0 && symbol < Symbols);

        if (has_index())
        {
        	const SumIndex* sum_index 	= this->sum_index();
        	auto find_result 			= sum_index->find_ge(symbol, rank);
        	Int blocks 					= sum_index->size();

        	if (find_result.idx() < blocks)
        	{
        		auto block_start   = find_result.idx() * ValuesPerBranch;
        		auto block_offset  = offsets()[find_result.idx()];
        		UBigInt local_rank = rank - find_result.prefix();

        		auto block_size_start  = this->size_index()->sum(0, find_result.idx());

        		return block_select(meta, symbols, block_start + block_offset, local_rank, block_size_start, symbol);
        	}
        	else {
        		return this->iterator(meta->size());
        	}
        }
        else {
        	return block_select(meta, symbols, 0, rank, 0, symbol);
        }
    }


    Iterator selectFw(Int pos, UBigInt rank, Int symbol) const
    {
    	MEMORIA_V1_ASSERT(rank, >=, 1);

    	auto meta = this->metadata();

    	if (pos < meta->size())
    	{
    		UBigInt rank_prefix = this->rank(pos + 1, symbol);
    		return select(rank_prefix + rank, symbol);
    	}
    	else {
    		return end();
    	}
    }

    Optional<Iterator> selectBw(Int pos, UBigInt rank, Int symbol) const
    {
    	MEMORIA_V1_ASSERT(rank, >=, 1);

    	auto meta = this->metadata();

    	if (pos < meta->size())
    	{
    		UBigInt rank_prefix = this->rank(pos, symbol);
    		if (rank_prefix > 0)
    		{
    			return Optional<Iterator>(select(rank_prefix - (rank - 1), symbol));
    		}
    		else {
    			return Optional<Iterator>();
    		}
    	}
    	else {
    		return Optional<Iterator>();
    	}
    }



    UBigInt count(Int pos) const
    {
    	auto location = find_run(pos);
    	return block_count(metadata(), symbols(), location);
    }


    void dump(std::ostream& out = cout, bool dump_index = true) const
    {
    	TextPageDumper dumper(out);

    	generateDataEvents(&dumper);
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        handler->startGroup("PACKED_RLE_SEQUENCE");
        auto meta = this->metadata();

        handler->value("SIZE", &meta->size());
        handler->value("DATA_SIZE", &meta->data_size());

        if (has_index())
        {
        	handler->startGroup("INDEXES");
        	size_index()->generateDataEvents(handler);
            sum_index()->generateDataEvents(handler);
            handler->endGroup();
        }

        handler->value("OFFSETS", PageValueProviderFactory::provider(true, number_of_offsets(), [&](Int idx) {
        	return offsets()[idx];
        }));

        handler->startGroup("SYMBOL RUNS", size());

        auto iter = this->iterator(0);

        BigInt values[4] = {0, 0, 0};

        while (iter.has_data())
        {
        	values[1] = iter.run().symbol();
        	values[2] = iter.run().length();

        	handler->value("RUN", values, 3);

        	values[0] += iter.run().length();

        	iter.next_run();
        }

        handler->endGroup();

        handler->endGroup();
    }

    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        const Metadata* meta = this->metadata();

        FieldFactory<Int>::serialize(buf, meta->size());
        FieldFactory<Int>::serialize(buf, meta->data_size());

        FieldFactory<OffsetsType>::serialize(buf, offsets(), number_of_offsets());

        if (has_index())
        {
            size_index()->serialize(buf);
            sum_index()->serialize(buf);
        }

        FieldFactory<Value>::serialize(buf, symbols(), meta->data_size());
    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        Metadata* meta = this->metadata();

        FieldFactory<Int>::deserialize(buf, meta->size());
        FieldFactory<Int>::deserialize(buf, meta->data_size());

        FieldFactory<OffsetsType>::deserialize(buf, offsets(), number_of_offsets());

        if (has_index()) {
            size_index()->deserialize(buf);
            sum_index()->deserialize(buf);
        }

        FieldFactory<Value>::deserialize(buf, symbols(), meta->data_size());
    }


    auto find_run(Int symbol_pos) const
    {
    	if (symbol_pos >= 0)
    	{
    		auto meta = this->metadata();

    		if (symbol_pos < meta->size())
    		{
    			if (!has_index())
    			{
    				return locate_run(meta, 0, symbol_pos, 0);
    			}
    			else
    			{
    				auto find_result = this->size_index()->find_gt(0, symbol_pos);

    				Int local_pos 		= symbol_pos - find_result.prefix();
    				size_t block_offset = find_result.idx() * ValuesPerBranch;
    				auto offset 		= offsets()[find_result.idx()];

    				block_offset += offset;

    				return locate_run(meta, block_offset, local_pos, find_result.prefix());
    			}
    		}
    		else {
    			return Location(meta->data_size(), 0, 0, meta->data_size(), symbol_pos, RLESymbolsRun(), true);
    		}
    	}
    	else {
    		throw Exception(MA_SRC, SBuf() << "Symbol index must be >= 0: " << symbol_pos);
    	}
    }

    void compactify()
    {
    	compactify_runs();
    	reindex();
    }

private:
    auto select_fw_is(Iterator iter, Int symbol, UBigInt rank) const
    {
    	MEMORIA_V1_ASSERT(rank, >=, 1);

    	UBigInt cnt = 0;

    	while (iter.has_data())
    	{
    		if (iter.symbol() == symbol) {
    			if (++cnt == rank)
    			{
    				return iter;
    			}
    		}
    	}

    	return end();
    }


    size_t block_rank(const Metadata* meta, size_t data_pos, size_t idx, Int symbol) const
    {
    	Codec codec;
    	size_t data_size = meta->data_size();
    	auto symbols = this->symbols();

    	size_t run_base  = 0;
    	UBigInt rank 	 = 0;

    	while (data_pos < data_size)
    	{
    		UBigInt run_value = 0;
    		auto len = codec.decode(symbols, run_value, data_pos);
    		auto run = decode_run(run_value);

    		auto run_length = run.length();

    		if (run.symbol() == symbol)
    		{
    			if (idx >= run_base + run_length)
    			{
    				rank += run_length;
    			}
    			else {
    				return rank + idx - run_base;
    			}
    		}
    		else if (idx < run_base + run_length)
			{
				return rank;
			}

			run_base += run_length;
			data_pos += len;
    	}

    	return rank;
    }

    class BlockSelectResult {
    	UBigInt rank_;
    	size_t data_pos_;
    	size_t local_idx_;
    	size_t block_idx_;
    public:
    	BlockSelectResult(UBigInt rank, size_t data_pos, size_t local_idx, size_t block_idx):
    		rank_(rank), data_pos_(data_pos), local_idx_(local_idx), block_idx_(block_idx)
    	{}

    	UBigInt rank() const {return rank_;}
    	size_t data_pos() const {return data_pos_;}
    	size_t local_idx() const {return local_idx_;}
    	size_t block_idx() const {return block_idx_;}
    };


    Iterator block_select(const Metadata* meta, const Value* symbols, size_t data_pos, UBigInt rank, size_t block_size_prefix, Int symbol) const
    {
    	Codec codec;
    	size_t data_size = meta->data_size();

    	UBigInt rank_base = 0;
    	size_t  run_base  = 0;

    	RLESymbolsRun run;

    	while (data_pos < data_size)
    	{
    		UBigInt run_value = 0;
    		auto len = codec.decode(symbols, run_value, data_pos);
    		run = decode_run(run_value);

    		auto run_length = run.length();

			if (run.symbol() == symbol)
			{
				if (rank > rank_base + run_length)
				{
					rank_base += run_length;
					run_base  += run_length;
				}
				else {
					size_t local_idx   = rank - rank_base - 1;
					return Iterator(symbols, data_pos, data_size, local_idx, run_base + block_size_prefix, run);
				}
			}
			else {
				run_base += run_length;
			}

			data_pos += len;
    	}

    	return Iterator(symbols, data_size, data_size, 0, run_base + block_size_prefix, run);
    }


    UBigInt block_count(const Metadata* meta, const Value* symbols, const Location& location) const
    {
    	size_t pos = location.data_pos();
    	size_t data_size = meta->data_size();

    	Codec codec;

    	UBigInt local_pos = location.local_idx();

    	UBigInt count = 0;
    	Int last_symbol = -1;

    	while (pos < data_size)
    	{
    		UBigInt run_value = 0;
    		auto len = codec.decode(symbols, run_value, pos);
    		auto run = decode_run(run_value);

    		if (run.symbol() == last_symbol || last_symbol == -1)
    		{
    			pos += len;
    			count += run.length() - local_pos;
    			last_symbol = run.symbol();

    			local_pos = 0;
    		}
    		else {
    			return count;
    		}
    	}

    	return count;
    }


    void compactify_runs()
    {
    	auto meta 	 = this->metadata();
    	auto symbols = this->symbols();

    	size_t pos = 0;
    	size_t data_size = meta->data_size();

    	Codec codec;

    	while (pos < data_size)
    	{
    		size_t pos0 = pos;

    		Int last_symbol = -1;
    		BigInt total_length = 0;

    		size_t runs;
    		for (runs = 0; pos0 < data_size; runs++)
    		{
    			UBigInt run_value = 0;
    			auto len = codec.decode(symbols, run_value, pos0);
    			auto run = decode_run(run_value);

    			if (run.symbol() == last_symbol || last_symbol == -1)
    			{
    				if (total_length + run.length() <= MaxRunLength)
    				{
    					pos0 += len;
    					total_length += run.length();
    					last_symbol = run.symbol();
    				}
    				else {
    					break;
    				}
    			}
    			else {
    				break;
    			}
    		}


    		if (runs > 1)
    		{
    			auto new_run_value 			= encode_run(last_symbol, total_length);
    			size_t new_run_value_length = codec.length(new_run_value);
    			size_t current_length		= pos0 - pos;

    			if (current_length > new_run_value_length)
    			{
    				codec.move(symbols, pos0, pos + new_run_value_length, data_size - pos0);

    				codec.encode(symbols, new_run_value, pos);

    				data_size -= current_length - new_run_value_length;
    				pos += new_run_value_length;
    			}
    			else {
    				pos = pos0;
    			}
    		}
    		else {
    			pos = pos0;
    		}
    	}

    	meta->data_size() = data_size;

    	shrink_to_data();
    }



    Location locate_run(const Metadata* meta, size_t pos, size_t idx, size_t size_base) const
    {
    	Codec codec;
    	auto symbols = this->symbols();

    	size_t base = 0;

    	size_t limit = meta->data_size();

    	while (pos < limit)
    	{
    		UBigInt run_value = 0;
    		auto len = codec.decode(symbols, run_value, pos);
    		auto run = decode_run(run_value);

    		auto run_length = run.length();

    		if (idx >= base + run_length)
    		{
    			base += run_length;
    			pos += len;
    		}
    		else {
    			return Location(pos, len, idx - base, size_base, base, run);
    		}
    	}

    	throw Exception(MA_SRC, SBuf() << "Symbol index is out of bounds: " << idx << " " <<meta->size());
    }



    void remove_runs(const Location& start, const Location& end)
    {
    	if (start.data_pos() < end.data_pos())
    	{
    		auto meta = this->metadata();

    		Codec codec;

    		UBigInt new_start_run_value 	= encode_run(start.symbol(), start.run_prefix());
    		size_t new_start_run_length 	= codec.length(new_start_run_value);

    		UBigInt new_end_run_value 		= encode_run(end.symbol(), end.run_suffix());
    		size_t new_end_run_length 		= codec.length(new_end_run_value);

    		size_t hole_start 	= start.data_pos() + new_start_run_length;
    		size_t hole_end 	= end.data_pos() + (end.data_length() - new_end_run_length);

    		size_t to_remove  	= hole_end - hole_start;

    		auto symbols = this->symbols();

    		codec.move(symbols, hole_end, hole_start, meta->data_size() - hole_end);

    		codec.encode(symbols, new_start_run_value, start.data_pos());
    		codec.encode(symbols, new_end_run_value, hole_start);

    		meta->data_size() -= to_remove;
    	}
    	else {
    		BigInt delta = end.local_idx() - start.local_idx();
    		add_run_length0(start, -delta);
    	}
    }


    void remove_runs_one_sided_left(const Location& start, const Location& end)
    {
    	Codec codec;

    	auto meta = this->metadata();

    	UBigInt new_end_run_value 		= encode_run(end.symbol(), end.run_suffix());
    	size_t new_end_run_length 		= codec.length(new_end_run_value);

    	size_t hole_start 	= start.data_pos();
    	size_t hole_end 	= end.data_pos() + (end.data_length() - new_end_run_length);

    	size_t to_remove  	= hole_end - hole_start;

    	auto symbols = this->symbols();

    	codec.move(symbols, hole_end, hole_start, meta->data_size() - hole_end);

    	codec.encode(symbols, new_end_run_value, hole_start);

    	meta->data_size() -= to_remove;
    }


    void remove_runs_one_sided(const Location& start, size_t end)
    {
    	Codec codec;

    	auto meta = this->metadata();

    	UBigInt new_run_value 	= encode_run(start.symbol(), start.run_prefix());
    	size_t new_run_length 	= codec.length(new_run_value);

    	size_t hole_start = start.data_pos() + new_run_length;
    	size_t to_remove  = end - hole_start;

    	auto symbols = this->symbols();

    	codec.move(symbols, end, hole_start, meta->data_size() - end);

    	codec.encode(symbols, new_run_value, start.data_pos());

    	meta->data_size() -= to_remove;
    }

    void remove_runs_two_sided(size_t start, size_t end)
    {
    	size_t to_remove  = end - start;

    	auto symbols = this->symbols();

    	auto meta = this->metadata();

    	Codec codec;
    	codec.move(symbols, end, start, meta->data_size() - end);

    	meta->data_size() -= to_remove;
    }

    void try_merge_two_adjustent_runs(size_t run_pos)
    {
    	auto meta 	 = this->metadata();
    	auto symbols = this->symbols();

    	Codec codec;

    	UBigInt first_run_value = 0;
    	auto first_len = codec.decode(symbols, first_run_value, run_pos);

    	if (run_pos + first_len < meta->data_size())
    	{
    		UBigInt next_run_value = 0;
    		auto next_len = codec.decode(symbols, next_run_value, run_pos + first_len);

    		auto first_run 	= decode_run(first_run_value);
    		auto next_run 	= decode_run(next_run_value);

    		if (first_run.symbol() == next_run.symbol() && first_run.length() + next_run.length() <= MaxRunLength)
    		{
    			UBigInt new_run_value 		= encode_run(first_run.symbol(), first_run.length() + next_run.length());
    			size_t new_run_value_length = codec.length(new_run_value);

    			size_t window_size = first_len + next_len;

    			if (new_run_value_length <= window_size)
    			{
    				size_t to_remove = window_size - new_run_value_length;

    				codec.move(symbols, run_pos + first_len + next_len, run_pos + new_run_value_length, meta->data_size() - (run_pos  + window_size));
    				codec.encode(symbols, new_run_value, run_pos);

    				meta->data_size() -= to_remove;
    			}
    		}
    	}
    }

    Location split_run(const Location& location, UBigInt subtraction = 0)
    {
    	UBigInt pos = location.local_idx();

    	if (pos > 0 && pos < location.length())
    	{
    		Codec codec;
    		auto symbols = this->symbols();
    		auto meta    = this->metadata();

    		UBigInt prefix_run_value 	= encode_run(location.symbol(), location.run_prefix());
    		size_t  prefix_value_length = codec.length(prefix_run_value);

    		UBigInt suffix_run_value = encode_run(location.symbol(), location.run_suffix() - subtraction);
    		size_t  suffix_value_length = codec.length(suffix_run_value);

    		size_t total_length = prefix_value_length + suffix_value_length;

    		if (total_length >= location.data_length())
    		{
    			size_t delta = total_length - location.data_length();

    			if (delta > 0)
    			{
    				ensure_capacity(delta);
    				codec.move(symbols, location.data_end(), location.data_end() + delta, meta->data_size() - location.data_end());
    			}

    			auto pos_tmp = location.data_pos();
    			pos_tmp += codec.encode(symbols, prefix_run_value, pos_tmp);
    			codec.encode(symbols, suffix_run_value, pos_tmp);

    			meta->data_size() += delta;
    			meta->size() -= subtraction;
    		}
    		else {
    			size_t delta = location.data_length() - total_length;

    			codec.move(symbols, location.data_end(), location.data_end() - delta, meta->data_size() - location.data_end());

    			auto pos_tmp = location.data_pos();
    			pos_tmp += codec.encode(symbols, prefix_run_value, pos_tmp);
    			codec.encode(symbols, suffix_run_value, pos_tmp);

    			meta->data_size() -= delta;
    			meta->size() -= subtraction;

    			shrink_to_data();
    		}

    		return Location(
    				location.data_pos() + prefix_value_length,
					suffix_value_length,
					0,
					location.block_base(),
					location.run_base() + location.run_prefix(),
					RLESymbolsRun(location.symbol(), location.run_suffix())
			);
    	}
    	else {
    		throw Exception(MA_SRC, SBuf() << "split_run: invalid split position: " << pos << " " << location.length());
    	}
    }

    size_t add_run_length0(const Location& location, BigInt length)
    {
    	Codec codec;
    	auto meta = this->metadata();

		UBigInt run_value 		= encode_run(location.symbol(), location.length() + length);
		size_t run_value_length = codec.length(run_value);

		ensure_capacity(run_value_length);

		auto symbols = this->symbols();

		codec.move(symbols, location.data_end(), location.data_pos() + run_value_length, meta->data_size() - location.data_end());
		codec.encode(symbols, run_value, location.data_pos());

		meta->data_size() += (run_value_length - location.data_length());

		return run_value_length;
    }

    size_t add_run_length(const Location& location, BigInt length)
    {
    	auto run_value_length = add_run_length0(location, length);

    	auto meta = this->metadata();
		meta->size() += length;

		return run_value_length;
    }

    void remove_run(const Location& location)
    {
    	Codec codec;
    	auto meta = this->metadata();

    	auto symbols = this->symbols();

    	codec.move(symbols, location.data_end(), location.data_pos(), meta->data_size() - location.data_end());

    	meta->data_size() -= location.data_length();
    	meta->size() -= location.length();

    	shrink_to_data();
    }

    size_t insert_run(const size_t& location, Int symbol, UBigInt length)
    {
    	Codec codec;
    	auto symbols = this->symbols();
    	auto meta    = this->metadata();

    	UBigInt run_value 		= encode_run(symbol, length);
    	size_t run_value_length = codec.length(run_value);

    	ensure_capacity(run_value_length);

    	codec.move(symbols, location, location + run_value_length, meta->data_size() - location);
    	codec.encode(symbols, run_value, location);

    	meta->data_size() += run_value_length;
    	meta->size() += length;

    	return run_value_length;
    }


};

template <typename T>
struct StructSizeProvider<PkdRLESeq<T>> {
    static const Int Value = PkdRLESeq<T>::Indexes;
};



}}
