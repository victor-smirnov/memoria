
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_PAGES_DISPATCHERS_DISPATCHER_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_PAGES_DISPATCHERS_DISPATCHER_HPP

namespace memoria    {
namespace btree      {

using memoria::TL;

template <typename Types, int idx> class NDT0;
template <typename Types, int idx> class NDT1;


template <typename Types>
class NDT: public NDT0<Types, ListSize<typename Types::List>::Value - 1> {};

template <typename Types>
class NDT0<Types, -1> {

	typedef typename Types::NodeBase NodeBase;

public:
    template <typename Functor>
    static void Dispatch(NodeBase*, Functor &) {
        throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    template <typename Functor>
    static void DispatchConst(const NodeBase*, Functor &) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    template <typename Functor>
    static void Dispatch(NodeBase* node1, NodeBase* node2, Functor &functor) {}

    template <typename Functor>
    static void DoubleDispatch(NodeBase* node1, NodeBase* node2, Functor &functor) {}


    template <typename Functor>
    static void DispatchConst(const NodeBase* node1, const NodeBase* node2, Functor &functor) {}

    template <typename Functor>
    static void DoubleDispatchConst(const NodeBase* node1, const NodeBase* node2, Functor &functor) {}

    static void BuildMetadataList(MetadataList &list) {}
};


template <typename Types>
class NDT1<Types, -1> {
	typedef typename Types::NodeBase NodeBase;
public:
    template <typename Node, typename Functor>
    static void Dispatch(Node*, NodeBase *, Functor &) {
        throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    template <typename Node, typename Functor>
    static void DispatchConst(const Node*, const NodeBase*, Functor &) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }
};


template <typename Types, int Idx>
class NDT1 {

	typedef typename Types::NodeBase NodeBase;
	typedef typename SelectByIndexTool<Idx, typename Types::List>::Result Head;

    static const bool   Root  = Head::Descriptor::Root;
    static const bool   Leaf  = Head::Descriptor::Leaf;
    static const BigInt Level = Head::Descriptor::Level;

public:

    template <typename Node, typename Functor>
    static void Dispatch(Node *node1, NodeBase *node2, Functor &functor)
    {
        bool level2EQ = Level == ANY_LEVEL ? true : Level == node2->level();

        if (level2EQ && Root == node2->is_root() && Leaf == node2->is_leaf())
        {
            functor(
                node1,
                static_cast<Head*>(node2)
            );
        }
        else {
            NDT1<Types, Idx - 1>::Dispatch(node1, node2, functor);
        }
    }

    template <typename Node, typename Functor>
    static void DispatchConst(const Node* node1, const NodeBase* node2, Functor &functor)
    {
    	bool level2EQ = Level == ANY_LEVEL ? true : Level == node2->level();

    	if (level2EQ && Root == node2->is_root() && Leaf == node2->is_leaf())
    	{
    		functor(
    				node1,
    				static_cast<const Head*>(node2)
    		);
    	}
    	else {
    		NDT1<Types, Idx - 1>::DispatchConst(node1, node2, functor);
    	}
    }
};


template <typename Types, int Idx>
class NDT0 {

	typedef typename Types::NodeBase NodeBase;
	typedef typename SelectByIndexTool<Idx, typename Types::List>::Result Head;

    static const bool   Root  = Head::Descriptor::Root;
    static const bool   Leaf  = Head::Descriptor::Leaf;
    static const BigInt Level = Head::Descriptor::Level;


public:
    template <typename Functor>
    static void Dispatch(NodeBase* node, Functor &functor)
    {
        bool level1EQ = Level == ANY_LEVEL ? true : Level == node->level();

        if (level1EQ && Root == node->is_root() && Leaf == node->is_leaf())
        {
            functor(static_cast<Head*>(node));
        }
        else {
            NDT0<Types, Idx - 1>::Dispatch(node, functor);
        }
    }

    template <typename Functor>
    static void DispatchConst(const NodeBase* node, Functor &functor)
    {
    	bool level1EQ = Level == ANY_LEVEL ? true : Level == node->level();

    	if (level1EQ && Root == node->is_root() && Leaf == node->is_leaf())
    	{
    		functor(static_cast<const Head*>(node));
    	}
    	else {
    		NDT0<Types, Idx - 1>::DispatchConst(node, functor);
    	}
    }

    template <typename Functor>
    static void Dispatch(NodeBase* node1, NodeBase* node2, Functor &functor)
    {
        bool level1EQ = Level == ANY_LEVEL ? true : Level == node1->level();

        if (level1EQ && Root == node1->is_root() && Leaf == node1->is_leaf())
        {
            functor(static_cast<Head*>(node1), static_cast<Head*>(node2));
        }
        else {
            NDT0<Types, Idx - 1>::Dispatch(node1, node2, functor);
        }
    }

    template <typename Functor>
    static void DispatchConst(const NodeBase* node1, const NodeBase* node2, Functor &functor)
    {
        bool level1EQ = Level == ANY_LEVEL ? true : Level == node1->level();

        if (level1EQ && Root == node1->is_root() && Leaf == node1->is_leaf())
        {
            functor(static_cast<const Head*>(node1), static_cast<const Head*>(node2));
        }
        else {
            NDT0<Types, Idx - 1>::DispatchConst(node1, node2, functor);
        }
    }

    template <typename Functor>
    static void DoubleDispatch(NodeBase* node1, NodeBase* node2, Functor &functor)
    {
        bool level1EQ = Level == ANY_LEVEL ? true : Level == node1->level();
        if (level1EQ && Root == node1->is_root() && Leaf == node1->is_leaf())
        {
           NDT1<Types, ListSize<typename Types::List>::Value - 1>::Dispatch(
            	static_cast<Head*>(node1),
            	node2,
            	functor
           );
        }
        else {
            NDT0<Types, Idx - 1>::DoubleDispatch(node1, node2, functor);
        }
    }

    template <typename Functor>
    static void DoubleDispatchConst(const NodeBase* node1, const NodeBase* node2, Functor &functor)
    {
    	bool level1EQ = Level == ANY_LEVEL ? true : Level == node1->level();
    	if (level1EQ && Root == node1->is_root() && Leaf == node1->is_leaf())
    	{
    		NDT1<Types, ListSize<typename Types::List>::Value - 1>::Dispatch(
    				static_cast<const Head*>(node1),
    				node2,
    				functor
    		);
    	}
    	else {
    		NDT0<Types, Idx - 1>::DoubleDispatchConst(node1, node2, functor);
    	}
    }


    static void BuildMetadataList(MetadataList &list) {
        Head::Init();
        list.push_back(Head::reflection());
        NDT0<Types, Idx - 1>::BuildMetadataList(list);
    }
};


}
}


#endif
