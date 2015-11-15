
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_VLE_QUICK_TREE_BASE_HPP_
#define MEMORIA_CORE_PACKED_VLE_QUICK_TREE_BASE_HPP_

#include <memoria/core/packed/tree/packed_vle_quick_tree_base_base.hpp>


namespace memoria {

class PkdVQTreeMetadata {
	Int size_;
	Int index_size_;
	Int data_size_;
public:
	PkdVQTreeMetadata() = default;

	Int& size() {return size_;}
	const Int& size() const {return size_;}

	Int& index_size() {return index_size_;}
	const Int& index_size() const {return index_size_;}

	Int& data_size() {return data_size_;}
	const Int& data_size() const {return data_size_;}

	template <typename, Int, Int, Int, typename> friend class PkdVQTreeBase1;
	template <typename, typename, template <typename> class Codec, Int, Int> friend class PkdVQTreeBase;
};



template <
	typename IndexValueT,
	typename ValueT,
	template <typename> class CodecT,
	Int kBranchingFactor,
	Int kValuesPerBranch
>
class PkdVQTreeBase: public PkdVQTreeBaseBase<IndexValueT, kBranchingFactor, kValuesPerBranch, 4, PkdFQTreeMetadata> {

	using Base 		= PkdVQTreeBaseBase<IndexValueT, kBranchingFactor, kValuesPerBranch, 4, PkdFQTreeMetadata>;
	using MyType 	= PkdVQTreeBase<IndexValueT, ValueT, CodecT, kBranchingFactor, kValuesPerBranch>;

public:
    static constexpr UInt VERSION = 1;

    using IndexValue 	= IndexValueT;
    using Value 		= ValueT;
    using TreeTools		= PackedTreeTools<kBranchingFactor, kValuesPerBranch, Int>;

    using Metadata		= typename Base::Metadata;
    using TreeLayout	= typename Base::TreeLayout;
    using OffsetsType	= UBigInt;

    using Codec 		= CodecT<Value>;

    static const Int BranchingFactor        = kBranchingFactor;
    static const Int ValuesPerBranch        = kValuesPerBranch;

    static const bool FixedSizeElement      = false;

    static constexpr Int ValuesPerBranchMask 	= ValuesPerBranch - 1;
    static constexpr Int BranchingFactorMask 	= BranchingFactor - 1;

    static constexpr Int ValuesPerBranchLog2 	= Log2(ValuesPerBranch) - 1;
    static constexpr Int BranchingFactorLog2 	= Log2(BranchingFactor) - 1;

    static constexpr Int SegmentsPerBlock = 4;


    static constexpr Int BITS_PER_OFFSET        = Codec::BitsPerOffset;
    static constexpr Int BITS_PER_DATA_VALUE    = Codec::ElementSize;

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

    PkdVQTreeBase() = default;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
				Value
    >;

    static constexpr Int value_segment_size(Int values)
    {
    	return PackedAllocatable::roundUpBitsToAlignmentBlocks(values * BITS_PER_DATA_VALUE);
    }

    static constexpr Int number_of_offsets(Int values)
    {
    	return Base::template divUpV(values);
    }

    static constexpr Int offsets_segment_size(Int values)
    {
    	return PackedAllocator::roundUpBitsToAlignmentBlocks(number_of_offsets(values) * BITS_PER_OFFSET);
    }

    static constexpr Int index1_segment_size(Int index_size) {
    	return PackedAllocator::roundUpBytesToAlignmentBlocks(index_size * sizeof(IndexValue));
    }

    static constexpr Int index2_segment_size(Int index_size) {
    	return PackedAllocator::roundUpBytesToAlignmentBlocks(index_size * sizeof(int));
    }


    void init(Int data_block_size, Int blocks)
    {
    	Base::init(data_block_size, blocks * SegmentsPerBlock + 1);

    	Metadata* meta = this->template allocate<Metadata>(Base::METADATA);

    	Int max_size        = FindTotalElementsNumber2(data_block_size, InitFn(blocks));

    	meta->size()        = 0;
    	meta->max_size()   	= max_size;
    	meta->index_size()  = MyType::index_size(max_size);

    	Int offsets_size = offsets_segment_size(max_size);

    	for (Int block = 0; block < blocks; block++)
    	{
    		this->template allocateArrayBySize<IndexValue>(block * SegmentsPerBlock + Base::VALUE_INDEX, meta->index_size());
    		this->template allocateArrayBySize<Int>(block * SegmentsPerBlock + Base::SIZE_INDEX, meta->index_size());
    		this->template allocateArrayBySize<Byte>(block * SegmentsPerBlock + Base::OFFSETS, offsets_size);
    		this->template allocateArrayBySize<Value>(block * SegmentsPerBlock + Base::VALUES, max_size);
    	}
    }

    void init(Int blocks)
    {
    	Base::init(block_size, blocks * SegmentsPerBlock + 1);

    	Metadata* meta = this->template allocate<Metadata>(Base::METADATA);

    	meta->size()        = 0;
    	meta->data_size()   = 0;
    	meta->index_size()  = 0;

    	for (Int block = 0; block < blocks; block++)
    	{
    		this->template allocateArrayBySize<IndexValue>(block * SegmentsPerBlock + Base::VALUE_INDEX, 0);
    		this->template allocateArrayBySize<Value>(block * SegmentsPerBlock + Base::SIZE_INDEX, 0);
    		this->template allocateArrayBySize<Byte>(block * SegmentsPerBlock + Base::OFFSETS, 0);
    		this->template allocateArrayBySize<Value>(block * SegmentsPerBlock + Base::VALUES, 0);
    	}
    }

    static Int block_size(Int blocks, Int capacity)
    {
    	Int metadata_length = Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

    	Int index_size      = MyType::index_size(capacity);
    	Int index_length    = Base::roundUpBytesToAlignmentBlocks(index_size * sizeof(IndexValue));

    	Int values_length   = Base::roundUpBytesToAlignmentBlocks(capacity * sizeof(Value));

    	Int offsets_length 	= offsets_segment_size(capacity);

    	Int sizes_length	= Base::roundUpBytesToAlignmentBlocks(index_size * sizeof(Int));

    	return Base::block_size(metadata_length + (index_length + values_length + offsets_length + sizes_length) * blocks, blocks * SegmentsPerBlock + 1);
    }

    Int block_size() const
    {
    	return Base::block_size();
    }

    static Int index_size(Int capacity)
    {
    	TreeLayout layout;
    	Base::compute_tree_layout(capacity, layout);
    	return layout.index_size;
    }

    static Int tree_size(Int blocks, Int block_size)
    {
    	return block_size >= (Int)sizeof(Value) ? FindTotalElementsNumber2(block_size, InitFn(blocks)) : 0;
    }


    Metadata* metadata() {
    	return this->template get<Metadata>(Base::METADATA);
    }
    const Metadata* metadata() const {
    	return this->template get<Metadata>(Base::METADATA);
    }

    Int offset(Int block, Int idx) const
    {
    	GetBits(offsets(block), idx, BITS_PER_OFFSET);
    }

    void set_offset(Int block, Int idx, Int value)
    {
    	SetBits(offsets(block), idx, value, BITS_PER_OFFSET);
    }

    void set_offset(OffsetsType* block, Int idx, Int value)
    {
    	SetBits(block, idx, value, BITS_PER_OFFSET);
    }

    OffsetsType* offsets(Int block) {
    	return this->template get<OffsetsType>(block * SegmentsPerBlock + Base::OFFSETS);
    }

    const OffsetsType* offsets(Int block) const {
    	return this->template get<OffsetsType>(block * SegmentsPerBlock + Base::OFFSETS);
    }

    Value* values(Int block) {
    	return this->template get<Value>(block * SegmentsPerBlock + Base::VALUES);
    }
    const Value* values(Int block) const {
    	return this->template get<Value>(block * SegmentsPerBlock + Base::VALUES);
    }

    Value& value(Int block, Int idx) {
    	return values(block)[idx];
    }

    const Value& value(Int block, Int idx) const {
    	return values(block)[idx];
    }

    bool has_index() const {
    	return this->element_size(1) > 0;
    }

    struct FindGEWalker {
    	IndexValue sum_ = 0;
    	IndexValue target_;

    	IndexValue next_;

    	Int idx_;
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

    	Int& idx() {return idx_;}
    	const Int& idx() const {return idx_;}

    	FindGEWalker& idx(Int value) {
    		idx_ = value;
    		return *this;
    	}

    	IndexValue prefix() const {
    		return sum_;
    	}
    };

    struct FindGTWalker {
    	IndexValue sum_ = 0;
    	IndexValue target_;

    	IndexValue next_;

    	Int idx_;
    public:
    	FindGTWalker(IndexValue target): target_(target) {}

    	template <typename T>
    	bool compare(T value)
    	{
    		next_ = value;
    		return sum_ + next_ > target_;
    	}

    	void next() {
    		sum_ += next_;
    	}

    	Int& idx() {return idx_;}
    	const Int& idx() const {return idx_;}

    	FindGTWalker& idx(Int value) {
    		idx_ = value;
    		return *this;
    	}

    	IndexValue prefix() const {
    		return sum_;
    	}
    };



    auto find_ge(Int block, IndexValue value) const
    {
    	return find(block, FindGEWalker(value));
    }

    auto find_gt(Int block, IndexValue value) const
    {
    	return find(block, FindGTWalker(value));
    }

    auto find_ge_fw(Int block, Int start, IndexValue value) const
    {
    	return walk_fw(block, start, FindGEWalker(value));
    }

    auto find_gt_fw(Int block, Int start, IndexValue value) const
    {
    	return walk_fw(block, start, FindGTWalker(value));
    }


    auto find_ge_bw(Int block, Int start, IndexValue value) const
    {
    	return walk_bw(block, start, FindGEWalker(value));
    }

    auto find_gt_bw(Int block, Int start, IndexValue value) const
    {
    	return walk_bw(block, start, FindGTWalker(value));
    }

    class Location {
    	Int window_start_;
    	Int size_prefix_;
    	Int pos_;
    public:
    	Location(Int window_start, Int size_prefix, Int pos):
    		window_start_(window_start), size_prefix_(size_prefix), pos_(pos)
    	{}

    	Int window_start() const {return window_start_;}
    	Int size_prefix() const {return size_prefix_;}
    	Int pos() const {return pos_;}
    };

    Int locate(Int block, Int idx) const
    {
    	auto meta = this->metadata();
    	auto values = this->values(block);

    	TreeLayout layout;
    	this->compute_tree_layout(meta, layout);

    	return locate(layout, meta, values, block, idx);
    }

    Int locate(TreeLayout layout, const Metadata meta, const Value* values, Int block, Int idx) const
    {
    	size_t data_size = meta->data_size();

    	if (layout.levels_max >= 0)
    	{
    		layout.valaue_block_size_prefix = this->size_index(block);

    		Int window_num= this->locate_index(layout, idx);

    		Int window_start = window_num << ValuesPerBranchLog2;
    		if (window_start >= 0)
    		{
    			Codec codec;

    			size_t offset = this->offset(block, window_num);

    			Int c = layout.size_prefix_sum;
    			for (size_t pos = window_start + offset; pos < data_size && c < idx; c++)
    			{
    				auto len = codec.length(values, pos, data_size);
    				pos += len;
    			}

    			return c;
    		}
    		else {
    			return data_size;
    		}
    	}
    }


    Int locate_with_sum(TreeLayout& layout, const Metadata meta, const Value* values, Int block, Int idx) const
    {
    	layout.value_sum = 0;
    	layout.size_prefix_sum = 0;

    	size_t data_size = meta->data_size();

    	if (layout.levels_max >= 0)
    	{
    		layout.valaue_block_size_prefix = this->size_index(block);
    		layout.indexes = this->value_index(block);

    		Int window_num= this->locate_index(layout, idx);

    		Int window_start = window_num << ValuesPerBranchLog2;
    		if (window_start >= 0)
    		{
    			Codec codec;

    			size_t offset = this->offset(block, window_num);

    			Int c = layout.size_prefix_sum;
    			for (size_t pos = window_start + offset; pos < data_size && c < idx; c++)
    			{
    				Value value;
    				auto len = codec.decode(values, value, pos, data_size);
    				pos += len;
    				layout.value_sum += value;
    			}

    			return c;
    		}
    		else {
    			return data_size;
    		}
    	}
    }


    template <typename Walker>
    auto find(Int block, Walker&& walker) const
    {
    	auto metadata = this->metadata();
    	auto values = this->values(block);

    	size_t data_size = metadata->data_size();
    	Int size = metadata->size();

    	Codec codec;

    	if (this->element_size(block * SegmentsPerBlock + 1) == 0)
    	{
    		size_t pos = 0;
    		for (Int c = 0; pos < data_size; c++)
    		{
    			Value value;
    			size_t length = codec.decode(values, value, pos, data_size);

    			if (walker.compare(value))
    			{
    				return walker.idx(c, value, pos);
    			}
    			else {
    				pos += length;
    				walker.next();
    			}
    		}

    		return walker.idx(size);
    	}
    	else {
    		TreeLayout data;

    		this->compute_tree_layout(metadata, data);

    		data.indexes = this->value_index(block);

    		Int idx = this->find_index(data, walker);

    		if (idx >= 0)
    		{
    			size_t local_pos = idx << ValuesPerBranchLog2  + this->offset(idx);

    			for (Int local_idx = data.size_prefix_sum; local_pos < data_size; local_idx++)
    			{
        			Value value;
        			size_t length = codec.decode(values, value, local_pos, data_size);

    				if (walker.compare(value))
    				{
    					return walker.idx(local_idx, value, local_pos);
    				}
    				else {
    					local_pos += length;
    					walker.next();
    				}
    			}

    			return walker.idx(size);
    		}
    		else {
    			return walker.idx(size);
    		}
    	}
    }



    template <typename Walker>
    auto walk_fw(Int block, Int start, Walker&& walker) const
    {
    	auto metadata = this->metadata();
    	auto values = this->values(block);

    	TreeLayout layout;
    	this->compute_tree_layout(metadata, layout);

    	Int size = metadata->size();
    	Int data_size = metadata->data_size();

    	size_t pos = this->locate(layout, metadata, values, block, start);

    	if (pos < data_size)
    	{
    		Codec codec;

    		if (layout.levels_max < 0 || data_size - pos  < ValuesPerBranch)
    		{
    			for (Int c = start; pos < data_size; c++)
    			{
    				Value value;
    				auto len = codec.decode(values, value, pos, data_size);

    				if (walker.compare(value))
    				{
    					return walker.idx(c, value, pos);
    				}
    				else {
    					pos += len;
    					walker.next();
    				}
    			}

    			return walker.idx(size, 0, data_size);
    		}
    		else {
    			size_t window_end = (pos | ValuesPerBranchMask) + 1;

    			Int c = start;
    			for (c = start; pos < window_end; c++)
    			{
    				Value value;
    				auto len = codec.decode(values, value, pos, data_size);

    				if (walker.compare(value))
    				{
    					return walker.idx(c, value, pos);
    				}
    				else {
    					pos += len;
    					walker.next();
    				}
    			}

    			layout.size_prefix_sum = start;

    			layout.indexes = this->value_index(block);

    			Int idx = this->walk_index_fw(
    					layout,
						window_end >> ValuesPerBranchLog2,
						layout.levels_max,
						std::forward<Walker>(walker)
    			);

    			if (idx >= 0)
    			{
    				size_t local_pos = idx << ValuesPerBranchLog2  + this->offset(idx);

    				for (Int local_idx = layout.size_prefix_sum; local_pos < data_size; local_idx++)
    				{
    					Value value;
    					size_t length = codec.decode(values, value, local_pos, data_size);

    					if (walker.compare(value))
    					{
    						return walker.idx(local_idx, value, local_pos);
    					}
    					else {
    						local_pos += length;
    						walker.next();
    					}
    				}

    				return walker.idx(size, 0, data_size);
    			}
    			else {
    				return walker.idx(size, 0, data_size);
    			}
    		}
    	}
    	else {
    		return walker.idx(size, 0, data_size);
    	}
    }


    template <typename Walker>
    auto walk_bw(Int block, Int start, Walker&& walker) const
    {
    	auto metadata = this->metadata();
    	auto values = this->values(block);

    	Int data_size = metadata->data_size();

    	Codec codec;

    	TreeLayout layout;
    	this->compute_tree_layout(metadata, layout);

    	size_t pos = this->locate(layout, metadata, values, start);

    	ValueT value_data[ValuesPerBranch];

    	if (pos < ValuesPerBranch)
    	{
    		size_t local_pos = 0;

    		for (Int c = 0; c < start; c++)
    		{
    			Value value;
    			local_pos += codec.decode(values, value, local_pos, data_size);
    			value_data[c] = value;
    		}

    		for (Int c = start; c >= 0; c--)
    		{
    			if (walker.compare(values[c]))
    			{
    				return walker.idx(c, values[c], -1);
    			}
    			else {
    				walker.next();
    			}
    		}

    		return walker.idx(-1);
    	}
    	else {
    		size_t window_start = (pos & ~ValuesPerBranchMask);
    		size_t window_end   = (pos | ValuesPerBranchMask) + 1;

    		size_t local_pos = window_start;

    		for (Int c = 0; local_pos < window_end && c < start; c++)
    		{
    			Value value;
    			local_pos += codec.decode(values, value, local_pos, data_size);
    			value_data[c] = value;
    		}

    		for (Int c = start; c > window_end; c--)
    		{
    			if (walker.compare(values[c]))
    			{
    				return walker.idx(c, values[c], -1);
    			}
    			else {
    				walker.next();
    			}
    		}

    		layout.indexes = this->value_index(block);

    		Int idx = this->walk_index_bw(
    				layout,
					(window_start >> ValuesPerBranchLog2) - 1,
					layout.levels_max,
					std::forward<Walker>(walker)
    		);

    		if (idx >= 0)
    		{
        		size_t window_start = (pos & ~ValuesPerBranchMask);
        		size_t window_end   = (pos | ValuesPerBranchMask) + 1;

        		size_t local_pos = window_start;

        		Int c;
        		for (c = 0; local_pos < window_end; c++)
        		{
        			Value value;
        			local_pos += codec.decode(values, value, local_pos, data_size);
        			value_data[c] = value;
        		}

        		c--;
    			for (; c >= 0; c--)
    			{
    				if (walker.compare(values[c]))
    				{
    					return walker.idx(c, values[c], 0);
    				}
    				else {
    					walker.next();
    				}
    			}

    			return walker.idx(-1, 0, 0);
    		}
    		else {
    			return walker.idx(-1, 0, 0);
    		}
    	}
    }





    IndexValue sum(Int block) const
    {
    	auto meta = this->metadata();
    	TreeLayout layout;
    	compute_tree_layout(meta, layout);

    	return sum(layout, meta, block, meta->size());
    }



    IndexValue sum(Int block, Int end) const
    {
    	auto meta = this->metadata();
    	TreeLayout layout;
    	compute_tree_layout(meta, layout);

    	return sum(layout, meta, block, end);
    }

    IndexValue sum(Int block, Int start, Int end) const
    {
    	auto meta = this->metadata();
    	TreeLayout layout;
    	compute_tree_layout(meta, layout);

    	return sum(layout, meta, block, start, end);
    }

    IndexValue sum(TreeLayout& layout, const Metadata* meta, Int block, Int end) const
    {
    	if (end < meta->size())
    	{
    		auto* values = this->values(block);
    		this->locate_with_sum(layout, meta, values, block, end);
    	}
    	else if (has_index())
    	{
    		auto index = this->index(block);
    		return index[0];
    	}
    	else {
    		auto* values = this->values(block);

    		Codec codec;
    		size_t pos = 0;
    		size_t data_size = meta->data_size();

    		IndexValue sum = 0;

    		for(int c  = 0; pos < data_size && c < end; c++)
    		{
    			Value value;
    			pos += codec.decode(values, value, pos, data_size);
    			sum += value;
    		}

    		return sum;
    	}
    }

    IndexValue sum(TreeLayout& layout, const Metadata* meta, Int block, Int start, Int end) const
    {
    	auto end_sum = sum(layout, meta, block, end);
    	auto start_sum = sum(layout, meta, block, start);

    	return end - start;
    }


    void reindex(Int blocks)
    {
    	Metadata* meta = this->metadata();

    	TreeLayout layout;
    	Int levels = this->compute_tree_layout(meta, layout);

    	meta->index_size() = layout.index_size;

    	for (Int block = 0; block < blocks; block++)
    	{
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

    void dump_index(Int blocks, std::ostream& out = cout) const
    {
    	auto meta = this->metadata();

    	out<<"size_         = "<<meta->size()<<std::endl;
    	out<<"index_size_   = "<<meta->index_size()<<std::endl;

    	out<<std::endl;

    	TreeLayout layout;

    	Int levels = this->compute_tree_layout(meta->max_size(), layout);

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

    	for (Int block = 0; block < blocks; block++)
    	{
    		out<<"++++++++++++++++++ Block: "<<block<<" ++++++++++++++++++"<<endl;

    		if (levels > 0)
    		{
    			auto indexes = this->value_index(block);

    			out<<"Index:"<<endl;
    			for (Int c = 0; c < meta->index_size(); c++)
    			{
    				out<<c<<": "<<indexes[c]<<endl;
    			}
    		}
    	}

    }


    void dump(Int blocks, std::ostream& out = cout) const
    {
    	auto meta = this->metadata();

    	out<<"size_         = "<<meta->size()<<std::endl;
    	out<<"index_size_   = "<<meta->index_size()<<std::endl;

    	out<<std::endl;

    	TreeLayout layout;

    	Int levels = this->compute_tree_layout(meta->max_size(), layout);

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

    	for (Int block = 0; block < blocks; block++)
    	{
    		out<<"++++++++++++++++++ Block: "<<block<<" ++++++++++++++++++"<<endl;

    		if (levels > 0)
    		{
    			auto value_index = this->value_index(block);
    			auto size_index  = this->size_index(block);

				out<<"Index:"<<endl;
    			for (Int c = 0; c < meta->index_size(); c++)
    			{
    				out<<c<<": "<<value_index[c]<<" "<<size_index[c]<<endl;
    			}

    			out<<"Offsets: ";
    			for (Int c = 0; c < Base::divUpV(meta->data_size()); c++)
    			{
    				out<<this->offset(block, c)<<", ";
    			}
    			out<<endl;
    		}

    		out<<endl;
    		out<<"Values: "<<endl;

    		auto values = this->values(block);

    		Codec codec;

    		size_t pos = 0;

    		for (Int c = 0; pos < meta->data_size(); c++)
    		{
    			Value value;
    			auto len = codec.decode(values, value, pos, meta->data_size());

    			out<<c<<": "<<pos<<" "<<values[c]<<endl;

    			pos += len;
    		}
    	}
    }



    auto findGTForward(Int block, Int start, IndexValue val) const
    {
        return this->find_gt_fw(block, start, val);
    }



    auto findGTBackward(Int block, Int start, IndexValue val) const
    {
    	return this->find_gt_bw(block, start, val);
    }



    auto findGEForward(Int block, Int start, IndexValue val) const
    {
    	return this->find_ge_fw(block, start, val);
    }

    auto findGEBackward(Int block, Int start, IndexValue val) const
    {
    	return this->find_ge_bw(block, start, val);
    }

    class FindResult {
    	IndexValue prefix_;
    	Int idx_;
    public:
    	template <typename Fn>
    	FindResult(Fn&& fn): prefix_(fn.prefix()), idx_(fn.idx()) {}

    	IndexValue prefix() {return prefix_;}
    	Int idx() const {return idx_;}
    };

    auto findForward(SearchType search_type, Int block, Int start, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTForward(block, start, val));
        }
        else {
            return FindResult(findGEForward(block, start, val));
        }
    }

    auto findBackward(SearchType search_type, Int block, Int start, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTBackward(block, start, val));
        }
        else {
            return FindResult(findGEBackward(block, start, val));
        }
    }








protected:
    void reindex_block(const Metadata* meta, Int block, TreeLayout& layout)
    {
    	auto values = this->values(block);
    	auto indexes = this->index(block);
    	auto size_index = this->size_index(block);
    	auto offsets = this->offsets(block);

    	this->clear(block * SegmentsPerBlock + Base::VALUE_INDEX);
    	this->clear(block * SegmentsPerBlock + Base::SIZE_INDEX);
    	this->clear(block * SegmentsPerBlock + Base::OFFSETS);

    	layout.indexes = indexes;
    	layout.valaue_block_size_prefix = size_index;

    	Int levels = layout.levels_max + 1;

    	Int level_start = layout.level_starts[levels - 1];

    	Int data_size = meta->data_size();

    	Codec codec;

    	size_t pos = 0;
    	IndexValueT value_sum = 0;
    	Int size_cnt = 0;
    	size_t threshold = ValuesPerBranch;

    	set_offset(offsets, 0, 0);

    	Int idx = 0;
    	while(pos < data_size)
    	{
    		if (pos >= threshold)
    		{
    			set_offset(offsets, idx + 1, pos - threshold);

    			indexes[level_start + idx] = value_sum;
    			size_index[level_start + idx] = size_cnt;

    			threshold += ValuesPerBranch;

    			idx++;
    		}

    		Value value;
    		auto len = codec.decode(values, value, pos, data_size);

    		value_sum += value;
    		size_cnt++;

    		pos += len;
    	}

    	indexes[level_start + idx] = value_sum;
    	size_index[level_start + idx] = size_cnt;

    	for (Int level = levels - 1; level > 0; level--)
    	{
    		Int previous_level_start = layout.level_starts[level - 1];
    		Int previous_level_size  = layout.level_sizes[level - 1];

    		Int current_level_start  = layout.level_starts[level];

    		Int current_level_size = layout.level_sizes[level];

    		for (int i = 0; i < previous_level_size; i++)
    		{
    			IndexValue sum = 0;

    			Int start 		= (i << BranchingFactorLog2) + current_level_start;
    			Int window_end 	= ((i + 1) << BranchingFactorLog2);

    			Int end = (window_end <= current_level_size ? window_end : current_level_size) + current_level_start;

    			for (Int c = start; c < end; c++) {
    				sum += indexes[c];
    			}

    			indexes[previous_level_start + i] = sum;
    		}
    	}
    }
};

}


#endif
