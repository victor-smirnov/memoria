
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

    static const Int Indexes                                                    = Types::Indexes;

private:


public:

    CtrPart(MyType &me):
    	Base(me),
    	me_(me)
    {}

    virtual ~CtrPart() {

    }

    MyType &me() {
    	return me_;
    }

    template <
            typename Comparator
    >
    struct FindFn {
        Iterator        i_;
        bool            rtn_;
        NodeBaseG       node_;
        Key             key_;
        Int             c_;
        MyType&         model_;
        Comparator&     cmp_;
        
        Int             idx_;

    public:
        FindFn(Comparator& cmp, const Key& key, Int c, MyType &model):
            i_(model), rtn_(false), node_(),
            key_(key), c_(c), model_(model),
            cmp_(cmp) {}

        template <typename Node>
        void operator()(Node *node)
        {
            rtn_ = false;
            idx_ = cmp_.Find(node, c_, key_);

            if (idx_ >= 0)
            {
                if (!node->is_leaf()) {
                    node_ = model_.GetChild(node, idx_);
                }
                else {
                	node_           = node;
                    i_.page()       = node;
                    i_.key_idx()    = idx_;
                    cmp_.SetupIterator(i_);

//                    i_.Init();

                    rtn_            = true;
                }
            }
            else if (cmp_.for_insert_ && cmp_.CompareMax(key_, node->map().max_key(c_)))
            {
            	if (!node->is_leaf())
                {
                    idx_     = node->map().size() - 1;
                    node_    = model_.GetChild(node, idx_);
                }
                else
                {
                    node_           = node;
                    i_.page()       = node;
                    i_.key_idx()    = node->map().size();
                    cmp_.SetupIterator(i_);

//                    i_.Init();
                    rtn_            = true;
                }
            }
            else {
                node_           = NULL;
                i_.page()       = NULL;
                i_.key_idx()    = -1;
                i_.state()      = 0;
                cmp_.SetupIterator(i_);
                
                i_.Init();
                rtn_ = true;
            }
        }

        const Iterator iterator() const {
            return i_;
        }

        bool rtn() const {
            return rtn_;
        }

        NodeBaseG& node() {
            return node_;
        }

    };

    template <typename Comparator>
    const Iterator _find(Key key, Int c, bool for_insert);

    Iterator FindStart();

    const Iterator FindREnd() const;

    const Iterator FindEnd() const;

    Iterator FindRStart();

    BigInt GetTotalKeyCount();

    BigInt GetSize() {
    	return me_.GetTotalKeyCount();
    }

    Iterator Begin() {
    	return me_.FindStart();
    }

    const Iterator RBegin() const {
    	return me_.FindRStart();
    }

    Iterator RBegin() {
    	return me_.FindRStart();
    }

    const Iterator End() const {
    	return me_.FindEnd();
    }

    Iterator End() {
    	return me_.FindEnd();
    }

    Iterator REnd() {
    	return me_.FindREnd();
    }

MEMORIA_CONTAINER_PART_END



#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::FindName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
template <typename Comparator>
const typename M_TYPE::Iterator M_TYPE::_find(Key key, Int c, bool for_insert)
{
	MEMORIA_TRACE(me_, "begin", key, c, for_insert, me_.root());
	NodeBase *node = me_.GetRoot();

	if (node != NULL)
	{
		MEMORIA_TRACE(me_, "Init search, start from", node->id());
		Comparator cmp(for_insert);
		while(1)
		{
			FindFn<Comparator> fn(cmp, key, c, me_);

			NodeDispatcher::Dispatch(node, fn);

			if (fn.rtn_)
			{
				return fn.i_;
			}
			else {
				node = fn.node();
				cmp.AdjustKey(key);
			}
		}
	}
	else {
		MEMORIA_TRACE(me_, "No Root for Container");
		return Iterator(me_);
	}
}



M_PARAMS
typename M_TYPE::Iterator M_TYPE::FindStart()
{
	NodeBase *node = me_.GetRoot();
	if (node != NULL)
	{
		while(!node->is_leaf())
		{
			node = me_.GetChild(node, 0);
		}

		return Iterator(node, 0, me_);
	}
	else {
		return Iterator(me_);
	}
}

M_PARAMS
const typename M_TYPE::Iterator M_TYPE::FindREnd() const
{
	NodeBase *node = me_.GetRoot();
	if (node != NULL)
	{
		while(!node->is_leaf())
		{
			node = me_.GetChild(node, 0);
		}

		return Iterator(node, -1, me_);
	}
	else {
		return Iterator(me_);
	}
}

M_PARAMS
const typename M_TYPE::Iterator M_TYPE::FindEnd() const
{
	NodeBase *node = me_.GetRoot();
	if (node != NULL)
	{
		while(!node->is_leaf())
		{
			node = me_.GetLastChild(node);
		}

		return Iterator(node, me_.GetChildrenCount(node), me_, true);
	}
	else {
		return Iterator(me_);
	}
}


M_PARAMS
typename M_TYPE::Iterator M_TYPE::FindRStart()
{
	NodeBase *node = me_.GetRoot();
	if (node != NULL)
	{
		while(!node->is_leaf())
		{
			node = me_.GetLastChild(node);
		}

		return Iterator(node, me_.GetChildrenCount(node) - 1, me_, true);
	}
	else {
		return Iterator(me_);
	}
}

M_PARAMS
BigInt M_TYPE::GetTotalKeyCount() {
	NodeBase *node = me_.GetRoot();
	if (node != NULL) {
		return node->counters().key_count();
	}
	else {
		return 0;
	}
}

#undef M_TYPE
#undef M_PARAMS

}

#endif	//_MEMORIA_MODELS_KVMAP_MODEL_FIND_HPP
