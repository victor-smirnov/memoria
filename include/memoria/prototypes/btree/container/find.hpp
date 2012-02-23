
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

    static const Int Indexes                                                    = Types::Indexes;


private:


public:

    CtrPart():Base(){}
    virtual ~CtrPart() {}

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
                    node_ = model_.GetChild(node, idx_, Allocator::READ);
                }
                else {
                	node_           = model_.allocator().GetPageG(node);
                    i_.page()       = node_;
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
                    idx_     = node->children_count() - 1;
                    node_    = model_.GetChild(node, idx_, Allocator::READ);
                }
                else
                {
                    node_           = model_.allocator().GetPageG(node);
                    i_.page()       = node_;
                    i_.key_idx()    = node->children_count();
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

    Iterator FindREnd();

    Iterator FindEnd();

    Iterator FindRStart();

    BigInt GetTotalKeyCount();
    void SetTotalKeyCount(BigInt value);
    void AddTotalKeyCount(BigInt value);

    BigInt GetSize() {
    	return me()->GetTotalKeyCount();
    }

    Iterator Begin() {
    	return me()->FindStart();
    }

    Iterator RBegin() {
    	return me()->FindRStart();
    }

    Iterator End() {
    	return me()->FindEnd();
    }

    Iterator REnd() {
    	return me()->FindREnd();
    }

MEMORIA_CONTAINER_PART_END



#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::FindName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
template <typename Comparator>
const typename M_TYPE::Iterator M_TYPE::_find(Key key, Int c, bool for_insert)
{
	MEMORIA_TRACE(me(), "begin", key, c, for_insert, me()->root());
	NodeBaseG node = me()->GetRoot(Allocator::READ);

	if (node != NULL)
	{
		MEMORIA_TRACE(me(), "Init search, start from", node->id());
		Comparator cmp(for_insert);
		while(1)
		{
			FindFn<Comparator> fn(cmp, key, c, *me());

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
		MEMORIA_TRACE(me(), "No Root for Container");
		return Iterator(*me());
	}
}



M_PARAMS
typename M_TYPE::Iterator M_TYPE::FindStart()
{
	NodeBaseG node = me()->GetRoot(Allocator::READ);
	if (node != NULL)
	{
		while(!node->is_leaf())
		{
			node = me()->GetChild(node, 0, Allocator::READ);
		}

		return Iterator(node, 0, *me());
	}
	else {
		return Iterator(*me());
	}
}

M_PARAMS
typename M_TYPE::Iterator M_TYPE::FindREnd()
{
	NodeBaseG node = me()->GetRoot(Allocator::READ);
	if (node != NULL)
	{
		while(!node->is_leaf())
		{
			node = me()->GetChild(node, 0, Allocator::READ);
		}

		return Iterator(node, -1, *me());
	}
	else {
		return Iterator(*me());
	}
}

M_PARAMS
typename M_TYPE::Iterator M_TYPE::FindEnd()
{
	NodeBaseG node = me()->GetRoot(Allocator::READ);
	if (node != NULL)
	{
		while(!node->is_leaf())
		{
			node = me()->GetLastChild(node, Allocator::READ);
		}

		return Iterator(node, node->children_count(), *me(), true);
	}
	else {
		return Iterator(*me());
	}
}


M_PARAMS
typename M_TYPE::Iterator M_TYPE::FindRStart()
{
	NodeBaseG node = me()->GetRoot(Allocator::READ);
	if (node != NULL)
	{
		while(!node->is_leaf())
		{
			node = me()->GetLastChild(node, Allocator::READ);
		}

		return Iterator(node, node->children_count() - 1, *me(), true);
	}
	else {
		return Iterator(*me());
	}
}

M_PARAMS
BigInt M_TYPE::GetTotalKeyCount()
{
	NodeBaseG node = me()->GetRoot(Allocator::READ);

	if (node != NULL)
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


#undef M_TYPE
#undef M_PARAMS

}

#endif	//_MEMORIA_MODELS_KVMAP_MODEL_FIND_HPP
