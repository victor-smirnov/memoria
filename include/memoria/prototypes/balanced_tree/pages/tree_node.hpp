
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_NODE_FACTORY2_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_NODE_FACTORY2_HPP

#include <memoria/core/types/typehash.hpp>
#include <memoria/core/tools/reflection.hpp>

#include <memoria/prototypes/balanced_tree/pages/tree_map.hpp>
#include <memoria/prototypes/balanced_tree/baltree_names.hpp>

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
    typedef TreePage<Base>                      Me;
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


    template <typename PageType>
    void copyFrom(const PageType* page)
    {
        Base::copyFrom(page);

        this->set_root(page->is_root());
        this->set_leaf(page->is_leaf());
        this->setBitmap(page->isBitmap());

        this->level() = page->level();
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
public:

    static const UInt VERSION                                                   = 1;

    typedef TreeMapNode<Types, root, leaf>                                      Me;
    typedef RootPage<
    			typename Types::Metadata,
    			typename Types::NodePageBase, root
    >  																			Base;

private:

public:

    typedef TreeMap<
                TreeMapTypes<
                    typename Types::Key,
                    typename Types::Value,
                    Types::Indexes,
                    Accumulators<typename Types::Key, Types::Indexes>
                >
    >                                                                           Map;

private:

    Map map_;

public:



    static const long INDEXES                                                   = Types::Indexes;

    typedef typename Types::Key                                                 Key;
    typedef typename Types::Value                                               Value;

    TreeMapNode(): Base(), map_() {}


    const Map &map() const
    {
        return map_;
    }

    Map &map()
    {
        return map_;
    }

    void reindex()
    {
        map().reindex();
    }

    Int data_size() const
    {
        return sizeof(Me) + map_.getDataSize();
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
            tgt->map().init(new_size - sizeof(Me) + sizeof(typename Me::Map) );

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





}

template <typename Base>
struct TypeHash<balanced_tree::TreePage<Base>> {
    static const UInt Value = HashHelper<
    		TypeHash<Base>::Value,
    		balanced_tree::TreePage<Base>::VERSION,
    		TypeHash<Int>::Value,
    		TypeHash<Int>::Value,
    		TypeHash<Int>::Value,
    		TypeHash<Int>::Value,
    		TypeHash<Int>::Value
    >::Value;
};


template <typename Metadata, typename Base>
struct TypeHash<RootPage<Metadata, Base, true> > {
    static const UInt Value = HashHelper<TypeHash<Base>::Value, TypeHash<Metadata>::Value>::Value;
};


template <typename Metadata, typename Base>
struct TypeHash<RootPage<Metadata, Base, false> > {
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
