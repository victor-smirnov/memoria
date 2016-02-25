
// Copyright Victor Smirnov 2016+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_VLE_BIGMAX_TREE_HPP_
#define MEMORIA_CORE_PACKED_VLE_BIGMAX_TREE_HPP_

#include <memoria/core/packed/buffer/packed_vle_input_buffer_co.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_quick_tree_base.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_tools.hpp>


#include <memory>

namespace memoria {


class TextPageDumper;



template <typename> class ValueCodec;

template <
	typename ValueT,
	template <typename> class CodecT,
	Int kBranchingFactor
>
struct PkdVBMTreeTypes {
	using Value = ValueT;

	template <typename T>
	using Codec = CodecT<T>;
	static constexpr Int BranchingFactor = kBranchingFactor;
};

template <typename Types> class PkdVBMTree;

template <
	typename ValueT,
	template <typename> class CodecT = ValueCodec,
	Int kBranchingFactor = 1024
>
using PkdVBMTreeT = PkdVBMTree<PkdVBMTreeTypes<ValueT, CodecT, kBranchingFactor>>;


template <typename Types>
class PkdVBMTree: public PackedAllocator {

	using Base 		= PackedAllocator;
	using MyType 	= PkdVBMTree<Types>;

public:
    static constexpr UInt VERSION 	= 1;


    static constexpr Int Blocks 	= 1;

	static const Int BranchingFactorI        	= PackedTreeBranchingFactor;
	static const Int BranchingFactorV        	= Types::BranchingFactor;

    static constexpr Int BranchingFactorVMask 	= BranchingFactorV - 1;
    static constexpr Int BranchingFactorVLog2 	= Log2(BranchingFactorV) - 1;

    static constexpr Int BranchingFactorIMask 	= BranchingFactorI - 1;
    static constexpr Int BranchingFactorILog2 	= Log2(BranchingFactorI) - 1;

    enum {METADATA, INDEX, SIZE_INDEX, OFFSETS, VALUES, TOTAL_BLOCKS};


    class Metadata {
    	Int size_;
    	Int data_size_;
    public:
    	Int& size(){return size_;}
    	const Int& size() const {return size_;}

    	Int& data_size() {return data_size_;}
    	const Int& data_size() const {return data_size_;}
    };

    using FieldsList = MergeLists<>;

    using OffsetsType	= UShort;
    using SizesValue 	= Int;
    using Value		 	= typename Types::Value;
    using IndexValue	= typename Types::Value;

    using Values 		= core::StaticVector<Value, 1>;

    using InputBuffer 	= PkdVLEColumnOrderInputBuffer<Types>;
    using InputType 	= Values;

    using SizesT = core::StaticVector<Int, 1>;

    using Codec 		= typename Types::template Codec<Value>;

    using ValueData = typename Codec::BufferType;

    struct TreeLayout {
    	Int level_starts[8];
    	Int level_sizes[8];
    	Int levels_max = 0;
    	Int index_size = 0;

    	const Int* valaue_block_size_prefix;
    };

    struct LocateResult {
    	Int idx = 0;
    	Int index_cnt = 0;

    	LocateResult(Int idx_, Int index_cnt_ = 0) :
    		idx(idx_), index_cnt(index_cnt_)
    	{}

    	LocateResult() {}

    	Int local_cnt() const {return idx - index_cnt;}
    };


    void init()
    {
    	Base::init(empty_size(), TOTAL_BLOCKS);

    	Metadata* meta = this->template allocate<Metadata>(METADATA);

    	meta->size() = 0;
    	meta->data_size() = 0;

    	this->template allocateArrayBySize<Int>(SIZE_INDEX, 0);
    	this->template allocateArrayBySize<OffsetsType>(OFFSETS, number_of_offsets(0));
    	this->template allocateArrayBySize<ValueData>(VALUES, 0);
    }

    Int block_size() const
    {
    	return Base::block_size();
    }



    Metadata* metadata() {
    	return this->template get<Metadata>(METADATA);
    }

    const Metadata* metadata() const {
    	return this->template get<Metadata>(METADATA);
    }


    const Int& size() const {
    	return metadata()->size();
    }

    Int& size() {
    	return metadata()->size();
    }

    Value value(Int block, Int idx) const
    {
    	return value(idx);
    }



    Value value(Int idx) const
    {
    	MEMORIA_ASSERT(idx, >=, 0);
    	MEMORIA_ASSERT(idx, <, this->size());

    	auto meta 		  = this->metadata();

    	Int data_size	  = meta->data_size();
    	auto values 	  = this->values();
    	TreeLayout layout = compute_tree_layout(data_size);

		Int start_pos  	  = locate(layout, values, idx).idx;

		if (start_pos >= data_size) {
			int a = 0; a++;
			this->dump();
		}

		MEMORIA_ASSERT(start_pos, <, data_size);

		Codec codec;
		Value value;
		codec.decode(values, value, start_pos);

		return value;
    }


    static Int empty_size()
    {
    	Int metadata_length = Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

    	Int sizes_length	= 0;

    	Int values_length   = 0;
    	Int offsets_length 	= offsets_segment_size(0);

    	Int segments_length = values_length + offsets_length + sizes_length;


    	return PackedAllocator::block_size(
    			metadata_length +
				segments_length,
				TOTAL_BLOCKS
    	);
    }






//    bool check_capacity(Int size) const
//    {
//    	MEMORIA_ASSERT_TRUE(size >= 0);
//
//    	auto alloc = this->allocator();
//
//    	Int total_size          = this->size() + size;
//    	Int total_block_size    = MyType::block_size(total_size);
//    	Int my_block_size       = alloc->element_size(this);
//    	Int delta               = total_block_size - my_block_size;
//
//    	return alloc->free_space() >= delta;
//    }


    // ================================ Container API =========================================== //





    // ========================================= Insert/Remove/Resize ============================================== //





public:
    void splitTo(MyType* other, Int idx)
    {
    	auto meta = this->metadata();

    	TreeLayout layout 	= compute_tree_layout(meta->data_size());
    	auto values 		= this->values();

    	Codec codec;

    	Int start 		= locate(layout, values, idx).idx;
    	Int data_size  	= meta->data_size() - start;

    	other->insert_space(0, data_size);
    	codec.copy(values, start, other->values(), 0, data_size);

    	Int size 		= meta->size();
        other->size() 	+= size - idx;

        other->reindex();

        remove(idx, size);
    }


    void mergeWith(MyType* other)
    {
    	auto meta 		= this->metadata();
    	auto other_meta = other->metadata();

    	Int data_size 		= meta->data_size();
    	Int other_data_size = other_meta->data_size();
    	Int start 			= other_data_size;

    	other->insert_space(other_data_size, data_size);

    	Codec codec;
    	codec.copy(this->values(), 0, other->values(), start, data_size);

    	other->size() += meta->size();

    	other->reindex();

    	clear();
    }


    void removeSpace(Int start, Int end) {
    	remove(start, end);
    }

    void remove(Int start, Int end)
    {
    	auto meta 			= this->metadata();

    	auto values			= this->values();
    	TreeLayout layout 	= compute_tree_layout(meta->data_size());

    	Int start_pos = locate(layout, values, start).idx;
    	Int end_pos   = locate(layout, values, end).idx;

    	this->remove_space(start_pos, end_pos - start_pos);

    	meta->size() -= end - start;

    	reindex();
    }




    template <typename T>
    void insert(Int idx, const core::StaticVector<T, 1>& values)
    {
    	this->_insert(idx, 1, [&](Int block, Int idx) {
    		return values[block];
    	});
    }



    template <typename Adaptor>
    void insert(Int pos, Int processed, Adaptor&& adaptor) {
    	_insert(pos, processed, std::forward<Adaptor>(adaptor));
    }


    template <typename Adaptor>
    void _insert(Int pos, Int processed, Adaptor&& adaptor)
    {
    	auto meta = this->metadata();

    	Int size = meta->size();

    	MEMORIA_ASSERT(pos, >=, 0);
    	MEMORIA_ASSERT(pos, <=, size);
    	MEMORIA_ASSERT(processed, >=, 0);

    	Codec codec;

    	SizeT total_lengths = 0;

    	for (SizeT c = 0; c < processed; c++)
    	{
    		auto value 	= adaptor(0, c);
    		auto len 	= codec.length(value);
    		total_lengths += len;
    	}

    	Int data_size		= meta->data_size();
    	auto values			= this->values();
    	TreeLayout layout 	= compute_tree_layout(data_size);

    	auto lr = locate(layout, values, pos);

    	size_t insertion_pos = lr.idx;

    	insert_space(insertion_pos, total_lengths);

    	values = this->values();

    	for (Int c = 0; c < processed; c++)
    	{
    		auto value = adaptor(0, c);
    		auto len = codec.encode(values, value, insertion_pos);
    		insertion_pos += len;
    	}

    	meta->size() += processed;

    	reindex();
    }


    SizesT positions(Int idx) const
    {
    	auto meta = this->metadata();

    	MEMORIA_ASSERT(idx, >=, 0);
    	MEMORIA_ASSERT(idx, <=, meta->size());

    	Int data_size		= meta->data_size();
    	auto values			= this->values();
    	TreeLayout layout 	= compute_tree_layout(data_size);

    	return locate(layout, values, idx).idx;
    }


    SizesT insert_buffer(SizesT at, const InputBuffer* buffer, SizesT starts, SizesT ends, Int size)
    {
    	auto meta = this->metadata();

    	Codec codec;

    	SizesT total_lengths = ends - starts;


    	auto values	= this->values();

    	size_t insertion_pos = at[0];

    	insert_space(insertion_pos, total_lengths[0]);

    	values = this->values();

    	codec.copy(buffer->values(), starts[0], values, insertion_pos, total_lengths[0]);

    	meta->size() += size;

    	reindex();

    	return at + total_lengths;
    }

    void insert_buffer(Int pos, const InputBuffer* buffer, Int start, Int size)
    {
    	Codec codec;

    	SizesT starts = buffer->positions(start);
    	SizesT ends   = buffer->positions(start + size);

    	SizesT at 	  = this->positions(pos);

    	SizesT total_lengths = ends - starts;

    	auto values	= this->values();

    	size_t insertion_pos = at[0];

    	insert_space(insertion_pos, total_lengths[0]);

    	values = this->values();

    	codec.copy(buffer->values(), starts[0], values, insertion_pos, total_lengths[0]);

    	this->size() += size;

    	reindex();
    }



    template <typename Adaptor>
    SizesT populate(const SizesT& at, Int size, Adaptor&& adaptor)
    {
    	auto meta = this->metadata();

    	Codec codec;

    	SizesT total_lengths;

    	for (Int c = 0; c < size; c++)
    	{
    		total_lengths[0] += codec.length(adaptor(0, c));
    	}

    	size_t insertion_pos = at[0];

    	auto values = this->values();

    	for (Int c = 0; c < size; c++)
    	{
    		auto value = adaptor(0, c);
    		auto len = codec.encode(values, value, insertion_pos);
    		insertion_pos += len;
    	}

    	meta->data_size() += total_lengths[0];

    	meta->size() += size;

    	return at + total_lengths;
    }



    template <typename UpdateFn>
    void update_values(Int start, Int end, UpdateFn&& update_fn)
    {
    	auto meta = this->metadata();

    	Codec codec;

    	auto values			= this->values();
    	Int data_size 		= meta->data_size();
    	TreeLayout layout 	= compute_tree_layout(data_size);
    	size_t data_start	= locate(layout, values, start);

    	for (Int window_start = start; window_start < end; window_start += 32)
    	{
    		Int window_end = (window_start + 32) < end ? window_start + 32 : end;

    		Int old_length = 0;
    		Int new_length = 0;

    		auto values	= this->values();

    		size_t data_start_tmp = data_start;

    		Value buffer[32];

    		for (Int c = window_start; c < window_end; c++)
    		{
    			Value old_value;
    			auto len = codec.decode(values, old_value, data_start_tmp, data_size);

    			auto new_value = update_fn(0, c, old_value);

    			buffer[c - window_start] = new_value;

    			old_length += len;
    			new_length += codec.length(new_value);

    			data_start_tmp += len;
    		}

    		if (new_length > old_length)
    		{
    			auto delta = new_length - old_length;
    			insert_space(data_start, delta);

    			values = this->values();
    		}
    		else if (new_length < old_length)
    		{
    			auto delta = old_length - new_length;
    			remove_space(data_start, delta);

    			values = this->values();
    		}

    		for (Int c = window_start; c < window_end; c++)
    		{
    			data_start += codec.encode(values, buffer[c], data_start);
    		}
    	}

    	reindex();
    }


    template <typename UpdateFn>
    void update_values(Int start, UpdateFn&& update_fn)
    {
    	update_value(start, std::forward<UpdateFn>(update_fn));
    }


    template <typename UpdateFn>
    void update_value(Int start, UpdateFn&& update_fn)
    {
    	auto meta = this->metadata();

    	MEMORIA_ASSERT(start, <=, meta->size());
    	MEMORIA_ASSERT(start, >=, 0);

    	Codec codec;

    	Int data_size 			= meta->data_size();

    	auto values				= this->values();
    	TreeLayout layout 		= compute_tree_layout(data_size);
    	size_t insertion_pos 	= locate(layout, values, start).idx;

    	Value value;
    	size_t old_length = codec.decode(values, value, insertion_pos, data_size);
    	auto new_value = update_fn(0, value);

    	if (new_value != value)
    	{
    		size_t new_length = codec.length(new_value);

    		if (new_length > old_length)
    		{
    			insert_space(insertion_pos, new_length - old_length);
    			values = this->values();

    		}
    		else if (old_length > new_length)
    		{
    			remove_space(insertion_pos, old_length - new_length);
    			values = this->values();
    		}

    		codec.encode(values, new_value, insertion_pos);

    		reindex();
    	}
    }





    void clear()
    {
        if (Base::has_allocator())
        {
            auto alloc = this->allocator();
            Int empty_size = MyType::empty_size();
            alloc->resizeBlock(this, empty_size);
        }

        init();
    }

    void dump(std::ostream& out = std::cout) const
    {
    	std::unique_ptr<TextPageDumper> dumper = std::make_unique<TextPageDumper>(out);
    	this->generateDataEvents(dumper.get());
    }


    void generateDataEvents(IPageDataEventHandler* handler) const
    {
    	Base::generateDataEvents(handler);

    	handler->startStruct();
    	handler->startGroup("VBM_TREE");

    	if (has_index())
    	{
    		handler->startGroup("INDEX");
    		index()->generateDataEvents(handler);
    		handler->endGroup();
    	}

    	auto meta = this->metadata();

    	handler->value("SIZE",      &meta->size());
    	handler->value("DATA_SIZE", &meta->data_size());

		TreeLayout layout = compute_tree_layout(meta->data_size());

		if (layout.levels_max >= 0)
		{
			handler->startGroup("TREE_LAYOUT", layout.index_size);

			for (Int c = 0; c < layout.index_size; c++)
			{
				handler->value("LAYOUT_ITEM", &this->size_index()[c]);
			}

			handler->endGroup();
		}


		auto offsets_num = number_of_offsets(meta->data_size());
		handler->startGroup("OFFSETS", offsets_num);

		auto offsets = this->offsets();

		handler->value("OFFSETS", offsets, offsets_num, IPageDataEventHandler::BYTE_ARRAY);

		handler->endGroup();



    	handler->startGroup("DATA", meta->size());

    	const ValueData* values = this->values();

    	size_t position = 0;

    	Int size = meta->size();

    	Codec codec;

    	for (Int idx = 0; idx < size; idx++)
    	{
    		Value value;
    		auto len = codec.decode(values, value, position);
    		position += len;

    		handler->value("TREE_ITEM", &value);
    	}

    	handler->endGroup();

    	handler->endGroup();

    	handler->endStruct();
    }

    void serialize(SerializationData& buf) const
    {
    	Base::serialize(buf);

    	auto meta = this->metadata();

    	FieldFactory<Int>::serialize(buf, meta->size());
    	FieldFactory<Int>::serialize(buf, meta->data_size());

    	if (has_index())
    	{
    		index()->serialize(buf);
    	}

    	Base::template serializeSegment<SizesValue>(buf, SIZE_INDEX);
    	Base::template serializeSegment<OffsetsType>(buf, OFFSETS);

    	Int data_block_size = this->data_block_size();

    	FieldFactory<ValueData>::serialize(buf, this->values(), data_block_size);

    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        auto meta = this->metadata();

    	FieldFactory<Int>::deserialize(buf, meta->size());
    	FieldFactory<Int>::deserialize(buf, meta->data_size());

    	if (has_index())
    	{
    		index()->deserialize(buf);
    	}

    	Base::template deserializeSegment<Int>(buf, SIZE_INDEX);
    	Base::template deserializeSegment<OffsetsType>(buf, OFFSETS);

    	Int data_block_size = this->data_block_size();

    	FieldFactory<ValueData>::deserialize(buf, this->values(), data_block_size);
    }


    template <typename Walker>
    auto find(Walker&& walker) const
    {
    	auto meta = this->metadata();
    	auto values = this->values();

    	size_t data_size = meta->data_size();
    	Int size = meta->size();

    	Codec codec;

    	if (!this->has_index())
    	{
    		size_t pos = 0;
    		for (Int c = 0; pos < data_size; c++)
    		{
    			Value value;
    			size_t length = codec.decode(values, value, pos, data_size);

    			if (walker.compare(value))
    			{
    				return walker.idx(c);
    			}
    			else {
    				pos += length;
    				walker.next();
    			}
    		}

    		return walker.idx(size);
    	}
    	else {
    		Int index_size  = this->index()->size();
    		auto idx = this->index()->find(walker).idx();
    		if (idx < index_size)
    		{
        		size_t local_pos = (idx << BranchingFactorVLog2) + offset(idx);

        		TreeLayout layout = compute_tree_layout(data_size);
        		layout.valaue_block_size_prefix = this->size_index();

        		Int prefix = this->sum_index(layout, idx);

        		for (Int local_idx = prefix; local_pos < data_size; local_idx++)
        		{
        			Value value;
        			size_t length = codec.decode(values, value, local_pos, data_size);

        			if (walker.compare(value))
        			{
        				return walker.idx(local_idx);
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


    auto find_ge(const Value& value) const
    {
    	return find(FindGEWalker(value));
    }

    auto find_gt(const Value& value) const
    {
    	return find(FindGTWalker(value));
    }




    auto findGTForward(const Value& val) const
    {
    	return this->find_gt(val);
    }

    auto findGEForward(const Value& val) const
    {
    	return this->find_ge(val);
    }

    auto findGTForward(Int block, const Value& val) const
    {
    	return this->find_gt(val);
    }

    auto findGEForward(Int block, const Value& val) const
    {
    	return this->find_ge(val);
    }



    class FindResult {
    	Int idx_;
    public:
    	template <typename Fn>
    	FindResult(Fn&& fn): idx_(fn.idx()) {}
    	Int idx() const {return idx_;}
    };

    auto findForward(SearchType search_type, Int block, Int start, Value val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTForward(block, start, val));
        }
        else {
            return FindResult(findGEForward(block, start, val));
        }
    }

    auto findForward(SearchType search_type, Int block, Value val) const
    {
    	if (search_type == SearchType::GT)
    	{
    		return FindResult(findGTForward(block, val));
    	}
    	else {
    		return FindResult(findGEForward(block, val));
    	}
    }






    template <typename ConsumerFn>
    Int scan(Int block, Int start, Int end, ConsumerFn&& fn) const
    {
    	auto meta = this->metadata();

    	auto values 		= this->values();
    	TreeLayout layout 	= compute_tree_layout(meta->data_size());
    	size_t pos 			= locate(layout, values, start).idx;
    	size_t data_size 	= meta->data_size();

    	Codec codec;

    	Int c;
    	for (c = start; c < end && pos < data_size; c++)
    	{
    		Value value;
    		auto len = codec.decode(values, value, pos);
    		fn(c, value);
    		pos += len;
    	}

    	return c;
    }


    template <typename T>
    void read(Int block, Int start, Int end, T* values) const
    {
    	MEMORIA_ASSERT(start, >=, 0);
    	MEMORIA_ASSERT(start, <=, end);
    	MEMORIA_ASSERT(end, <=, this->size());

    	scan(start, end, [&](Int c, auto&& value){
    		values[c - start] = value;
    	});
    }

    void reindex()
    {
    	auto meta = this->metadata();

    	Int data_size 	  = meta->data_size();
    	TreeLayout layout = compute_tree_layout(data_size);

    	reindex(layout, meta);
    }

    void check() const
    {
    	auto meta = this->metadata();

    	Int data_size 		= meta->data_size();
    	TreeLayout layout 	= compute_tree_layout(data_size);

    	check(layout, meta);
    }




    struct FindGEWalker {
    	Value target_;
    	Int idx_ = 0;
    public:
    	FindGEWalker(const Value& target): target_(target) {}

    	template <typename T>
    	bool compare(T&& value)
    	{
    		return value >= target_;
    	}

    	void next() {}

    	Int& idx() {return idx_;}
    	const Int& idx() const {return idx_;}

    	FindGEWalker& idx(Int value) {
    		idx_ = value;
    		return *this;
    	}
    };

    struct FindGTWalker {
    	Value target_;

    	Int idx_;
    public:
    	FindGTWalker(const Value& target): target_(target) {}

    	template <typename T>
    	bool compare(T&& value)
    	{
    		return value > target_;
    	}

    	void next() {}

    	Int& idx() {return idx_;}
    	const Int& idx() const {return idx_;}

    	FindGTWalker& idx(Int value) {
    		idx_ = value;
    		return *this;
    	}
    };


protected:

    Int data_block_size() const
    {
    	Int size = this->element_size(VALUES);
    	return PackedAllocatable::roundUpBytesToAlignmentBlocks(size) / sizeof(ValueData);
    }



    bool has_index() const
    {
    	return this->element_size(INDEX) > 0;
    }


    MyType* index() {
    	return this->template get<MyType>(INDEX);
    }

    const MyType* index() const {
    	return this->template get<MyType>(INDEX);
    }

    Int* size_index() {
    	return this->template get<Int>(SIZE_INDEX);
    }

    const Int* size_index() const {
    	return this->template get<Int>(SIZE_INDEX);
    }

    OffsetsType offset(Int idx) const
    {
    	return offsets()[idx];
    }

    void set_offset(Int idx, OffsetsType value)
    {
    	offsets()[idx] = value;
    }

    void set_offset(OffsetsType* block, Int idx, Int value)
    {
    	block[idx] = value;
    }

    OffsetsType* offsets() {
    	return this->template get<OffsetsType>(OFFSETS);
    }

    const OffsetsType* offsets() const {
    	return this->template get<OffsetsType>(OFFSETS);
    }

    ValueData* values() {
    	return this->template get<ValueData>(VALUES);
    }
    const ValueData* values() const {
    	return this->template get<ValueData>(VALUES);
    }

    static constexpr Int number_of_offsets(Int values)
    {
    	return values > 0 ? divUpV(values) : 1;
    }

    static constexpr Int offsets_segment_size(Int values)
    {
    	return PackedAllocator::roundUpBytesToAlignmentBlocks(number_of_offsets(values) * sizeof(OffsetsType));
    }

    static constexpr Int divUpV(Int value) {
    	return (value >> BranchingFactorVLog2) + ((value & BranchingFactorVMask) ? 1 : 0);
    }

    static constexpr Int divUpI(Int value) {
    	return (value >> BranchingFactorILog2) + ((value & BranchingFactorIMask) ? 1 : 0);
    }

    template <Int Divisor>
    static constexpr Int divUp(Int value, Int divisor) {
    	return (value / Divisor) + ((value % Divisor) ? 1 : 0);
    }



    LocateResult locate(TreeLayout& layout, const ValueData* values, Int idx) const
    {
    	auto meta = this->metadata();

    	size_t data_size = meta->data_size();

    	if (data_size > 0)
    	{
    		LocateResult locate_result;

    		if (layout.levels_max >= 0)
    		{
    			layout.valaue_block_size_prefix = this->size_index();
    			locate_result = this->locate_index(layout, idx);
    		}

    		Int window_num = locate_result.idx;

    		Int window_start = window_num << BranchingFactorVLog2;
    		if (window_start >= 0)
    		{
    			Codec codec;

    			size_t offset = this->offset(window_num);

    			Int c = 0;
    			Int local_idx = idx - locate_result.index_cnt;
    			size_t pos;
    			for (pos = window_start + offset; pos < data_size && c < local_idx; c++)
    			{
    				auto len = codec.length(values, pos, data_size);
    				pos += len;
    			}

    			locate_result.idx = pos;

    			return locate_result;
    		}
    		else {
    			return LocateResult(data_size, locate_result.index_cnt);
    		}
    	}
    	else {
    		return LocateResult(0, 0);
    	}
    }


    LocateResult locate_index(TreeLayout& data, Int idx) const
    {
    	Int branch_start = 0;

    	Int sum = 0;

    	for (Int level = 1; level <= data.levels_max; level++)
    	{
    		Int level_start = data.level_starts[level];

    		for (int c = level_start + branch_start; c < level_start + data.level_sizes[level]; c++)
    		{
    			if (sum + data.valaue_block_size_prefix[c] > idx)
    			{
    				if (level < data.levels_max)
    				{
    					branch_start = (c - level_start) << BranchingFactorILog2;
    					goto next_level;
    				}
    				else {
    					return LocateResult(c - level_start, sum);
    				}
    			}
    			else {
    				sum += data.valaue_block_size_prefix[c];
    			}
    		}

    		return LocateResult(-1, sum);

    		next_level:;
    	}

    	return LocateResult(-1, sum);
    }

    Int sum_index(const TreeLayout& layout, Int end) const
    {
    	Int sum = 0;
    	sum_index(layout, sum, 0, end, layout.levels_max);
    	return sum;
    }


    void sum_index(const TreeLayout& layout, Int& sum, Int start, Int end, Int level) const
    {
    	Int level_start = layout.level_starts[level];

    	Int branch_end = (start | BranchingFactorIMask) + 1;
    	Int branch_start = end & ~BranchingFactorIMask;

    	if (end <= branch_end || branch_start == branch_end)
    	{
    		for (Int c = start + level_start; c < end + level_start; c++)
    		{
    			sum += layout.valaue_block_size_prefix[c];
    		}
    	}
    	else {
    		for (Int c = start + level_start; c < branch_end + level_start; c++)
    		{
    			sum += layout.valaue_block_size_prefix[c];
    		}

    		sum_index(
    				layout,
    				sum,
					branch_end >> BranchingFactorILog2,
					branch_start >> BranchingFactorILog2,
					level - 1
    		);

    		for (Int c = branch_start + level_start; c < end + level_start; c++)
    		{
    			sum += layout.valaue_block_size_prefix[c];
    		}
    	}
    }


    void resize(Int data_size, Int length)
    {
    	Int new_data_size = data_size + length;

    	Int data_segment_size 	 = PackedAllocator::roundUpBytesToAlignmentBlocks(new_data_size);
    	Int offsets_segment_size = this->offsets_segment_size(new_data_size);
    	Int index_size 	 	   	 = MyType::index_size(new_data_size);

    	this->resizeBlock(VALUES, data_segment_size);
    	this->resizeBlock(OFFSETS, offsets_segment_size);
    	this->resizeBlock(SIZE_INDEX, index_size * sizeof(SizesValue));
    }


    void insert_space(Int start, Int length)
    {
    	auto meta = this->metadata();

    	Int data_size = meta->data_size();
    	resize(data_size, length);

    	auto values = this->values();

    	Codec codec;
    	codec.move(values, start, start + length, data_size - start);

    	meta->data_size() += length;
    }



    void remove_space(Int start, Int length)
    {
    	auto meta = this->metadata();

    	Int data_size = meta->data_size();
    	auto values = this->values();

    	Codec codec;
    	Int end = start + length;
    	codec.move(values, end, start, data_size - end);

    	resize(data_size, -(end - start));

    	meta->data_size() -= (end - start);
    }

    static Int index_size(Int capacity)
    {
    	TreeLayout layout;
    	compute_tree_layout(capacity, layout);
    	return layout.index_size;
    }


    void reindex(TreeLayout& layout, Metadata* meta)
    {
    	if (layout.levels_max >= 0)
    	{
    		auto values 	= this->values();
    		auto size_index = this->size_index();
    		auto offsets 	= this->offsets();

    		Base::clear(SIZE_INDEX);
    		Base::clear(OFFSETS);

    		layout.valaue_block_size_prefix = size_index;

    		Int levels 		= layout.levels_max + 1;
    		Int level_start = layout.level_starts[levels - 1];
    		Int data_size 	= meta->data_size();

    		std::unique_ptr<ValueData[]> buffer = std::make_unique<ValueData[]>(data_size);

    		Codec codec;

    		size_t pos = 0;
    		SizesValue size_cnt = 0;
    		size_t threshold = BranchingFactorV;
    		size_t buffer_pos = 0;

    		set_offset(offsets, 0, 0);

    		Int idx = 0;

    		Value value;

    		while(pos < data_size)
    		{
    			if (pos >= threshold)
    			{
    				set_offset(offsets, idx + 1, pos - threshold);
    				size_index[level_start + idx] = size_cnt;

    				threshold += BranchingFactorV;

    				buffer_pos += codec.encode(buffer.get(), value, buffer_pos, data_size);

    				idx++;
    				size_cnt  = 0;
    			}

    			auto len = codec.decode(values, value, pos, data_size);

    			size_cnt++;

    			pos += len;
    		}

    		buffer_pos += codec.encode(buffer.get(), value, buffer_pos);
    		size_index[level_start + idx] = size_cnt;

    		idx++;

    		for (Int level = levels - 1; level > 0; level--)
    		{
    			Int previous_level_start = layout.level_starts[level - 1];
    			Int previous_level_size  = layout.level_sizes[level - 1];

    			Int current_level_start  = layout.level_starts[level];

    			Int current_level_size = layout.level_sizes[level];

    			for (int i = 0; i < previous_level_size; i++)
    			{
    				SizesValue sizes_sum  = 0;

    				Int start 		= (i << BranchingFactorILog2) + current_level_start;
    				Int window_end 	= ((i + 1) << BranchingFactorILog2);

    				Int end = (window_end <= current_level_size ? window_end : current_level_size) + current_level_start;

    				for (Int c = start; c < end; c++)
    				{
    					sizes_sum += size_index[c];
    				}

    				size_index[previous_level_start + i] = sizes_sum;
    			}
    		}

    		if (idx > 0)
    		{
    			this->template allocateEmpty<MyType>(INDEX);
    			index()->insert_data(0, buffer.get(), 0, buffer_pos, idx);
    		}
    		else {
    			Base::free(INDEX);
    		}
    	}
    	else {
    		Base::clear(OFFSETS);
    		Base::free(INDEX);
    	}
    }


    void check(TreeLayout& layout, const Metadata* meta) const
    {
    	Int data_size 	 = meta->data_size();
    	Int offsets_size = this->element_size(OFFSETS);

    	if (layout.levels_max >= 0)
    	{
    		MEMORIA_ASSERT(this->element_size(SIZE_INDEX), >, 0);

    		auto values 	= this->values();
    		auto size_index = this->size_index();

    		layout.valaue_block_size_prefix = size_index;

    		Int levels = layout.levels_max + 1;

    		Int level_start = layout.level_starts[levels - 1];

    		Codec codec;

    		size_t pos = 0;
    		SizesValue size_cnt = 0;
    		size_t threshold = BranchingFactorV;
    		Int total_size = 0;

    		MEMORIA_ASSERT(offset(0), ==, 0);

    		auto index = has_index() ? this->index() : nullptr;

    		Value value;

    		Int idx = 0;
    		while(pos < data_size)
    		{
    			if (pos >= threshold)
    			{
    				MEMORIA_ASSERT(offset(idx + 1), ==, pos - threshold);
    				MEMORIA_ASSERT(size_index[level_start + idx], ==, size_cnt);

    				threshold += BranchingFactorV;

    				if (index)
    				{
    					MEMORIA_ASSERT(value, ==, index->value(idx));
    				}

    				idx++;

    				total_size += size_cnt;

    				size_cnt  = 0;
    			}

    			auto len = codec.decode(values, value, pos, data_size);

    			size_cnt++;

    			pos += len;
    		}

    		MEMORIA_ASSERT((Int)pos, ==, data_size);

    		if (index) {
    			MEMORIA_ASSERT(value, ==, index->value(idx));
    		}

    		MEMORIA_ASSERT(size_index[level_start + idx], ==, size_cnt);
    		MEMORIA_ASSERT(meta->size(), ==, size_cnt + total_size);

    		for (Int level = levels - 1; level > 0; level--)
    		{
    			Int previous_level_start = layout.level_starts[level - 1];
    			Int previous_level_size  = layout.level_sizes[level - 1];

    			Int current_level_start  = layout.level_starts[level];

    			Int current_level_size = layout.level_sizes[level];

    			for (int i = 0; i < previous_level_size; i++)
    			{
    				SizesValue sizes_sum  = 0;

    				Int start 		= (i << BranchingFactorILog2) + current_level_start;
    				Int window_end 	= ((i + 1) << BranchingFactorILog2);

    				Int end = (window_end <= current_level_size ? window_end : current_level_size) + current_level_start;

    				for (Int c = start; c < end; c++)
    				{
    					sizes_sum += size_index[c];
    				}

    				MEMORIA_ASSERT(size_index[previous_level_start + i], ==, sizes_sum);
    			}
    		}

    		if (index) {
    			index->check();
    		}
    	}
    	else {
    		MEMORIA_ASSERT(this->element_size(SIZE_INDEX), ==, 0);

    		if (data_size > 0)
    		{
    			MEMORIA_ASSERT(offsets_size, ==, Base::roundUpBytesToAlignmentBlocks(sizeof(OffsetsType)));
    			MEMORIA_ASSERT(offset(0), ==, 0);
    		}
    		else {
    			MEMORIA_ASSERT(offsets_size, ==, 0);
    		}

    		MEMORIA_ASSERT(meta->data_size(), <=, BranchingFactorV);
    	}
    }

    void insert_data(Int at, const ValueData* buffer, Int start, Int end, Int size)
    {
    	auto meta = this->metadata();

    	Codec codec;

    	Int data_size = end - start;

    	auto values	= this->values();

    	size_t insertion_pos = at;

    	insert_space(insertion_pos, data_size);

    	values = this->values();

    	codec.copy(buffer, start, values, insertion_pos, data_size);

    	meta->size() += size;

    	reindex();
    }

    static TreeLayout compute_tree_layout(Int size)
    {
    	TreeLayout layout;
    	compute_tree_layout(size, layout);
    	return layout;
    }

    static Int compute_tree_layout(Int size, TreeLayout& layout)
    {
    	if (size <= BranchingFactorV)
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


};




template <typename Types>
struct PkdStructSizeType<PkdVBMTree<Types>> {
	static const PackedSizeType Value = PackedSizeType::VARIABLE;
};


template <typename Types>
struct StructSizeProvider<PkdVBMTree<Types>> {
    static const Int Value = 1;
};

template <typename Types>
struct IndexesSize<PkdVBMTree<Types>> {
	static const Int Value = 1;
};


}


#endif
