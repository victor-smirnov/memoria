
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_MODEL_FIND_HPP
#define _MEMORIA_PROTOTYPES_BTREE_MODEL_FIND_HPP

#include <memoria/prototypes/btree/pages/tools.hpp>
#include <memoria/prototypes/btree/names.hpp>



namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::btree::FindName)

    typedef TypesType                                                           Types;
    typedef typename Types::Allocator                                           Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::Key                                                  Key;
    typedef typename Base::TreePath                                             TreePath;


    struct SearchModeDefault {
        typedef enum {NONE, FIRST, LAST} Enum;
    };

private:


public:

    template <typename Walker>
    Iterator find0(Walker& walker);

    MEMORIA_PUBLIC MEMORIA_DEPRECATED BigInt getSize() const
    {
        return me()->getTotalKeyCount();
    }

    MEMORIA_PUBLIC BigInt size() const
    {
    	return me()->getTotalKeyCount();
    }

    MEMORIA_PUBLIC Iterator Begin()
    {
    	typename Types::template FindBeginWalker<Types> walker(*me());
    	return me()->find0(walker);
    }

    MEMORIA_PUBLIC Iterator begin()
    {
    	typename Types::template FindBeginWalker<Types> walker(*me());
    	return me()->find0(walker);
    }

    MEMORIA_PUBLIC Iterator RBegin()
    {
    	typename Types::template FindRBeginWalker<Types> walker(*me());
    	return me()->find0(walker);
    }

    MEMORIA_PUBLIC Iterator End()
    {
    	typename Types::template FindEndWalker<Types> walker(*me());
    	return me()->find0(walker);
    }


    MEMORIA_PUBLIC Iterator end()
    {
        Iterator iter(*me());
        iter.type() = Iterator::END;
        return iter;
    }

    MEMORIA_PUBLIC IterEndMark endm()
    {
        return IterEndMark();
    }

    MEMORIA_PUBLIC Iterator REnd()
    {
    	typename Types::template FindREndWalker<Types> walker(*me());
    	return me()->find0(walker);
    }

MEMORIA_CONTAINER_PART_END



#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::btree::FindName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

//M_PARAMS
//template <typename Comparator>
//const typename M_TYPE::Iterator M_TYPE::_find(Key key, Int block_num)
//{
//    MEMORIA_ASSERT(block_num, >=, 0)
//
//    NodeBaseG node = me()->getRoot(Allocator::READ);
//
//    if (node->children_count() > 0)
//    {
//        Comparator cmp(block_num);
//
//        CheckBoundsFn<Comparator> bounds_fn(cmp, key);
//        NodeDispatcher::DispatchConst(node, bounds_fn);
//
//        if (bounds_fn.within_ranges())
//        {
//            FindFn<Comparator> fn(cmp, key, node, *me());
//
//            while(1)
//            {
//                NodeDispatcher::Dispatch(node, fn);
//
//                if (fn.end_)
//                {
//                    if (fn.found_)
//                    {
//                        fn.i_.key_idx() = fn.idx_;
//
//                        me()->finishPathStep(fn.i_.path(), fn.i_.key_idx());
//
//                        cmp.setupIterator(fn.i_);
//
//                        return fn.i_;
//                    }
//                    else
//                    {
//                        throw Exception(MEMORIA_SOURCE, SBuf()<<"Can't find key: "<<key);
//                    }
//                }
//                else
//                {
//                    node = fn.node();
//                    cmp.AdjustKey(key);
//                }
//            }
//        }
//        else {
//            return me()->End();
//        }
//    }
//    else {
//        return me()->End();
//    }
//}

M_PARAMS
template <typename Walker>
typename M_TYPE::Iterator M_TYPE::find0(Walker& walker)
{
	NodeBaseG node = me()->getRoot(Allocator::READ);
	if (node.isSet())
	{
		Iterator i(*me(), node->level() + 1);

		i.setNode(node, 0);

		if (node->children_count() > 0)
		{
			while (!node->is_leaf())
			{
				NodeDispatcher::DispatchConst(node, walker);
				Int idx = walker.idx();

				idx = walker.checkIdxBounds(idx, node.page());

				node = me()->getChild(node, idx, Allocator::READ);
				i.setNode(node, idx);
			}


			NodeDispatcher::Dispatch(node, walker);

			Int idx = walker.idx();
			walker.finish(idx, i);
		}
		else {
			walker.empty(i);
		}

		return i;
	}
	else {
		return Iterator(*me());
	}
}



//M_PARAMS
//typename M_TYPE::Iterator M_TYPE::findStart(bool reverse)
//{
//    NodeBaseG node = me()->getRoot(Allocator::READ);
//    if (node.isSet())
//    {
//        Iterator i(*me(), node->level() + 1);
//
//        i.setNode(node, 0);
//
//        while(!node->is_leaf())
//        {
//            node = me()->getChild(node, 0, Allocator::READ);
//
//            i.setNode(node, 0);
//        }
//
//        i.key_idx() = reverse ? -1 : 0;
//
//        i.init();
//
//        return i;
//    }
//    else {
//        return Iterator(*me());
//    }
//}


//M_PARAMS
//typename M_TYPE::Iterator M_TYPE::findEnd(bool reverse)
//{
//    NodeBaseG node = me()->getRoot(Allocator::READ);
//    if (node.isSet())
//    {
//        Iterator i(*me(), node->level() + 1);
//
//        i.setNode(node, 0);
//
//        while(!node->is_leaf())
//        {
//            Int parent_idx = node->children_count() - 1;
//
//            node = me()->getLastChild(node, Allocator::READ);
//
//            i.setNode(node, parent_idx);
//        }
//
//        i.key_idx() = i.page()->children_count() + (reverse ? -1 : 0);
//
//        i.init();
//
//        return i;
//    }
//    else {
//        return Iterator(*me());
//    }
//}




#undef M_TYPE
#undef M_PARAMS

}

#endif  //_MEMORIA_MODELS_KVMAP_MODEL_FIND_HPP
