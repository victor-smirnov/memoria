
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_NODE_FACTORY2_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_NODE_FACTORY2_HPP

#include <memoria/core/types/typehash.hpp>
#include <memoria/core/types/algo/select.hpp>
#include <memoria/core/tools/reflection.hpp>

#include <memoria/core/types/types.hpp>
#include <memoria/core/packed2/packed_fse_tree.hpp>
#include <memoria/core/packed2/packed_allocator.hpp>
#include <memoria/core/packed2/packed_dispatcher.hpp>

#include <memoria/prototypes/balanced_tree/baltree_tools.hpp>
#include <memoria/prototypes/balanced_tree/baltree_types.hpp>


namespace memoria    	{
namespace balanced_tree {


using memoria::BitBuffer;

template <
    	template <typename, bool, bool> class,
    	typename,
    	bool, bool
>
class NodePageAdaptor;

template <typename Base_>
class TreeNodeBase: public Base_ {
public:
    static const UInt VERSION = 1;

private:

    Int root_;
    Int leaf_;
    Int bitmap_;
    Int level_;

    Int size_;

public:



    enum {
        LEAF          = 0,
        ROOT          = 1,
        BITMAP        = 2
    }                                       	FLAGS;

    typedef Base_                               Base;
    typedef TreeNodeBase<Base>                  Me;
    typedef Me 									BasePageType;

    typedef typename Base::ID                   ID;

    TreeNodeBase(): Base() {}

    inline bool is_root() const {
        return root_;
    }

    void set_root(bool root) {
        root_ = root;
    }

    inline bool is_leaf() const {
        return leaf_;
    }

    void set_leaf(bool leaf) {
        leaf_ = leaf;
    }

    inline bool isBitmap() const {
        return bitmap_;
    }

    void setBitmap(bool bitmap) {
        bitmap_ = bitmap;
    }

    Int size() const
    {
        return size_;
    }

    Int children_count() const
    {
        return size_;
    }

    const Int& level() const
    {
    	return level_;
    }

    Int& level()
    {
    	return level_;
    }

protected:
    Int& map_size()
    {
        return size_;
    }
public:

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        handler->value("ROOT", 		&root_);
        handler->value("LEAF", 		&leaf_);
        handler->value("BITMAP",	&bitmap_);
        handler->value("SIZE", 		&size_);
        handler->value("LEVEL", 	&level_);
    }

    template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
        Base::template serialize<FieldFactory>(buf);

        FieldFactory<Int>::serialize(buf, root_);
        FieldFactory<Int>::serialize(buf, leaf_);
        FieldFactory<Int>::serialize(buf, bitmap_);
        FieldFactory<Int>::serialize(buf, level_);

        FieldFactory<Int>::serialize(buf, size_);
    }

    template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
        Base::template deserialize<FieldFactory>(buf);

        FieldFactory<Int>::deserialize(buf, root_);
        FieldFactory<Int>::deserialize(buf, leaf_);
        FieldFactory<Int>::deserialize(buf, bitmap_);
        FieldFactory<Int>::deserialize(buf, level_);

        FieldFactory<Int>::deserialize(buf, size_);
    }

    void copyFrom(const Me* page)
    {
        Base::copyFrom(page);

        this->set_root(page->is_root());
        this->set_leaf(page->is_leaf());
        this->setBitmap(page->isBitmap());

        this->level() = page->level();

        //???
        this->size_  = page->size();
    }
};




template <typename Metadata, typename Base, bool root>
class RootPage: public Base {
	Metadata root_metadata_;
public:

	Metadata& root_metadata()
	{
		return root_metadata_;
	}

	const Metadata& root_metadata() const
	{
		return root_metadata_;
	}

	void generateDataEvents(IPageDataEventHandler* handler) const
	{
		Base::generateDataEvents(handler);
		root_metadata_.generateDataEvents(handler);
	}

	template <template <typename> class FieldFactory>
	void serialize(SerializationData& buf) const
	{
		Base::template serialize<FieldFactory>(buf);

		FieldFactory<Metadata>::serialize(buf, root_metadata_);
	}

	template <template <typename> class FieldFactory>
	void deserialize(DeserializationData& buf)
	{
		Base::template deserialize<FieldFactory>(buf);

		FieldFactory<Metadata>::deserialize(buf, root_metadata_);
	}
};


template <typename Metadata, typename Base>
class RootPage<Metadata, Base, false>: public Base {

};


namespace internl1 {

template <typename T>
struct ValueHelper {
    static void setup(IPageDataEventHandler* handler, const T& value)
    {
        handler->value("VALUE", &value);
    }
};

template <typename T>
struct ValueHelper<PageID<T> > {
    typedef PageID<T>                                                   Type;

    static void setup(IPageDataEventHandler* handler, const Type& value)
    {
        IDValue id(&value);
        handler->value("VALUE", &id);
    }
};

template <>
struct ValueHelper<EmptyValue> {
    typedef EmptyValue Type;

    static void setup(IPageDataEventHandler* handler, const Type& value)
    {
        BigInt val = 0;
        handler->value("VALUE", &val);
    }
};

}




template <
	typename Types,
	bool root, bool leaf
>
class TreeMapNode: public RootPage<typename Types::Metadata, typename Types::NodePageBase, root>
{

    static const Int  BranchingFactor                                           = PackedTreeBranchingFactor;

    typedef TreeMapNode<Types, root, leaf>                                      Me;
    typedef TreeMapNode<Types, root, leaf>                                      MyType;

public:
    static const UInt VERSION                                                   = 1;

    typedef RootPage<
    			typename Types::Metadata,
    			typename Types::NodePageBase, root
    >  																			Base;

private:



public:

    typedef typename Types::Accumulator											Accumulator;
    typedef typename Types::Position											Position;


    typedef typename IfThenElse<
    			leaf,
    			typename Types::Value,
    			typename Types::ID
    >::Result 																	Value;
    typedef typename Types::Key                                                 Key;



    template <
        	template <typename, bool, bool> class,
        	typename,
        	bool, bool
    >
    friend class NodePageAdaptor;


	typedef PackedFSETreeTypes<
			Key,Key,Key
	>																			TreeTypes;

	typedef typename PackedStructListBuilder<
	    		TreeTypes,
	    		typename Types::StreamDescriptors
	>::NonLeafStructList														StreamsStructList;

	typedef typename ListHead<StreamsStructList>::Type::Type 					Tree;

	typedef typename PackedDispatcherTool<StreamsStructList>::Type				Dispatcher;

private:

    PackedAllocator allocator_;

public:


    static const long INDEXES                                                   = Tree::Indexes;

    static const Int ValuesBlockIdx												= ListSize<StreamsStructList>::Value;

    TreeMapNode(): Base() {}


private:
	struct InitFn {
		Int block_size(Int items_number) const
		{
			return MyType::block_size(items_number);
		}

		Int max_elements(Int block_size)
		{
			return block_size / 4;
		}
	};

public:

	template <typename T>
	T* get(Int idx)
	{
		return allocator_.template get<T>(idx);
	}

	template <typename T>
	const T* get(Int idx) const
	{
		return allocator_.template get<T>(idx);
	}

	Tree* tree() {
		return tree0();
	}

	const Tree* tree() const {
		return tree0();
	}

	Tree* tree0() {
		return allocator_.template get<Tree>(0);
	}

	const Tree* tree0() const {
		return allocator_.template get<Tree>(0);
	}

	Value* values() {
		return allocator_.template get<Value>(ValuesBlockIdx);
	}

	const Value* values() const {
		return allocator_.template get<Value>(ValuesBlockIdx);
	}

	Int capacity() const {
		return tree()->capacity();
	}

private:
	struct TreeSizeFn {
		Int size_ = 0;

		template <Int Idx, typename Node>
		void operator()(Node*, Int tree_size)
		{
			size_ += Node::block_size(tree_size);
		}
	};

public:
	static Int block_size(Int tree_size)
	{
		TreeSizeFn fn;

		Dispatcher::dispatchAllStatic(fn, tree_size);

		Int tree_block_size 	= fn.size_;
		Int array_block_size 	= PackedAllocator::roundUpBytesToAlignmentBlocks(tree_size * sizeof(Value));

		Int client_area = tree_block_size + array_block_size;

		return PackedAllocator::block_size(client_area, ValuesBlockIdx + 1);
	}

	static Int max_tree_size(Int block_size)
	{
		return FindTotalElementsNumber2(block_size, InitFn());
	}

	static Int max_tree_size_for_block(Int block_size)
	{
		return max_tree_size(block_size - sizeof(Me) + sizeof(allocator_));
	}

	void init(Int block_size)
	{
		init0(block_size - sizeof(Me) + sizeof(allocator_));
	}

private:

	struct InitStructFn {
		template <Int Idx, typename Tree>
		void operator()(Tree*, Int tree_size, PackedAllocator* allocator)
		{
			Int tree_block_size = Tree::block_size(tree_size);
			allocator->template allocate<Tree>(Idx, tree_block_size);
		}

		template <Int Idx>
		void operator()(Value*, Int tree_size, PackedAllocator* allocator)
		{
			allocator->template allocateArrayBySize<Value>(Idx, tree_size);
		}
	};

public:

	void init0(Int block_size)
	{
		allocator_.init(block_size, 2);

		Int tree_size = max_tree_size(block_size);

		Dispatcher::dispatchAllStatic(InitStructFn(), tree_size, &allocator_);

		allocator_.template allocateArrayBySize<Value>(ValuesBlockIdx, tree_size);
	}


    void clearUnused() {}

    struct ReindexFn {
    	template <Int Idx, typename Tree>
    	void operator()(Tree* tree)
    	{
    		tree->reindex();
    	}
    };

    void reindex()
    {
    	Dispatcher::dispatchAll(&allocator_, ReindexFn());
    }


    template <typename TreeType>
    struct TransferToFn {
    	template <Int Idx, typename Tree>
    	void operator()(const Tree* tree, TreeType* other)
    	{
    		tree->transferDataTo(other->template get<Tree>(Idx));
    	}
    };

    template <typename TreeType>
    void transferDataTo(TreeType* other) const
    {
    	Dispatcher::dispatchAll(&allocator_, TransferToFn<TreeType>(), other);

    	const auto* my_values 	= values();
    	auto* other_values 		= other->values();

    	Int size = this->size();

    	for (Int c = 0; c < size; c++)
    	{
    		other_values[c] = my_values[c];
    	}
    }

    struct ClearFn {
    	template <Int Idx, typename Tree>
    	void operator()(Tree* tree, Int start, Int end)
    	{
    		for (Int c = start; c < end; c++)
    		{
    			tree->clearValues(c);
    		}
    	}
    };

    void clear(Int start, Int end)
    {
    	Dispatcher::dispatchAll(&allocator_, ClearFn(), start, end);

    	Value* values	= this->values();

    	for (Int c = start; c < end; c++)
    	{
    		values[c] = 0;
    	}
    }


    Int data_size() const
    {
        return sizeof(Me) + this->getDataSize();
    }

    Int size() const {
    	return tree0()->size();
    }

    bool isEmpty() const
    {
    	return size() == 0;
    }

    bool isAfterEnd(const Position& idx) const
    {
    	return idx.get() >= size();
    }

    struct SetChildrenCountFn {
    	template <Int Idx, typename Tree>
    	void operator()(Tree* tree, Int size)
    	{
    		tree->size() = size;
    	}
    };

    void set_children_count(Int map_size)
    {
        Base::map_size() = map_size;
        Dispatcher::dispatchAll(&allocator_, SetChildrenCountFn(), map_size);
    }

    struct IncSizeFn {
    	template <Int Idx, typename Tree>
    	void operator()(Tree* tree, Int size)
    	{
    		tree->size() += size;
    	}
    };

    void inc_size(Int count)
    {
        Base::map_size() += count;
        Dispatcher::dispatchAll(&allocator_, IncSizeFn(), count);
    }

    struct InsertSpaceFn {
    	template <Int Idx, typename Tree>
    	void operator()(Tree* tree, Int room_start, Int room_length)
    	{
    		tree->insertSpace(room_start, room_length);
    		for (Int c = room_start; c < room_start + room_length; c++)
    		{
    			tree->clearValues(c);
    		}
    	}
    };

    void insertSpace(const Position& from_pos, const Position& length_pos)
    {
    	Int room_start 	= from_pos.get();
    	Int room_length = length_pos.get();

    	MEMORIA_ASSERT(room_start, <=, this->size());

    	Int size = this->size();

    	Dispatcher::dispatchAll(&allocator_, InsertSpaceFn(), room_start, room_length);

    	Value* values = this->values();
    	CopyBuffer(values + room_start, values + room_start + room_length, size - room_start);

    	for (Int c = room_start; c < room_start + room_length; c++)
    	{
    		values[c] = 0;
    	}

    	this->set_children_count(this->size());
    }

    struct RemoveSpaceFn {
    	template <Int Idx, typename Tree>
    	void operator()(Tree* tree, Int room_start, Int room_length)
    	{
    		tree->removeSpace(room_start, room_length);
    	}
    };


    Accumulator removeSpace(const Position& from_pos, const Position& length_pos, bool reindex = true)
    {
    	Int room_start = from_pos.get();
    	Int room_length = length_pos.get();

    	Accumulator accum = sum(room_start, room_start + room_length);

    	Int old_size = this->size();

    	Dispatcher::dispatchAll(&allocator_, RemoveSpaceFn(), room_start, room_length);

    	Value* values = this->values();

    	CopyBuffer(values + room_start + room_length, values + room_start, old_size - room_start - room_length);

    	clear(old_size - room_length, old_size);

    	this->set_children_count(this->size());

    	if (reindex)
    	{
    		this->reindex();
    	}

    	return accum;
    }

    bool shouldMergeWithSiblings() const
    {
    	return capacity() >= size();
    }

    bool canMergeWith(const MyType* target) const
    {
    	Int size 		= this->size();
    	Int capacity 	= target->capacity();

    	return size <= capacity;
    }


    struct CopyToFn {
    	template <Int Idx, typename Tree>
    	void operator()(const Tree* tree, MyType* other, Int copy_from, Int count, Int copy_to)
    	{
    		tree->copyTo(other->template get<Tree>(Idx), copy_from, count, copy_to);
    	}
    };


    void copyTo(MyType* other, Int copy_from, Int count, Int copy_to) const
    {
    	MEMORIA_ASSERT(copy_from + count, <=, size());
    	MEMORIA_ASSERT(copy_to + count, <=, other->max_size());

    	Dispatcher::dispatchAll(&allocator_, CopyToFn(), other, copy_from, count, copy_to);

    	CopyBuffer(this->values() + copy_from, other->values() + copy_to, count);
    }

    void mergeWith(MyType* target)
    {
    	Int size = this->size();
    	copyTo(target, 0, size, target->size());
    	target->inc_size(size);
    	target->reindex();
    }

    Int max_size() const
    {
    	return tree()->max_size();
    }

    Position nodeSizes() const
    {
    	return Position(size());
    }

    void reindexAll(Int from, Int to)
    {
    	tree()->reindex();
    }

    Key& key(Int block_num, Int key_num)
    {
    	MEMORIA_ASSERT(key_num, >=, 0);
    	MEMORIA_ASSERT(key_num, <, tree()->max_size());

    	Tree* tree = this->tree();
    	return tree->value(block_num * tree->max_size() + key_num);
    }

    const Key& key(Int block_num, Int key_num) const
    {
    	MEMORIA_ASSERT(key_num, >=, 0);
    	MEMORIA_ASSERT(key_num, <, tree()->max_size());

    	const Tree* tree = this->tree();
    	return tree->value(block_num * tree->max_size() + key_num);
    }

    const Key& key(Int key_num) const
    {
    	return key(0, key_num);
    }

    Accumulator keys(Int idx) const {
    	return keysAcc(idx);
    }

	Accumulator keysAcc(Int idx) const
	{
		Accumulator accum;

		std::get<0>(accum)[0] = tree()->value(idx);

		return accum;
	}

	struct KeysAtFn {
		template <Int Idx, typename Tree>
		void operator()(const Tree* tree, Int idx, Accumulator* acc)
		{
			std::get<Idx>(*acc)[0] = tree->value(idx);
		}
	};

    Accumulator keysAt(Int idx) const
    {
    	Accumulator acc;

    	Dispatcher::dispatchAll(&allocator_, KeysAtFn(), idx, &acc);

    	return acc;
    }

    struct MaxKeysFn {
    	template <Int Idx, typename Tree>
    	void operator()(const Tree* tree, Accumulator* acc)
    	{
    		std::get<Idx>(*acc)[0] = tree->sum();
    	}
    };

    Accumulator maxKeys() const
    {
    	Accumulator acc;

    	Dispatcher::dispatchAll(&allocator_, MaxKeysFn(), &acc);

    	return acc;
    }

    struct SetKeysFn {
    	template <Int Idx, typename Tree>
    	void operator()(Tree* tree, Int idx, Accumulator* keys)
    	{
    		for (Int c = 0; c < INDEXES; c++)
    		{
    			Key k = std::get<Idx>(*keys)[c];
    			tree->value(c * tree->max_size() + idx) = k;
    		}
    	}
    };

    void setKeys(Int idx, Accumulator& keys)
    {
    	Dispatcher::dispatchAll(&allocator_, SetKeysFn(), idx, &keys);
    }


    struct GetKeysFn {
    	template <Int Idx, typename Tree>
    	void operator()(Tree* tree, Int idx, Accumulator* keys)
    	{
    		for (Int c = 0; c < INDEXES; c++)
    		{
    			Key k = tree->value(c * tree->max_size() + idx);
    			std::get<Idx>(*keys)[c] = k;
    		}
    	}
    };

    Accumulator getKeys(Int idx) const
    {
    	Accumulator keys;

    	Dispatcher::dispatchAll(&allocator_, GetKeysFn(), idx, &keys);

    	return keys;
    }


    Value& value(Int idx)
    {
    	return *(values() + idx);
    }

    const Value& value(Int idx) const
    {
    	return *(values() + idx);
    }

    struct SumFn {
    	template <Int Idx, typename Tree>
    	void operator()(const Tree* tree, Int start, Int end, Accumulator* accum)
    	{
    		std::get<Idx>(*accum)[0] += tree->sum(start, end);
    	}

    	template <Int Idx, typename Tree>
    	void operator()(const Tree* tree, Int block_num, Int start, Int end, Key* accum)
    	{
    		*accum += tree->sum(start, end);
    	}
    };

    void sum(Int start, Int end, Accumulator& accum) const
    {
    	Dispatcher::dispatchAll(&allocator_, SumFn(), start, end, &accum);
    }

    Accumulator sum(Int start, Int end) const
    {
    	Accumulator accum;
    	Dispatcher::dispatchAll(&allocator_, SumFn(), start, end, &accum);
    	return accum;
    }

    void sum(Int block_num, Int start, Int end, Key& accum) const
    {
    	Dispatcher::dispatchAll(&allocator_, SumFn(), block_num, start, end, &accum);
    }

    struct FindLESFn {
    	typedef Int ResultType;

    	template <Int Idx, typename Tree>
    	Int operator()(const Tree* tree, const Key& k, Accumulator* accum)
    	{
    		auto result = tree->findLE(k);

        	std::get<Idx>(*accum)[0] += result.prefix();

        	return result.idx();
    	}
    };

    Int findLES(Int block_num, const Key& k, Accumulator& sum) const
    {
//    	const Tree* tree = this->tree();
//
//    	auto result = tree->findLE(k);
//
//    	std::get<0>(sum)[0] += result.prefix();
//
//    	return result.idx();

    	return Dispatcher::dispatchRtn(0, &allocator_, FindLESFn(), k, &sum);
    }

    Accumulator moveElements(MyType* tgt, const Position& from_pos, const Position& shift_pos)
    {
    	Int from 	= from_pos.get();
    	Int shift 	= shift_pos.get();

    	Accumulator result;

    	Int count = this->size() - from;

    	sum(from, from + count, result);

    	if (tgt->size() > 0)
    	{
    		tgt->insertSpace(Position(0), Position(count + shift));
    	}

    	copyTo(tgt, from, count, shift);
    	clear(from, from + count);

    	inc_size(-count);
    	tgt->inc_size(count + shift);

    	tgt->clear(0, shift);

    	reindex();
    	tgt->reindex();

    	return result;
    }

    void updateUp(Int block_num, Int idx, Key key_value)
    {
    	tree()->updateUp(block_num, idx, key_value);
    }

    Accumulator getCounters(const Position& pos, const Position& count) const
    {
    	return sum(pos.get(), count.get());
    }

    bool checkCapacities(const Position& pos) const
    {
    	return capacity() >= pos.get();
    }

    struct GenerateDataEventsFn {
    	template <Int Idx, typename Tree>
    	void operator()(const Tree* tree, IPageDataEventHandler* handler)
    	{
    		tree->generateDataEvents(handler);
    	}
    };

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        Dispatcher::dispatchAll(&allocator_, GenerateDataEventsFn(), handler);

        handler->startGroup("TREE_VALUES", size());

        for (Int idx = 0; idx < size(); idx++)
        {
        	internl1::ValueHelper<Value>::setup(handler, value(idx));
        }

        handler->endGroup();
    }

    struct SerializeFn {
    	template <Int Idx, typename Tree>
    	void operator()(const Tree* tree, SerializationData* buf)
    	{
    		tree->serialize(*buf);
    	}
    };

    template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
        Base::template serialize<FieldFactory>(buf);

        allocator_.serialize(buf);

        Dispatcher::dispatchAll(&allocator_, SerializeFn(), &buf);

        FieldFactory<Value>::serialize(buf, *values(), size());
    }

    struct DeserializeFn {
    	template <Int Idx, typename Tree>
    	void operator()(Tree* tree, DeserializationData* buf)
    	{
    		tree->deserialize(*buf);
    	}
    };

    template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
        Base::template deserialize<FieldFactory>(buf);

        allocator_.deserialize(buf);

        Dispatcher::dispatchAll(&allocator_, DeserializeFn(), &buf);

        FieldFactory<Value>::deserialize(buf, *values(), size());
    }
};





template <
	template <typename, bool, bool> class TreeNode,
	typename Types,
	bool root, bool leaf
>
class NodePageAdaptor: public TreeNode<Types, root, leaf>
{
public:

    typedef NodePageAdaptor<TreeNode, Types, root, leaf>                  		Me;
    typedef TreeNode<Types, root, leaf>                                			Base;

    typedef NodePageAdaptor<TreeNode, Types, true, leaf>						RootNodeType;
    typedef NodePageAdaptor<TreeNode, Types, false, leaf>						NonRootNodeType;

    typedef NodePageAdaptor<TreeNode, Types, root, true>						LeafNodeType;
    typedef NodePageAdaptor<TreeNode, Types, root, false>						NonLeafNodeType;


    static const UInt PAGE_HASH = TypeHash<Base>::Value;

    static const bool Leaf = leaf;
    static const bool Root = root;

    template <
    	template <typename, bool, bool> class,
    	typename,
    	bool, bool
    >
    friend class NodePageAdaptor;

private:
    static PageMetadata *page_metadata_;

public:


    NodePageAdaptor(): Base() {}

    static Int hash() {
        return PAGE_HASH;
    }

    static PageMetadata *page_metadata() {
        return page_metadata_;
    }

    class PageOperations: public IPageOperations
    {
        virtual Int serialize(const void* page, void* buf) const
        {
            const Me* me = T2T<const Me*>(page);

            SerializationData data;
            data.buf = T2T<char*>(buf);

            me->template serialize<FieldFactory>(data);

            return data.total;
        }

        virtual void deserialize(const void* buf, Int buf_size, void* page) const
        {
            Me* me = T2T<Me*>(page);

            DeserializationData data;
            data.buf = T2T<const char*>(buf);

            me->template deserialize<FieldFactory>(data);
        }

        virtual Int getPageSize(const void *page) const
        {
            const Me* me = T2T<const Me*>(page);
            return me->page_size();
        }

        virtual void resize(const void* page, void* buffer, Int new_size) const
        {
            const Me* me = T2T<const Me*>(page);
            Me* tgt = T2T<Me*>(buffer);

            tgt->copyFrom(me);
            tgt->init(new_size);

            me->transferDataTo(tgt);
            tgt->set_children_count(me->children_count());

            tgt->clearUnused();
            tgt->reindex();
        }

        virtual void generateDataEvents(
                        const void* page,
                        const DataEventsParams& params,
                        IPageDataEventHandler* handler
                     ) const
        {
            const Me* me = T2T<const Me*>(page);
            handler->startPage("BTREE_NODE");
            me->generateDataEvents(handler);
            handler->endPage();
        }

        virtual void generateLayoutEvents(
                        const void* page,
                        const LayoutEventsParams& params,
                        IPageLayoutEventHandler* handler
                     ) const
        {
            const Me* me = T2T<const Me*>(page);
            handler->startPage("BTREE_NODE");
            me->generateLayoutEvents(handler);
            handler->endPage();
        }
    };

    static Int initMetadata()
    {
        if (page_metadata_ == NULL)
        {
            Int attrs = 0;
            page_metadata_ = new PageMetadata("BTREE_PAGE", attrs, hash(), new PageOperations());
        }
        else {}

        return page_metadata_->hash();
    }
};


template <
	template <typename, bool, bool> class TreeNode,
	typename Types,
	bool root, bool leaf
>
PageMetadata* NodePageAdaptor<TreeNode, Types, root, leaf>::page_metadata_ = NULL;


template <
	template <typename, bool, bool> class AdaptedTreeNode,
	typename Types,
	bool root, bool leaf
>
using TreeNode = NodePageAdaptor<AdaptedTreeNode, Types, root, leaf>;


}

template <typename Base>
struct TypeHash<balanced_tree::TreeNodeBase<Base>> {
    static const UInt Value = HashHelper<
    		TypeHash<Base>::Value,
    		balanced_tree::TreeNodeBase<Base>::VERSION,
    		TypeHash<Int>::Value,
    		TypeHash<Int>::Value,
    		TypeHash<Int>::Value,
    		TypeHash<Int>::Value,
    		TypeHash<Int>::Value
    >::Value;
};


template <typename Metadata, typename Base>
struct TypeHash<balanced_tree::RootPage<Metadata, Base, true> > {
    static const UInt Value = HashHelper<TypeHash<Base>::Value, TypeHash<Metadata>::Value>::Value;
};


template <typename Metadata, typename Base>
struct TypeHash<balanced_tree::RootPage<Metadata, Base, false> > {
    static const UInt Value = TypeHash<Base>::Value;
};


template <typename Types, bool root, bool leaf>
struct TypeHash<balanced_tree::TreeMapNode<Types, root, leaf> > {

	typedef balanced_tree::TreeMapNode<Types, root, leaf> Node;

    static const UInt Value = HashHelper<
    		TypeHash<typename Node::Base>::Value,
    		Node::VERSION,
    		root,
    		leaf,
    		Types::Indexes,
    		TypeHash<typename Types::Name>::Value
    >::Value;
};


}

#endif
