// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_FSE_MAP_HPP_
#define MEMORIA_CORE_PACKED_FSE_MAP_HPP_


#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/metadata/page.hpp>

#include <utility>

namespace memoria {

template <Int Bits>
struct LabelDescr {};




namespace internal {

template <typename Labels, Int Idx = 0>
class LabelDispatcherListBuilder;

template <Int Bits, typename... Tail, Int Idx>
class LabelDispatcherListBuilder<TypeList<LabelDescr<Bits>, Tail...>, Idx> {
	typedef PkdFSSeq<typename PkdFSSeqTF<Bits>::Type>							LabelStream;
public:
	 typedef typename MergeLists<
	            StreamDescr<LabelStream, Idx>,
	            typename LabelDispatcherListBuilder<
	                TypeList<Tail...>,
	                Idx + 1
	            >::Type
	 >::Result                                                                  Type;
};

template <Int Idx>
class LabelDispatcherListBuilder<TypeList<>, Idx> {
public:
	typedef TypeList<>															Type;
};



template <typename List> struct LabelsBlockSizeBuilder;

template <typename LabelStream, Int Idx, typename... Tail>
struct LabelsBlockSizeBuilder<TypeList<StreamDescr<LabelStream, Idx>, Tail...>> {
	static const Int Value = LabelStream::Indexes +
			LabelsBlockSizeBuilder<TypeList<Tail...>>::Value;
};

template <>
struct LabelsBlockSizeBuilder<TypeList<>> {
	static const Int Value = 0;
};


}




template <
    typename Key,
    typename Value_,
    Int Blocks      = 1,
    typename HiddenLabels_ = TypeList<>,
    typename Labels_ = TypeList<>
>
struct PackedFSEMapTypes: Packed2TreeTypes<Key, BigInt, Blocks> {
	using MapValue 		= Value_;
	using HiddenLabels 	= HiddenLabels_;
	using Labels 		= Labels_;
};

template <typename Types>
class PackedFSEMap: public PackedAllocator {

    typedef PackedAllocator                                                     Base;
public:
    typedef PackedFSEMap<Types>                                                 MyType;

    typedef PkdFTree<Types>                                                     Tree;

    typedef typename Types::Value                                            	Key;
    typedef typename Types::MapValue                                            Value;

    typedef typename Tree::Values                                               Values;
    typedef typename Tree::Values2                                              Values2;

    typedef typename Tree::IndexValue                                           IndexValue;
    typedef typename Tree::ValueDescr                                           ValueDescr;

    typedef std::pair<typename Tree::Values, Value>								IOValue;

    typedef IDataSource<IOValue>												DataSource;
    typedef IDataTarget<IOValue>												DataTarget;

    typedef typename Types::HiddenLabels										HiddenLabelsList;
    typedef typename Types::Labels												LabelsList;

    static const Int HiddenLabelsOffset											= 1;
    static const Int LabelsOffset												= HiddenLabelsOffset +
    																				ListSize<HiddenLabelsList>::Value;


    typedef typename memoria::internal::LabelDispatcherListBuilder<HiddenLabelsList>::Type 	HiddenLabelsStructsList;
    typedef typename memoria::internal::LabelDispatcherListBuilder<LabelsList>::Type 		LabelsStructsList;


    typedef typename PackedDispatcherTool<
    		HiddenLabelsOffset,
    		HiddenLabelsStructsList
    >::Type																		HiddenLabelsDispatcher;

    typedef typename PackedDispatcherTool<
    		HiddenLabelsOffset + ListSize<HiddenLabelsList>::Value,
    		LabelsStructsList
    >::Type																		LabelsDispatcher;


    static const Int LabelsIndexes 	= memoria::internal::LabelsBlockSizeBuilder<HiddenLabelsStructsList>::Value +
    								  memoria::internal::LabelsBlockSizeBuilder<LabelsStructsList>::Value;

    typedef StaticVector<BigInt, 1 + Tree::Blocks + LabelsIndexes>				MapSums;

    static const Int SizedIndexes												= MapSums::Indexes;
    static const Int Indexes													= SizedIndexes - 1;

    enum {TREE, ARRAY = LabelsOffset + ListSize<LabelsList>::Value};

    Tree* tree() {
        return Base::template get<Tree>(TREE);
    }

    const Tree* tree() const {
        return Base::template get<Tree>(TREE);
    }

    Value* values() {
        return Base::template get<Value>(ARRAY);
    }

    const Value* values() const {
        return Base::template get<Value>(ARRAY);
    }

    Int size() const
    {
        return tree()->size();
    }

    Value& value(Int idx)
    {
        return values()[idx];
    }

    const Value& value(Int idx) const
    {
        return values()[idx];
    }

    struct EmptySizeFn {
        Int size_ = 0;

        template <Int StreamIdx, typename Stream>
        void stream(Stream*)
        {
            size_ += Stream::empty_size();
        }
    };


    static Int empty_size()
    {
        Int allocator_size  = PackedAllocator::empty_size(2);
        Int tree_empty_size = Tree::empty_size();

        EmptySizeFn fn;

        HiddenLabelsDispatcher::dispatchAllStatic(fn);
        LabelsDispatcher::dispatchAllStatic(fn);

        return allocator_size + tree_empty_size + fn.size_;
    }

    static Int block_size(Int size)
    {
    	return packed_block_size(size);
    }

    Int block_size(const MyType* other) const
    {
        return block_size(size() + other->size());
    }

    Int block_size() const {
    	return Base::block_size();
    }



    struct PackedBlockSizeFn {
        Int size_ = 0;

        template <Int StreamIdx, typename Stream>
        void stream(Stream*, Int stream_size)
        {
            size_ += Stream::estimate_block_size(stream_size);
        }
    };


    static Int packed_block_size(Int size)
    {
    	Int tree_block_size = Tree::packed_block_size(size);
    	Int data_block_size = Base::roundUpBytesToAlignmentBlocks(sizeof(Value)*size);

    	PackedBlockSizeFn fn;

    	HiddenLabelsDispatcher::dispatchAllStatic(fn);
    	LabelsDispatcher::dispatchAllStatic(fn);

    	Int my_block_size 	= Base::block_size(tree_block_size + data_block_size + fn.size_, 2);

    	return my_block_size;
    }

private:
    struct ElementsForFn {
        Int block_size(Int items_number) const {
            return MyType::packed_block_size(items_number);
        }

        Int max_elements(Int block_size)
        {
            return block_size;
        }
    };

public:
    static Int elements_for(Int block_size)
    {
    	return FindTotalElementsNumber2(block_size, ElementsForFn());
    }

    void init()
    {
    	init(empty_size());
    }


    struct InitStructFn {
    	PackedAllocator* allocator_;
    	Int offset_;

    	InitStructFn(PackedAllocator* target, Int offset): allocator_(target), offset_(offset) {}

    	template <Int StreamIdx, typename Stream>
    	void stream(Stream*)
    	{
    		allocator_->template allocateEmpty<Stream>(StreamIdx + offset_);
    	}
    };

    void init(Int block_size)
    {
    	Base::init(block_size, 2 + ListSize<HiddenLabelsList>::Value + ListSize<LabelsList>::Value);

    	HiddenLabelsDispatcher::dispatchAllStatic(InitStructFn(this, HiddenLabelsOffset));
    	LabelsDispatcher::dispatchAllStatic(InitStructFn(this, LabelsOffset));

    	Base::template allocateEmpty<Tree>(TREE);
    	Base::template allocateArrayByLength<Value>(ARRAY, 0);
    }



    struct InsertZeroFn {

    	template <Int StreamIdx, typename Stream>
    	void stream(Stream* stream, Int idx)
    	{
    		stream->insert(idx, 0);
    	}
    };


    void insert(Int idx, const Values& keys, const Value& value)
    {
        Int size = this->size();

        tree()->insert(idx, keys);

        HiddenLabelsDispatcher::dispatchAll(this, InsertZeroFn(), idx);
        LabelsDispatcher::dispatchAll(this, InsertZeroFn(), idx);

        Int requested_block_size = (size + 1) * sizeof(Value);

        Base::resizeBlock(ARRAY, requested_block_size);

        Value* values = this->values();

        CopyBuffer(values + idx, values + idx + 1, size - idx);

        values[idx] = value;
    }


    template <typename Labels>
    struct InsertFn {
    	const Labels& labels_;

    	MapSums& sums_;
    	Int offset_;

    	InsertFn(const Labels& labels, MapSums& sums, Int offset):
    		labels_(labels),
    		sums_(sums),
    		offset_(offset)
    	{}

    	template <Int LabelIdx, typename Stream>
    	void stream(Stream* stream, Int idx)
    	{
    		stream->insert(idx, std::get<LabelIdx>(labels_));

    		sums_[offset_ + std::get<LabelIdx>(labels_)] += 1;
    	}
    };


    template <typename Entry>
    void insert(Int idx, const Entry& entry, MapSums& sums)
    {
    	Int size = this->size();

    	tree()->insert(idx, entry.keys());

    	sums[0] += 1;
    	sums.sumAt(1, entry.keys());

    	HiddenLabelsDispatcher::dispatchAll(
    			this,
    			InsertFn<typename Entry::HiddenLabelsType>(entry.hidden_labels(), sums, HiddenLabelsOffset + 1),
    			idx
    	);

    	LabelsDispatcher::dispatchAll(
    			this,
    			InsertFn<typename Entry::LabelsType>(entry.labels(), sums, LabelsOffset + 1),
    			idx
    	);

    	Int requested_block_size = (size + 1) * sizeof(Value);

    	Base::resizeBlock(ARRAY, requested_block_size);

    	Value* values = this->values();

    	CopyBuffer(values + idx, values + idx + 1, size - idx);

    	values[idx] = entry.value();
    }




    struct InsertSpaceFn {
      	template <Int StreamIdx, typename Stream>
    	void stream(Stream* stream, Int start, Int length)
    	{
    		stream->insertSpace(start, length);
    	}
    };


    void insertSpace(Int room_start, Int room_length)
    {
    	Int size = this->size();

    	MEMORIA_ASSERT(room_start, >=, 0);
    	MEMORIA_ASSERT(room_length, >=, 0);

    	tree()->insertSpace(room_start, room_length);

    	HiddenLabelsDispatcher::dispatchAll(this, InsertSpaceFn(), room_start, room_length);
    	LabelsDispatcher::dispatchAll(this, InsertSpaceFn(), room_start, room_length);

    	Int requested_block_size = (size + room_length) * sizeof(Value);

    	Base::resizeBlock(ARRAY, requested_block_size);

    	Value* values = this->values();

    	CopyBuffer(values + room_start, values + room_start + room_length, size - room_start);
    }


    void removeSpace(Int room_start, Int room_end)
    {
        remove(room_start, room_end);
    }



    struct RemoveFn {
    	template <Int StreamIdx, typename Stream>
    	void stream(Stream* stream, Int start, Int end)
    	{
    		stream->removeSpace(start, end);
    	}
    };


    void remove(Int room_start, Int room_end)
    {
        Int old_size = this->size();

        MEMORIA_ASSERT(room_start, >=, 0);
        MEMORIA_ASSERT(room_end, >=, room_start);
        MEMORIA_ASSERT(room_end, <=, old_size);

        Value* values = this->values();

        CopyBuffer(values + room_end, values + room_start, old_size - room_end);

        Int requested_block_size = (old_size - (room_end - room_start)) * sizeof(Value);

        Base::resizeBlock(values, requested_block_size);

        HiddenLabelsDispatcher::dispatchAll(this, RemoveFn(), room_start, room_end);
        LabelsDispatcher::dispatchAll(this, RemoveFn(), room_start, room_end);

        tree()->remove(room_start, room_end);

        tree()->reindex();
    }


    struct SplitToFn {
    	PackedAllocator* target_;
    	Int offset_;

    	SplitToFn(PackedAllocator* target, Int offset): target_(target), offset_(offset) {}

    	template <Int StreamIdx, typename Stream>
    	void stream(Stream* stream, Int idx)
    	{
    		Stream* tgt_stream = target_->template get<Stream>(StreamIdx + offset_);
    		stream->splitTo(tgt_stream, idx);
    	}
    };

    void splitTo(MyType* other, Int split_idx)
    {
        Int size = this->size();

        tree()->splitTo(other->tree(), split_idx);

        HiddenLabelsDispatcher::dispatchAll(this, SplitToFn(other, HiddenLabelsOffset), split_idx);
        LabelsDispatcher::dispatchAll(this, SplitToFn(other, LabelsOffset), split_idx);


        Int remainder = size - split_idx;

        other->template allocateArrayBySize<Value>(ARRAY, remainder);

        Value* other_values = other->values();
        Value* my_values    = this->values();

        CopyBuffer(my_values + split_idx, other_values, remainder);
    }

    struct MergeWithFn {
    	PackedAllocator* target_;
    	Int offset_;

    	MergeWithFn(PackedAllocator* target, Int offset): target_(target), offset_(offset) {}

    	template <Int StreamIdx, typename Stream>
    	void stream(Stream* stream)
    	{
    		Stream* tgt_stream = target_->template get<Stream>(StreamIdx + offset_);
    		stream->mergeWith(tgt_stream);
    	}
    };

    void mergeWith(MyType* other)
    {
        Int other_size  = other->size();
        Int my_size     = this->size();

        tree()->mergeWith(other->tree());

        HiddenLabelsDispatcher::dispatchAll(this, MergeWithFn(other, HiddenLabelsOffset));
        LabelsDispatcher::dispatchAll(this, MergeWithFn(other, LabelsOffset));

        Int other_values_block_size          = other->element_size(ARRAY);
        Int required_other_values_block_size = (my_size + other_size) * sizeof(Value);

        if (required_other_values_block_size >= other_values_block_size)
        {
            other->resizeBlock(ARRAY, required_other_values_block_size);
        }

        CopyBuffer(values(), other->values() + other_size, my_size);
    }



    struct ReindexFn {
    	template <Int StreamIdx, typename Stream>
    	void stream(Stream* stream)
    	{
    		stream->reindex();
    	}
    };


    void reindex()
    {
        tree()->reindex();

        HiddenLabelsDispatcher::dispatchAll(this, ReindexFn());
        LabelsDispatcher::dispatchAll(this, ReindexFn());
    }



    struct CheckFn {
    	template <Int StreamIdx, typename Stream>
    	void stream(const Stream* stream)
    	{
    		stream->check();
    	}
    };

    void check() const
    {
        tree()->check();

        HiddenLabelsDispatcher::dispatchAll(this, CheckFn());
        LabelsDispatcher::dispatchAll(this, CheckFn());
    }



    struct DumpFn {
    	template <Int StreamIdx, typename Stream>
    	void stream(const Stream* stream, std::ostream& out)
    	{
    		out<<"Stream: "<<StreamIdx<<std::endl;
    		stream->dump(out);
    	}
    };


    void dump(std::ostream& out = std::cout) const
    {
        tree()->dump(out);

        out<<"Hidden Labels:"<<std::endl;
        HiddenLabelsDispatcher::dispatchAll(this, DumpFn(), out);

        out<<"Labels:"<<std::endl;
        LabelsDispatcher::dispatchAll(this, DumpFn(), out);

        Int size = this->size();

        auto values = this->values();

        out<<"DATA"<<std::endl;

        for (Int c = 0; c < size; c++)
        {
            out<<c<<" "<<values[c]<<std::endl;
        }

        out<<std::endl;
    }


    ValueDescr findGEForward(Int block, Int start, IndexValue val) const
    {
        return this->tree()->findGEForward(block, start, val);
    }

    ValueDescr findGTForward(Int block, Int start, IndexValue val) const
    {
        return this->tree()->findGTForward(block, start, val);
    }

    ValueDescr findGEBackward(Int block, Int start, IndexValue val) const
    {
        return this->tree()->findGEForward(block, start, val);
    }

    ValueDescr findGTBackward(Int block, Int start, IndexValue val) const
    {
        return this->tree()->findGTForward(block, start, val);
    }


    ValueDescr findForward(SearchType search_type, Int block, Int start, IndexValue val) const
    {
    	if (search_type == SearchType::GT)
    	{
    		return findGTForward(block, start, val);
    	}
    	else {
    		return findGEForward(block, start, val);
    	}
    }

    ValueDescr findBackward(SearchType search_type, Int block, Int start, IndexValue val) const
    {
    	if (search_type == SearchType::GT)
    	{
    		return findGTBackward(block, start, val);
    	}
    	else {
    		return findGEBackward(block, start, val);
    	}
    }


    struct Sums0Fn {
    	Int idx_;
    	Sums0Fn(Int start): idx_(start) {}

    	template <Int StreamIdx, typename Stream>
    	void stream(const Stream* stream, MapSums& sums)
    	{
    		sums.sumAt(idx_, stream->sums());
    		idx_ += Stream::Indexes;
    	}
    };


    void sums(MapSums& sums) const
    {
        Values2 tree_sums;
    	tree()->sums(tree_sums);

    	sums.sumAt(0, tree_sums);

    	Sums0Fn fn(Values2::Indexes);

    	HiddenLabelsDispatcher::dispatchAll(this, fn, sums);
    	LabelsDispatcher::dispatchAll(this, fn, sums);
    }

    void sums(Values& values) const
    {
        tree()->sums(values);
    }

    void sums(Int from, Int to, Values& values) const
    {
        tree()->sums(from, to, values);
    }



    struct Sums2Fn {
    	Int idx_;
    	Sums2Fn(Int start): idx_(start) {}

    	template <Int StreamIdx, typename Stream>
    	void stream(const Stream* stream, Int from, Int to, MapSums& sums)
    	{
    		sums.sumAt(idx_, stream->sums(from, to));
    		idx_ += Stream::Indexes;
    	}
    };

    void sums(Int from, Int to, MapSums& sums) const
    {
        Values2 tree_sums;
    	tree()->sums(from, to, tree_sums);

    	sums.sumAt(0, tree_sums);

    	Sums2Fn fn(Values2::Indexes);

    	HiddenLabelsDispatcher::dispatchAll(this, fn, from, to, sums);
    	LabelsDispatcher::dispatchAll(this, fn, from, to, sums);
    }


    struct Sums1Fn {
    	Int idx_;
    	Sums1Fn(Int start): idx_(start) {}

    	template <Int StreamIdx, typename Stream>
    	void stream(const Stream* stream, Int idx, MapSums& sums)
    	{
    		sums.sumAt(idx_, stream->sumsAt(idx));
    		idx_ += Stream::Indexes;
    	}
    };

    void sums(Int idx, MapSums& sums) const
    {
        Values2 tree_sums;
        tree()->sums(idx, tree_sums);

        sums.sumAt(0, tree_sums);

        Sums1Fn fn(Values2::Indexes);

        HiddenLabelsDispatcher::dispatchAll(this, fn, idx, sums);
        LabelsDispatcher::dispatchAll(this, fn, idx, sums);
    }

    void sums(Int idx, Values& values) const
    {
    	tree()->sums(idx, values);
    }

    IndexValue sum(Int block) const {
        return tree()->sum(block);
    }

    IndexValue sum(Int block, Int to) const {
        return tree()->sum(block, to);
    }

    IndexValue sum(Int block, Int from, Int to) const {
        return tree()->sum(block, from, to);
    }

    IndexValue sumWithoutLastElement(Int block) const {
        return tree()->sumWithoutLastElement(block);
    }

    void addValue(Int block, Int idx, Key value)
    {
    	tree()->addValue(block, idx, value);
    }

    // ============================ IO =============================================== //


    void insert(IData* data, Int pos, Int length)
    {
    	DataSource* src = static_cast<DataSource*>(data);

    	MEMORIA_ASSERT_TRUE(to_bool(src->api() & IDataAPI::Single));
    	MEMORIA_ASSERT(src->getRemainder(), >=, length);

    	for (SizeT c = 0; c < length; c++)
    	{
    		IOValue v = src->get();

    		this->insert(pos + c, v.first, v.second);
    	}

    	reindex();
    }

    void update(IData* data, Int pos, Int length)
    {
        MEMORIA_ASSERT(pos, <=, size());
        MEMORIA_ASSERT(pos + length, <=, size());

        DataSource* src = static_cast<DataSource*>(data);

        MEMORIA_ASSERT_TRUE(to_bool(src->api() & IDataAPI::Single));
        MEMORIA_ASSERT(src->getRemainder(), >=, length);

        Tree* 	tree 	= this->tree();
        Value* 	array 	= this->values();

        for (SizeT c = pos; c < pos + length; c++)
        {
        	auto v = src->get();

        	for (Int d = 0; d < Values::Indexes; d++)
        	{
        		tree->value(d, c) 	= v.first[d];
        		array[c]			= v.second;
        	}
        }

        reindex();
    }

    void read(IData* data, Int pos, Int length) const
    {
        MEMORIA_ASSERT(pos, <=, size());
        MEMORIA_ASSERT(pos + length, <=, size());

        DataTarget* tgt = static_cast<DataTarget*>(data);

        MEMORIA_ASSERT_TRUE(to_bool(tgt->api() & IDataAPI::Single));
        MEMORIA_ASSERT(tgt->getRemainder(), >=, length);

        const Tree*  tree 	= this->tree();
        const Value* array 	= this->values();

        for (SizeT c = pos; c < pos + length; c++)
        {
        	IOValue v(tree->values(c), array[c]);
        	tgt->put(v);
        }
    }


    // ============================ Serialization ==================================== //

    struct GenerateDataEventsFn {
    	template <Int StreamIdx, typename Stream>
    	void stream(const Stream* stream, IPageDataEventHandler* handler)
    	{
    		stream->generateDataEvents(handler);
    	}
    };


    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        handler->startGroup("FSE_MAP");

        Base::generateDataEvents(handler);

        tree()->generateDataEvents(handler);

        HiddenLabelsDispatcher::dispatchAll(this, GenerateDataEventsFn(), handler);
        LabelsDispatcher::dispatchAll(this, GenerateDataEventsFn(), handler);

        handler->startGroup("DATA", size());

        auto values = this->values();

        for (Int idx = 0; idx < size(); idx++)
        {
            vapi::ValueHelper<Value>::setup(handler, values[idx]);
        }

        handler->endGroup();

        handler->endGroup();
    }

    struct SerializeFn {
    	template <Int StreamIdx, typename Stream>
    	void stream(const Stream* stream, SerializationData& buf)
    	{
    		stream->serialize(buf);
    	}
    };

    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        tree()->serialize(buf);

        HiddenLabelsDispatcher::dispatchAll(this, SerializeFn(), buf);
        LabelsDispatcher::dispatchAll(this, SerializeFn(), buf);

        FieldFactory<Value>::serialize(buf, values(), size());
    }

    struct DeserializeFn {
    	template <Int StreamIdx, typename Stream>
    	void stream(Stream* stream, DeserializationData& buf)
    	{
    		stream->deserialize(buf);
    	}
    };

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        tree()->deserialize(buf);

        HiddenLabelsDispatcher::dispatchAll(this, DeserializeFn(), buf);
        LabelsDispatcher::dispatchAll(this, DeserializeFn(), buf);

        FieldFactory<Value>::deserialize(buf, values(), size());
    }

};



}

#endif
