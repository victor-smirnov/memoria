
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_NODE_FACTORY2_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_NODE_FACTORY2_HPP

#include <memoria/core/types/typehash.hpp>
#include <memoria/core/types/algo/select.hpp>
#include <memoria/core/tools/reflection.hpp>


#include <memoria/prototypes/balanced_tree/nodes/tree_map.hpp>
#include <memoria/prototypes/balanced_tree/baltree_types.hpp>
#include <memoria/prototypes/balanced_tree/baltree_tools.hpp>

namespace memoria    	{
namespace balanced_tree {


using memoria::BitBuffer;



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



    typedef TreeMap<
                TreeMapTypes<
                    typename Types::Key,
                    Value,
                    Types::Indexes,
                    Accumulator
                >
    >                                                                           Map;

private:

    Map map_;

public:



    static const long INDEXES                                                   = Types::Indexes;

    TreeMapNode(): Base(), map_() {}


    const Map& map() const
    {
        return map_;
    }

    Map& map()
    {
        return map_;
    }

    void init(Int block_size)
    {
    	map_.init(block_size - sizeof(Me) + sizeof(Map));
    }

    void reindex()
    {
        map().reindex();
    }

    Int data_size() const
    {
        return sizeof(Me) + map_.getDataSize();
    }

    Int size() const {
    	return map_.size();
    }

    bool isEmpty() const
    {
    	return size() == 0;
    }

    bool isAfterEnd(const Position& idx) const
    {
    	return idx.get() >= size();
    }

    void set_children_count(Int map_size)
    {
        Base::map_size() = map_size;
        map_.size()      = map_size;
    }

    void inc_size(Int count)
    {
        Base::map_size() += count;
        map_.size()      += count;
    }

    void insertSpace(const Position& from_pos, const Position& length_pos)
    {
    	Int from 	= from_pos.get();
    	Int length 	= length_pos.get();

    	map_.insertSpace(from, length);

    	for (Int c = from; c < from + length; c++)
    	{
    		for (Int d = 0; d < INDEXES; d++)
    		{
    			map_.key(d, c) = 0;
    		}

    		map_.data(c) = 0;
    	}

    	this->set_children_count(map_.size());
    }


    Accumulator removeSpace(const Position& from_pos, const Position& length_pos, bool reindex = true)
    {
    	Int from = from_pos.get();
    	Int count = length_pos.get();

    	Accumulator accum = map_.sum(from, from + count);

    	Int old_size = map_.size();

    	map_.removeSpace(from, count);

    	map_.clear(old_size - count, old_size);

    	this->set_children_count(map_.size());

    	if (reindex)
    	{
    		map_.reindex();
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

//    	map_.dump();
//    	target->map_.dump();

//    	return size() <= target->capacity();

    	return size <= capacity;
    }

    void mergeWith(MyType* target)
    {
    	Int size = this->size();
    	map().copyTo(&target->map(), 0, size, target->size());
    	target->inc_size(size);
    	target->map().reindex();
    }

    Int capacity() const
    {
    	return map_.capacity();
    }

    Int max_size() const
    {
    	return map_.max_size();
    }

    Position nodeSizes() const {
    	return Position(size());
    }

    void reindexAll(Int from, Int to)
    {
    	map_.reindexAll(from, to);
    }

    Key key(Int idx) const
    {
    	return map_.key(0, idx);
    }

    Accumulator keys(Int idx) const {
    	return map_.keysAcc(idx);
    }

    Accumulator maxKeys() const {
    	return map_.maxKeys();
    }

    Value& value(Int idx) {
    	return map_.data(idx);
    }

    const Value& value(Int idx) const {
    	return map_.data(idx);
    }

    Accumulator moveElements(MyType* tgt, const Position& from_pos, const Position& shift_pos)
    {
    	Int from 	= from_pos.get();
    	Int shift 	= shift_pos.get();

    	Accumulator result;

    	Int count = map_.size() - from;

    	map_.sum(from, from + count, result);

    	if (tgt->size() > 0)
    	{
    		tgt->insertSpace(Position(0), Position(count + shift));
    	}

    	map_.copyTo(&tgt->map(), from, count, shift);
    	map_.clear(from, from + count);

    	inc_size(-count);
    	tgt->inc_size(count + shift);

    	tgt->map().clear(0, shift);

    	map_.reindex();
    	tgt->reindex();

    	return result;
    }

    Accumulator getCounters(const Position& pos, const Position& count) const
    {
    	return map_.sum(pos.get(), count.get());
    }

    bool checkCapacities(const Position& pos) const
    {
    	return capacity() >= pos.get();
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);
        map_.generateDataEvents(handler);
    }

    template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
        Base::template serialize<FieldFactory>(buf);

        FieldFactory<Map>::serialize(buf, map_);
    }

    template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
        Base::template deserialize<FieldFactory>(buf);

        FieldFactory<Map>::deserialize(buf, map_);
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

            me->map().transferDataTo(&tgt->map());
            tgt->set_children_count(me->children_count());

            tgt->map().clearUnused();
            tgt->map().reindex();
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
    		TypeHash<typename Types::Name>::Value,
    		TypeHash<typename Node::Map>::Value
    >::Value;
};


}

#endif
