
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_FSE_QUICK_TREE_HPP_
#define MEMORIA_CORE_PACKED_FSE_QUICK_TREE_HPP_

#include <memoria/core/packed/tools/packed_allocator.hpp>

#include <memoria/core/packed/tree/packed_tree_walkers.hpp>
#include <memoria/core/packed/tree/packed_tree_tools.hpp>

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/dump.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/tools/accessors.hpp>
#include <memoria/core/tools/static_array.hpp>


#include <type_traits>

namespace memoria {

template <typename IndexValueT, typename ValueT = IndexValueT, Int kBranchingFactor = PackedTreeBranchingFactor, Int kValuesPerBranch = PackedTreeBranchingFactor>
class PkdFQTree: public PackedAllocator {

    using Base = PackedAllocator;

public:
    static constexpr UInt VERSION = 1;


    using MyType = PkdFQTree<IndexValueT, ValueT, kBranchingFactor, kValuesPerBranch>;

    using IndexValue 	= IndexValueT;
    using Value 		= ValueT;
    using TreeTools		= PackedTreeTools<kBranchingFactor, kValuesPerBranch, Int>;

    static const Int BranchingFactor        = kBranchingFactor;
    static const Int ValuesPerBranch        = kValuesPerBranch;

    static const bool FixedSizeElement      = true;

    static constexpr Int ValuesPerBranchMask 	= ValuesPerBranch - 1;
    static constexpr Int BranchingFactorMask 	= BranchingFactor - 1;

    static constexpr Int ValuesPerBranchLog2 	= Log2(ValuesPerBranch) - 1;
    static constexpr Int BranchingFactorLog2 	= Log2(BranchingFactor) - 1;

    static constexpr Int METADATA = 0;

    class Metadata {
    	Int size_;
    	Int index_size_;
    	Int max_size_;
    	Int blocks_;
    public:
    	Metadata() = default;

    	Int& size() {return size_;}
    	const Int& size() const {return size_;}

    	Int& index_size() {return index_size_;}
    	const Int& index_size() const {return index_size_;}

    	Int& max_size() {return max_size_;}
    	const Int& max_size() const {return max_size_;}

    	Int& blocks() {return blocks_;}
    	const Int& blocks() const {return blocks_;}

    	template <typename, typename, Int, Int> friend class PkdFQTree;
    };

    class BlockData {
    	Metadata* metadata_;
    	IndexValue* indexes_;
    	Value* values_;
    public:
    	BlockData(Metadata* metadata, IndexValue* indexes, Value* values):
    		metadata_(metadata), indexes_(indexes), values_(values)
    	{}
    };

    class ConstBlockData {
    	const Metadata* metadata_;
    	const IndexValue* indexes_;
    	const Value* values_;
    public:
    	ConstBlockData(const Metadata* metadata, const IndexValue* indexes, const Value* values):
    		metadata_(metadata), indexes_(indexes), values_(values)
    	{}

    	const Metadata* metadata() const {return metadata_;}
        const IndexValue* indexes() const {return indexes_;}
        const Value* values() const {return values_;}
    };

    struct TreeLayout {
    	const IndexValue* indexes = nullptr;
    	Int level_starts[32];
    	Int level_sizes[32];
    	Int levels_max = 0;
    	Int index_size = 0;
    };

    struct InitFn {
    	Int blocks_;

    	InitFn(Int blocks): blocks_(blocks) {}

    	Int block_size(Int items_number) const {
    		return MyType::block_size(blocks_, items_number);
    	}

    	Int max_elements(Int block_size)
    	{
    		return block_size;
    	}
    };

public:

    PkdFQTree() = default;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
				ConstValue<Int, kBranchingFactor>,
				ConstValue<Int, kValuesPerBranch>,
                decltype(Metadata::size_),
                decltype(Metadata::max_size_),
                decltype(Metadata::index_size_),
				decltype(Metadata::blocks_),
                IndexValue,
                Value
    >;

    void init(Int data_block_size, Int blocks)
    {
    	Base::init(data_block_size, blocks * 2 + 1);

    	Metadata* meta = this->template allocate<Metadata>(METADATA);

    	Int max_size        = FindTotalElementsNumber2(data_block_size, InitFn(blocks));

    	meta->size()        = 0;
    	meta->max_size()   	= max_size;
    	meta->index_size()  = MyType::index_size(max_size);
    	meta->blocks()		= blocks;

    	for (Int block = 0; block < blocks; block++)
    	{
    		this->allocateArrayBySize<IndexValue>(block * 2 + 1, meta->index_size());
    		this->allocateArrayBySize<Value>(block * 2 + 2, max_size);
    	}
    }

    void init(Int blocks)
    {
    	Base::init(block_size, blocks * 2 + 1);

    	Metadata* meta = this->template allocate<Metadata>(METADATA);

    	Int max_size        = 0;

    	meta->size()        = 0;
    	meta->data_size()   = 0;
    	meta->index_size()  = 0;
    	meta->blocks()		= blocks;

    	for (Int block = 0; block < blocks; block++)
    	{
    		this->allocateArrayByLength<IndexValue>(block * 2 + 1, meta->index_size());
    		this->allocateArrayByLength<IndexValue>(block * 2 + 2, max_size);
    	}
    }

    static Int block_size(Int blocks, Int capacity)
    {
    	Int metadata_length = Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

    	Int index_size      = MyType::index_size(capacity);
    	Int index_length    = Base::roundUpBytesToAlignmentBlocks(index_size * sizeof(IndexValue));

    	Int values_length   = Base::roundUpBytesToAlignmentBlocks(capacity * sizeof(Value));

    	return Base::block_size(metadata_length + (index_length + values_length) * blocks, blocks * 2 + 1);
    }

    static Int index_size(Int capacity)
    {
    	TreeLayout layout;
    	compute_tree_layout(capacity, layout);
    	return layout.index_size;
    }


    Metadata* metadata() {
    	return this->template get<Metadata>(METADATA);
    }
    const Metadata* metadata() const {
    	return this->template get<Metadata>(METADATA);
    }


    IndexValue* index(Int block) {
    	return this->template get<IndexValue>(block * 2 + 1);
    }
    const IndexValue* index(Int block) const {
    	return this->template get<IndexValue>(block * 2 + 1);
    }


    Value* values(Int block) {
    	return this->template get<Value>(block * 2 + 2);
    }
    const Value* values(Int block) const {
    	return this->template get<Value>(block * 2 + 2);
    }


    BlockData block_data(Int block) {
    	return BlockData(metadata(), index(block), values(block));
    }

    ConstBlockData block_data(Int block) const {
    	return ConstBlockData(metadata(), index(block), values(block));
    }


    class FindGEWalker {
    	IndexValue sum_ = 0;
    	IndexValue target_;

    	IndexValue next_;
    public:
    	FindGEWalker(IndexValue target): target_(target) {}

    	template <typename T>
    	bool compare(T value)
    	{
    		next_ = value;
    		return sum_ + next_ >= target_;
    	}

    	void next() {
    		sum_ += next_;
    	}
    };


    Int find_ge(Int block, Value value) const
    {
    	return find(block, FindGEWalker(value));
    }



    template <typename Walker>
    Int find(Int block, Walker&& walker) const
    {
    	auto metadata = this->metadata();
    	auto values = this->values(block);

    	Int size = metadata->size();

    	if (this->element_size(block * 2 + 1) == 0)
    	{
    		for (Int c = 0; c < size; c++)
    		{
    			if (walker.compare(values[c]))
    			{
    				return c;
    			}
    			else {
    				walker.next();
    			}
    		}

    		return size;
    	}
    	else {

    		TreeLayout data;

    		this->compute_tree_layout(metadata, data);

    		data.indexes = this->index(block);

    		Int idx = this->find_index(data, walker);

    		if (idx >= 0)
    		{
    			idx <<= ValuesPerBranchLog2;

    			for (Int c = idx; c < size; c++)
    			{
    				if (walker.compare(values[c]))
    				{
    					return c;
    				}
    				else {
    					walker.next();
    				}
    			}

    			return size;
    		}
    		else {
    			return size;
    		}
    	}
    }

    template <typename Walker>
    Int walk_fw(Int block, Int start, Walker&& walker) const
    {
    	auto metadata = this->metadata();
    	auto values = this->values(block);

    	Int size = metadata->size();
    	Int window_end = (start | ValuesPerBranchMask) + 1;

    	if (this->element_size(block * 2 + 1) == 0 || size <= window_end)
    	{
    		for (int c = start; c < size; c++)
    		{
    			if (walker.compare(values[c]))
    			{
    				return c;
    			}
    			else {
    				walker.next();
    			}
    		}

    		return size;
    	}
    	else {

    		TreeLayout data;

    		Int levels = this->compute_tree_layout(metadata, data);

    		data.levels_max = levels - 1;
    		data.indexes = this->index(block);

    		for (Int c = start; c < window_end; c++)
    		{
    			if (walker.compare(values[c]))
    			{
    				return c;
    			}
    			else {
    				walker.next();
    			}
    		}

    		Int window_start = this->walk_index_fw(
    				data,
					window_end >> ValuesPerBranchLog2,
					data.levels_max,
					std::forward<Walker>(walker)
			) << ValuesPerBranchLog2;

    		if (window_start >= 0)
    		{
    			for (Int c = window_start; c < size; c++)
    			{
    				if (walker.compare(values[c]))
    				{
    					return c;
    				}
    				else {
    					walker.next();
    				}
    			}

    			return size;
    		}
    		else {
    			return size;
    		}
    	}
    }

    void reindex()
    {
    	Metadata* meta = this->metadata();

    	TreeLayout layout;
    	Int levels = compute_tree_layout(meta, layout);

    	meta->index_size() = layout.index_size;

    	for (Int block = 0; block < meta->blocks(); block++)
    	{
    		this->resizeBlock(block * 2 + 1, layout.index_size * sizeof(IndexValue));
    		if (levels > 0)
    		{
    			this->reindex_block(meta, block, layout);
    		}
    	}
    }

    const Int& size() const {
    	return metadata()->size();
    }

    Int& size() {
    	return metadata()->size();
    }

    Int index_size() const {
    	return metadata()->index_size();
    }

    Int max_size() const {
    	return metadata()->max_size();
    }


    void dump(std::ostream& out = cout) const
    {
    	auto meta = this->metadata();

    	out<<"size_         = "<<meta->size()<<std::endl;
    	out<<"index_size_   = "<<meta->index_size()<<std::endl;

    	out<<std::endl;

    	TreeLayout layout;

    	Int levels = compute_tree_layout(meta->max_size(), layout);

    	if (levels > 0)
    	{
    		out<<"TreeLayout: "<<endl;

    		out<<"Level sizes: ";
    		for (Int c = 0; c <= layout.levels_max; c++) {
    			out<<layout.level_sizes[c]<<" ";
    		}
    		out<<endl;

    		out<<"Level starts: ";
    		for (Int c = 0; c <= layout.levels_max; c++) {
    			out<<layout.level_starts[c]<<" ";
    		}
    		out<<endl;



    	}

    	for (Int block = 0; block < meta->blocks(); block++)
    	{
    		out<<"++++++++++++++++++ Block: "<<block<<" ++++++++++++++++++"<<endl;

    		if (levels > 0)
    		{
    			auto indexes = this->index(block);

				out<<"Index:"<<endl;
    			for (Int c = 0; c < meta->index_size(); c++)
    			{
    				out<<c<<": "<<indexes[c]<<endl;
    			}
    		}

    		out<<endl;
    		out<<"Values: "<<endl;

    		auto values = this->values(block);

    		for (Int c = 0; c < meta->size(); c++)
    		{
    			out<<c<<": "<<values[c]<<endl;
    		}
    	}
    }


private:

    static constexpr Int divUpV(Int value) {
    	return (value >> ValuesPerBranchLog2) + ((value & ValuesPerBranchMask) ? 1 : 0);
    }

    static constexpr Int divUpI(Int value) {
    	return (value >> BranchingFactorLog2) + ((value & BranchingFactorMask) ? 1 : 0);
    }

    void reindex_block(const Metadata* meta, Int block, TreeLayout& layout)
    {
    	auto values = this->values(block);
    	auto indexes = this->index(block);
    	layout.indexes = indexes;

    	Int levels = layout.levels_max + 1;

    	Int level_start = layout.level_starts[levels - 1];
    	Int level_size = layout.level_sizes[levels - 1];

    	Int size = meta->size();

    	for (int i = 0; i < level_size; i++)
    	{
    		IndexValue sum = 0;

    		Int start = i << ValuesPerBranchLog2;
    		Int window_end = (i + 1) << ValuesPerBranchLog2;

    		Int end = window_end <= size ? window_end : size;

    		for (Int c = start; c < end; c++) {
    			sum += values[c];
    		}

    		indexes[level_start + i] = sum;
    	}

    	for (Int level = levels - 1; level > 0; level--)
    	{
    		Int previous_level_start = layout.level_starts[level - 1];
    		Int previous_level_size  = layout.level_sizes[level - 1];

    		Int current_level_start  = layout.level_starts[level];

    		for (int i = 0; i < previous_level_size; i++)
    		{
    			IndexValue sum = 0;

    			Int start = (i << BranchingFactorLog2) + current_level_start;
    			Int window_end = ((i + 1) << BranchingFactorLog2) + current_level_start;

    			Int end = window_end <= size ? window_end : size;

    			for (Int c = start; c < end; c++) {
    				sum += indexes[c];
    			}

    			indexes[previous_level_start + i] = sum;
    		}
    	}
    }

    Int compute_tree_layout(const Metadata* meta, TreeLayout& layout) const {
    	return compute_tree_layout(meta->max_size(), layout);
    }

    static Int compute_tree_layout(Int size, TreeLayout& layout)
    {
    	if (size <= ValuesPerBranch)
    	{
    		layout.levels_max = -1;
    		layout.index_size = 0;

    		return 0;
    	}
    	else {
    		Int level = 0;

    		layout.level_sizes[level] = divUpV(size);
    		level++;

    		while((layout.level_sizes[level] = divUpI(layout.level_sizes[level - 1])) > 1)
    		{
    			level++;
    		}

    		level++;

    		for (int c = 0; c < level / 2; c++)
    		{
    			auto tmp = layout.level_sizes[c];
    			layout.level_sizes[c] = layout.level_sizes[level - c - 1];
    			layout.level_sizes[level - c - 1] = tmp;
    		}

    		Int level_start = 0;

    		for (int c = 0; c < level; c++)
    		{
    			layout.level_starts[c] = level_start;
    			level_start += layout.level_sizes[c];
    		}

    		layout.index_size = level_start;
    		layout.levels_max = level - 1;

    		return level;
    	}
    }

    auto sum_idx(Int block, Int start, Int end) const
    {

    }

    void sum_index(IndexValue& sum, const IndexValue* indexes, const Int* layout, Int start, Int end, Int level) const
    {
    	Int level_start = layout[level];

    	Int branch_end = (start | BranchingFactorMask) + 1;
    	Int branch_start = end & ~BranchingFactorMask;

    	if (end <= branch_end || branch_start == branch_end)
    	{
    		for (Int c = start + level_start; c < end + level_start; c++)
    		{
    			sum += indexes[c];
    		}
    	}
    	else {
    		for (Int c = start + level_start; c < branch_end + level_start; c++)
    		{
    			sum += indexes[c];
    		}

    		sum_index(
    				sum,
					indexes,
					layout,
					branch_end >> BranchingFactorLog2,
					(branch_start - 1) >> BranchingFactorLog2,
					level - 1
    		);

    		for (Int c = branch_start + level_start; c < end + level_start; c++)
    		{
    			sum += indexes[c];
    		}
    	}
    }




    template <typename Walker>
    Int walk_index_fw(const TreeLayout& data, Int start, Int level, Walker&& walker) const
    {
    	Int level_start = data.level_starts[level];
    	Int level_size = data.level_sizes[level];

    	Int branch_end = (start | BranchingFactorMask) + 1;

    	if (level_size > branch_end)
    	{
    		for (Int c = level_start + start; c < branch_end; c++)
    		{
    			if (walker.compare(data.indexes[c]))
    			{
    				if (level < data.levels_max)
    				{
    					return walk_index_fw(
    							data,
								c << BranchingFactorLog2,
								level + 1
						);
    				}
    				else {
    					return c;
    				}
    			}
    			else {
    				walker.next();
    			}
    		}

    		if (level > 0)
    		{
    			return walk_index_fw(
    					data,
						branch_end >> BranchingFactorLog2,
						level - 1
    			);
    		}
    		else {
    			return -1;
    		}
    	}
    	else {
    		for (Int c = level_start + start; c < level_size; c++)
    		{
    			if (walker.compare(data.indexes[c])) {
    				return c;
    			}
    			else {
    				walker.next();
    			}
    		}

    		return -1;
    	}
    }

    template <typename Walker>
    Int find_index(const TreeLayout& data, Walker&& walker) const
    {
    	Int branch_start = 0;

    	for (Int level = 1; level <= data.levels_max; level++)
    	{
    		Int level_start = data.level_starts[level];

    		for (int c = level_start + branch_start; c < level_start + data.level_sizes[level]; c++)
    		{
    			if (walker.compare(data.indexes[c]))
    			{
    				if (level < data.levels_max)
    				{
    					branch_start = (c - branch_start) << BranchingFactorLog2;
    					goto next_level;
    				}
    				else {
    					return (c - level_start);
    				}
    			}
    			else {
    				walker.next();
    			}
    		}

    		return -1;

    		next_level:;
    	}

    	return -1;
    }
};


template <typename Types>
struct PkdStructSizeType<PkdFQTree<Types>> {
	static const PackedSizeType Value = PackedSizeType::FIXED;
};

template <typename T>
struct StructSizeProvider<PkdFQTree<T>> {
    static const Int Value = PkdFQTree<T>::Indexes;
};

}


#endif
