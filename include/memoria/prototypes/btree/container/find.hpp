
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
    struct FindFn {

        Iterator        i_;
        Key&            key_;
        MyType&         model_;
        Comparator&     cmp_;
        Int             idx_;
        bool			end_;
        Int				level_;
        bool			found_;

    public:
        FindFn(Comparator& cmp, Key& key, NodeBaseG& root, MyType &model):
            i_(model),
            key_(key),
            model_(model),
            cmp_(cmp),
            end_(false),
        	level_(root->level()),
        	found_(true)
        {
        	i_.path().Resize(level_ + 1);

        	i_.SetNode(root, 0);
        }

        template <typename Node>
        void operator()(Node *node)
        {
        	idx_ = cmp_.Find(node, key_);

        	if (!node->is_leaf())
        	{
        		if (idx_ >= 0)
        		{
        			auto& path_item 		= i_.path()[node->level() - 1];

        			path_item.parent_idx()	= idx_;
        			path_item.node() 		= model_.GetChild(node, idx_, Allocator::READ);

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
    class CheckBoundsFn {
    	Key&            key_;
    	Comparator&     cmp_;

    	bool 			within_ranges_;

    public:
    	CheckBoundsFn(Comparator& cmp, Key& key):
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

    Iterator FindStart(bool reverse = false);
    Iterator FindEnd  (bool reverse = false);

    BigInt GetSize() const
    {
    	return me()->GetTotalKeyCount();
    }

    Iterator Begin()
    {
    	return me()->FindStart(false);
    }

    Iterator begin()
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
    	return me()->FindStart(true);
    }

MEMORIA_CONTAINER_PART_END



#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::FindName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
template <typename Comparator>
const typename M_TYPE::Iterator M_TYPE::_find(Key key, Int block_num)
{
	NodeBaseG node = me()->GetRoot(Allocator::READ);

	if (node->children_count() > 0)
	{
		Comparator cmp(block_num);

		CheckBoundsFn<Comparator> bounds_fn(cmp, key);
		NodeDispatcher::DispatchConst(node, bounds_fn);

		if (bounds_fn.within_ranges())
		{
			FindFn<Comparator> fn(cmp, key, node, *me());

			while(1)
			{
				NodeDispatcher::Dispatch(node, fn);

				if (fn.end_)
				{
					if (fn.found_)
					{
						fn.i_.key_idx() = fn.idx_;

						me()->FinishPathStep(fn.i_.path(), fn.i_.key_idx());

						cmp.SetupIterator(fn.i_);

						return fn.i_;
					}
					else
					{
						throw MemoriaException(MEMORIA_SOURCE, "Can't find key: "+ToString(key));
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




#undef M_TYPE
#undef M_PARAMS

}

#endif	//_MEMORIA_MODELS_KVMAP_MODEL_FIND_HPP
