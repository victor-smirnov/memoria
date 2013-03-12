
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_FSE_SEQUENCE_HPP_
#define MEMORIA_CORE_PACKED_FSE_SEQUENCE_HPP_

#include <memoria/core/packed2/packed_tree_base.hpp>
#include <memoria/core/packed2/packed_tree_walkers.hpp>
#include <memoria/core/tools/exint_codec.hpp>

#include <memoria/core/tools/accessors.hpp>

namespace memoria {

template <
	Int BitsPerSymbol_,
	typename V,
	typename Allocator_ = EmptyAllocator,
	Int BF = PackedTreeBranchingFactor,
	Int VPB = PackedTreeBranchingFactor
>
struct PackedFSESequenceTypes {
    typedef Int             IndexKey;
    typedef V               Value;
    typedef Allocator_  	Allocator;

    static const Int Blocks                 = 1<<BitsPerSymbol_;
    static const Int BranchingFactor        = BF;
    static const Int ValuesPerBranch        = VPB;
    static const Int BitsPerSymbol			= BitsPerSymbol_;
};



template <typename Types_>
class PackedFSESequence: public PackedTreeBase<PackedFSESequence<Types_>, Types_> {

	typedef PackedTreeBase<PackedFSESequence<Types_>, Types_>					Base;

public:
	static const UInt VERSION               									= 1;

	typedef Types_																Types;
	typedef PackedFSESequence<Types>               								MyType;

	typedef typename Types::Allocator											Allocator;

	typedef typename Types::IndexKey											IndexKey;
	typedef typename Types::Value												Value;

	static const Int BranchingFactor        = Types::BranchingFactor;
	static const Int ValuesPerBranch        = Types::ValuesPerBranch;
	static const Int Indexes        		= Types::Blocks;

	static const Int ALIGNMENT_BLOCK		= 8; //Bytes

private:

	Int size_;
	Int index_size_;
	Int max_size_;

	Value buffer_[];

public:
	PackedFSESequence() {}

	void setAllocatorOffset(const void* allocator)
	{
		const char* my_ptr = T2T<const char*>(this);
		const char* alc_ptr = T2T<const char*>(allocator);
		size_t diff = T2T<size_t>(my_ptr - alc_ptr);
		Base::allocator_offset() = diff;
	}

	Int& size() {return size_;}
	const Int& size() const {return size_;}

	const Int& index_size() const {return index_size_;}

	const Int& max_size() const {return max_size_;}




	IndexKey* indexes(Int index_block)
	{
		return T2T<IndexKey*>(buffer_ + index_size_ * sizeof(IndexKey) * (index_block));
	}

	const IndexKey* indexes(Int index_block) const
	{
		return T2T<const IndexKey*>(buffer_ + index_size_ * sizeof(IndexKey) * (index_block));
	}

	Value* getValues()
	{
		return buffer_ + index_size_ * sizeof(IndexKey) * Indexes;
	}

	const Value* getValues() const
	{
		return buffer_ + index_size_ * sizeof(IndexKey) * Indexes;
	}


	static Int getValueBlocks(Int items_num)
	{
		return items_num / ValuesPerBranch + (items_num % ValuesPerBranch ? 1 : 0);
	}

	Int getValueBlocks() const {
		return getValueBlocks(max_size_);
	}

	static Int getIndexSize(Int items_number)
	{
		if (items_number > ValuesPerBranch)
		{
			return MyType::template compute_index_size<MyType>(items_number);
		}
		else {
			return 0;
		}
	}

	static Int getBlockSize(Int items_num)
	{
		Int index_size = getIndexSize(items_num);

		return index_size * Indexes * sizeof(IndexKey) + items_num;
	}


	// ================================= Reindexing ======================================== //

private:

	class ReindexFn: public ReindexFnBase<MyType> {
		typedef ReindexFnBase<MyType> Base;
	public:
		ReindexFn(MyType& me): Base(me) {}

		void buildFirstIndexLine(Int index_level_start, Int index_level_size)
		{
			if (me_.index_size() == 0)
			{
				return;
			}

			Int limit = ValuesPerBranch - 1;
			Int block_num = 0;

			const Value* values = Base::me_.values();

			for (Int c = 0, block_num = 0; c < size(); c += ValuesPerBranch, blok_num++)
			{
				Int end;
				if (c + ValuesPerBranch < size())
				{
					end = c + ValuesPerBranch;
				}
				else {
					end = size();
				}

				Base::indexes_[1][block_num] = PopCount(values, c, end);
				Base::indexes_[0][block_num] = (end - c) - Base::indexes_[1];
			}
		}
	};



public:

	void reindex()
	{
		ReindexFn fn(*this);
		Base::reindex(0, size(), fn);
	}

	// ==================================== Value ========================================== //

	Value getValue(Int idx) const
	{
		return buffer_[idx];
	}

	void setValue(Int idx, Value value)
	{
		buffer_[idx] = value;
	}

	Value& value(Int idx)
	{
		return buffer_[idx];
	}

	Value& value(Int idx) const
	{
		return buffer_[idx];
	}

	BitmapAccessor<Value*, Value, BitsPerSymbol>
	operator[](Int idx) {
		return BitmapAccessor<Value*, Value, BitsPerSymbol>(buffer_, idx);
	}

	BitmapAccessor<const Value*, Value, BitsPerSymbol>
	operator[](Int idx) const {
		return BitmapAccessor<const Value*, Value, BitsPerSymbol>(buffer_, idx);
	}

private:
	class InitByBlockFn {
		const MyType& tree_;
	public:

		InitByBlockFn(const MyType& tree): tree_(tree) {}

		Int last(Int block_size) const {
			return block_size;
		}

		Int getBlockSize(Int items_number) const
		{
			return MyType::getBlockSize(items_number);
		}

		Int extend(Int items_number) const {
			return items_number;
		}

		Int getIndexSize(Int items_number) const {
			return MyType::template compute_index_size<MyType>(items_number);
		}
	};

public:
	void initByBlock(Int block_size)
	{
		size_ = 0;

		max_size_   = MyType::getMaxSize(block_size, InitByBlockFn(*this));
		index_size_ = getIndexSize(max_size_);
	}


	void initSizes(Int max)
	{
		size_       = 0;
		max_size_   = max;
		index_size_ = getIndexSize(max_size_);
	}

	// ==================================== Dump =========================================== //


	void dump(std::ostream& out = cout) const
	{
		out<<"size_       = "<<size_<<endl;
		out<<"max_size_   = "<<max_size_<<endl;
		out<<"index_size_ = "<<index_size_<<endl;
		out<<endl;

		Int idx_max = index_size_;

		out<<"Indexes:"<<endl;

		for (Int c = 0; c < idx_max; c++)
		{
			out<<c<<" ";

			for (Int idx = 0; idx < Indexes; idx++)
			{
				out<<this->indexes(idx)[c]<<" ";
			}

			out<<endl;
		}

		out<<endl;

		out<<"Data:"<<endl;

		Base::template dumpArray<Value>(out, size_, [&](Int idx) -> Value {
			return buffer_[idx];
		});
	}


};


}


#endif
