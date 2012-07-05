
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_MODEL_FIND_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_MODEL_FIND_HPP

#include <memoria/prototypes/btree/pages/tools.hpp>
#include <memoria/prototypes/btree/names.hpp>



namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::btree::FindName)

    typedef TypesType                                                			Types;
    typedef typename Types::Allocator                                           Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                            					Iterator;
    typedef typename Base::NodeDispatcher                               		NodeDispatcher;
    typedef typename Base::Key                                                  Key;
    typedef typename Base::TreePath                                             TreePath;


    struct SearchModeDefault {
    	typedef enum {NONE, FIRST, LAST} Enum;
    };

private:


public:

    template <typename Comparator>
    struct findFn {

        Iterator        i_;
        Key&            key_;
        MyType&         model_;
        Comparator&     cmp_;
        Int             idx_;
        bool			end_;
        Int				level_;
        bool			found_;

    public:
        findFn(Comparator& cmp, Key& key, NodeBaseG& root, MyType &model):
            i_(model),
            key_(key),
            model_(model),
            cmp_(cmp),
            end_(false),
        	level_(root->level()),
        	found_(true)
        {
        	i_.path().Resize(level_ + 1);

        	i_.setNode(root, 0);
        }

        template <typename Node>
        void operator()(Node *node)
        {
        	idx_ = cmp_.find(node, key_);

        	if (!node->is_leaf())
        	{
        		if (idx_ >= 0)
        		{
        			auto& path_item 		= i_.path()[node->level() - 1];

        			path_item.parent_idx()	= idx_;
        			path_item.node() 		= model_.getChild(node, idx_, Allocator::READ);

        			level_--;
        		}
        		else
        		{
        			idx_ 	= 0;
        			found_ 	= false;
        			end_ 	= true;
        		}
        	}
        	else
        	{
        		if (idx_ < 0)
        		{
        			found_ 	= false;
        		}

        		end_ = true;
        	}
        }

        NodeBaseG& node()
        {
            return i_.path()[level_];
        }
    };


    template <typename Comparator>
    class checkBoundsFn {
    	Key&            key_;
    	Comparator&     cmp_;

    	bool 			within_ranges_;

    public:
    	checkBoundsFn(Comparator& cmp, Key& key):
    		key_(key),
    		cmp_(cmp)
    	{}

    	template <typename Node>
    	void operator()(Node *node)
    	{
    		within_ranges_ = cmp_.IsKeyWithinRange(node, key_);
    	}

    	bool within_ranges() const {
    		return within_ranges_;
    	}
    };

    template <typename Comparator>
    const Iterator _find(Key key, Int c);

    Iterator findStart(bool reverse = false);
    Iterator findEnd  (bool reverse = false);

    BigInt getSize() const
    {
    	return me()->getTotalKeyCount();
    }

    Iterator Begin()
    {
    	return me()->findStart(false);
    }

    Iterator begin()
    {
    	return me()->findStart(false);
    }

    Iterator RBegin()
    {
    	return me()->findEnd(true);
    }

    Iterator End()
    {
    	return me()->findEnd(false);
    }

    Iterator end()
    {
    	Iterator iter(*me());
    	iter.type() = Iterator::END;
    	return iter;
    }

    IterEndMark endm()
    {
    	return IterEndMark();
    }

    Iterator REnd() {
    	return me()->findStart(true);
    }

MEMORIA_CONTAINER_PART_END



#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::FindName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
template <typename Comparator>
const typename M_TYPE::Iterator M_TYPE::_find(Key key, Int block_num)
{
	NodeBaseG node = me()->getRoot(Allocator::READ);

	if (node->children_count() > 0)
	{
		Comparator cmp(block_num);

		checkBoundsFn<Comparator> bounds_fn(cmp, key);
		NodeDispatcher::DispatchConst(node, bounds_fn);

		if (bounds_fn.within_ranges())
		{
			findFn<Comparator> fn(cmp, key, node, *me());

			while(1)
			{
				NodeDispatcher::Dispatch(node, fn);

				if (fn.end_)
				{
					if (fn.found_)
					{
						fn.i_.key_idx() = fn.idx_;

						me()->FinishPathStep(fn.i_.path(), fn.i_.key_idx());

						cmp.setupIterator(fn.i_);

						return fn.i_;
					}
					else
					{
						throw Exception(MEMORIA_SOURCE, SBuf()<<"Can't find key: "<<key);
					}
				}
				else
				{
					node = fn.node();
					cmp.AdjustKey(key);
				}
			}
		}
		else {
			return me()->End();
		}
	}
	else {
		return me()->End();
	}
}



M_PARAMS
typename M_TYPE::Iterator M_TYPE::findStart(bool reverse)
{
	NodeBaseG node = me()->getRoot(Allocator::READ);
	if (node.is_set())
	{
		Iterator i(*me(), node->level() + 1);

		i.setNode(node, 0);

		while(!node->is_leaf())
		{
			node = me()->getChild(node, 0, Allocator::READ);

			i.setNode(node, 0);
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
typename M_TYPE::Iterator M_TYPE::findEnd(bool reverse)
{
	NodeBaseG node = me()->getRoot(Allocator::READ);
	if (node.is_set())
	{
		Iterator i(*me(), node->level() + 1);

		i.setNode(node, 0);

		while(!node->is_leaf())
		{
			Int parent_idx = node->children_count() - 1;

			node = me()->getLastChild(node, Allocator::READ);

			i.setNode(node, parent_idx);
		}

		i.key_idx() = i.page()->children_count() + (reverse ? -1 : 0);

		i.Init();

		return i;
	}
	else {
		return Iterator(*me());
	}
}




#undef M_TYPE
#undef M_PARAMS

}

#endif	//_MEMORIA_MODELS_KVMAP_MODEL_FIND_HPP
