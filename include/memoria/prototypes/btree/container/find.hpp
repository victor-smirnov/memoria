
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_MODEL_FIND_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_MODEL_FIND_HPP

#include <memoria/vapi/models/logs.hpp>

#include <memoria/prototypes/btree/pages/tools.hpp>
#include <memoria/prototypes/btree/names.hpp>



namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_NO_CTR_BEGIN(memoria::btree::FindName)

    typedef TypesType                                                			Types;
    typedef typename Types::Allocator                                           Allocator;

    typedef typename Allocator::Page                                            Page;
    typedef typename Page::ID                                                   ID;
    typedef typename Allocator::Transaction                                     Transaction;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Types::Counters                                            Counters;
    typedef Iter<typename Types::IterTypes>                            			Iterator;

    typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
    typedef typename Types::Pages::RootDispatcher                               RootDispatcher;
    typedef typename Types::Pages::LeafDispatcher                               LeafDispatcher;
    typedef typename Types::Pages::NonLeafDispatcher                            NonLeafDispatcher;

    typedef typename Types::Pages::Node2RootMap                                 Node2RootMap;
    typedef typename Types::Pages::Root2NodeMap                                 Root2NodeMap;

    typedef typename Types::Pages::NodeFactory                                  NodeFactory;

    typedef typename Base::Key                                                  Key;
    typedef typename Types::Value                                               Value;

    typedef typename Base::Metadata												Metadata;

    typedef typename Base::TreePath                                             TreePath;
    typedef typename Base::TreePathItem                                         TreePathItem;

    static const Int Indexes                                                    = Types::Indexes;

    struct SearchModeDefault {
    	typedef enum {NONE, FIRST, LAST} Enum;
    };

private:


public:

    CtrPart():Base(){}
    virtual ~CtrPart() {}

    template <typename Comparator>
    struct FindFn {

        Iterator        i_;
        Key&            key_;
        Int             key_num_;
        MyType&         model_;
        Comparator&     cmp_;
        Int             idx_;
        bool			end_;
        Int				level_;

    public:
        FindFn(Comparator& cmp, Key& key, Int key_num, NodeBaseG& root, MyType &model):
            i_(model),
            key_(key),
            key_num_(key_num),
            model_(model),
            cmp_(cmp),
            end_(false),
        	level_(root->level())
        {
        	i_.path().Resize(level_ + 1);

        	i_.SetNode(root, 0);
        }

        template <typename Node>
        void operator()(Node *node)
        {
        	idx_ = cmp_.Find(node, key_num_, key_);

        	if (!node->is_leaf())
        	{
        		if (idx_ >= 0)
        		{
        			auto& path_item 		= i_.path()[node->level() - 1];

        			path_item.parent_idx()	= idx_;
        			path_item.node() 		= model_.GetChild(node, idx_, Allocator::READ);
        		}
        		else if (cmp_.search_mode_ != SearchModeDefault::NONE)
            	{
        			//FIXME: key_idx() == END for leaf

        			if (cmp_.search_mode() == SearchModeDefault::LAST)
        			{
        				idx_ = node->children_count() - 1;
        			}
        			else {
        				idx_ = 0;
        			}

        			auto& path_item 		= i_.path()[node->level() - 1];

        			path_item.parent_idx()	= idx_;
        			path_item.node() 		= model_.GetChild(node, idx_, Allocator::READ);
            	}
        		else
        		{
        			idx_ = 0;
        			end_ = true;
        		}
        	}
        	else
        	{
        		if (idx_ < 0)
        		{
        			if (cmp_.search_mode() == SearchModeDefault::LAST)
        			{
        				idx_ = node->children_count();
        			}
        			else
        			{
        				idx_ = -1;
        			}
        		}

        		end_ = true;
        	}

        	if (end_)
        	{
        		i_.key_idx() = idx_;
        		cmp_.SetupIterator(i_);
        	}
        	else {
        		level_--;
        	}
        }

        NodeBaseG& node()
        {
            return i_.path()[level_];
        }
    };

    template <typename Comparator>
    const Iterator _find(Key key, Int c, bool for_insert);

    Iterator FindStart(bool reverse = false);
    Iterator FindEnd  (bool reverse = false);

    BigInt GetTotalKeyCount() const;
    void SetTotalKeyCount(BigInt value);
    void AddTotalKeyCount(BigInt value);
    void AddTotalKeyCount(TreePath& path, BigInt value);

    BigInt GetSize() const
    {
    	return me()->GetTotalKeyCount();
    }

    Iterator Begin()
    {
    	return me()->FindStart(false);
    }

    Iterator RBegin()
    {
    	return me()->FindEnd(true);
    }

    Iterator End()
    {
    	return me()->FindEnd(false);
    }

    Iterator REnd() {
    	return me()->FindStart(true);
    }

MEMORIA_CONTAINER_PART_END



#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::FindName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
template <typename Comparator>
const typename M_TYPE::Iterator M_TYPE::_find(Key key, Int c, bool for_insert)
{
	NodeBaseG node = me()->GetRoot(Allocator::READ);

	if (node.is_set())
	{
		Comparator cmp(for_insert ? SearchModeDefault::LAST : SearchModeDefault::NONE);

		FindFn<Comparator> fn(cmp, key, c, node, *me());

		while(1)
		{
			NodeDispatcher::Dispatch(node, fn);

			if (fn.end_)
			{
				me()->FinishPathStep(fn.i_.path(), fn.i_.key_idx());
//				fn.i_.Init();
				return fn.i_;
			}
			else
			{
				node = fn.node();

				cmp.AdjustKey(key);
			}
		}
	}
	else {
		MEMORIA_TRACE(me(), "No Root for Container");
		return Iterator(*me());
	}
}



M_PARAMS
typename M_TYPE::Iterator M_TYPE::FindStart(bool reverse)
{
	NodeBaseG node = me()->GetRoot(Allocator::READ);
	if (node.is_set())
	{
		Iterator i(*me(), node->level() + 1);

		i.SetNode(node, 0);

		while(!node->is_leaf())
		{
			node = me()->GetChild(node, 0, Allocator::READ);

			i.SetNode(node, 0);
		}

		i.key_idx() = reverse ? -1 : 0;

		i.Init();

		return i;
	}
	else {
		return Iterator(*me());
	}
}


M_PARAMS
typename M_TYPE::Iterator M_TYPE::FindEnd(bool reverse)
{
	NodeBaseG node = me()->GetRoot(Allocator::READ);
	if (node.is_set())
	{
		Iterator i(*me(), node->level() + 1);

		i.SetNode(node, 0);

		while(!node->is_leaf())
		{
			Int parent_idx = node->children_count() - 1;

			node = me()->GetLastChild(node, Allocator::READ);

			i.SetNode(node, parent_idx);
		}

		i.key_idx() = i.page()->children_count() + (reverse ? -1 : 0);

		i.Init();

		return i;
	}
	else {
		return Iterator(*me());
	}
}


M_PARAMS
BigInt M_TYPE::GetTotalKeyCount() const
{
	NodeBaseG node = me()->GetRoot(Allocator::READ);

	if (node.is_set())
	{
		Metadata meta = me()->GetRootMetadata(node);
		return meta.key_count();
	}
	else {
		return 0;
	}
}

M_PARAMS
void M_TYPE::SetTotalKeyCount(BigInt value)
{
	NodeBaseG node = me()->GetRoot(Allocator::UPDATE);
	if (node.is_set())
	{
		Metadata meta = me()->GetRootMetadata(node);
		meta.key_count() = value;

		me()->SetRootMetadata(node, meta);
	}
	else {
		throw MemoriaException(MEMORIA_SOURCE, String("Root node is not set for this container: ") + me()->type_name());
	}
}

M_PARAMS
void M_TYPE::AddTotalKeyCount(BigInt value)
{
	NodeBaseG node = me()->GetRoot(Allocator::UPDATE);
	if (node.is_set())
	{
		Metadata meta = me()->GetRootMetadata(node);
		meta.key_count() += value;

		me()->SetRootMetadata(node, meta);
	}
	else {
		throw MemoriaException(MEMORIA_SOURCE, String("Root node is not set for this container: ") + me()->type_name());
	}
}


M_PARAMS
void M_TYPE::AddTotalKeyCount(TreePath& path, BigInt value)
{
	NodeBaseG& node = path[path.GetSize() - 1].node();

	Metadata meta = me()->GetRootMetadata(node);

	meta.key_count() += value;

	me()->SetRootMetadata(node, meta);
}


#undef M_TYPE
#undef M_PARAMS

}

#endif	//_MEMORIA_MODELS_KVMAP_MODEL_FIND_HPP
