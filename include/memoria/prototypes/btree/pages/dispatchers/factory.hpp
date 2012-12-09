
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_PAGES_DISPATCHERS_FACTORY_HPP
#define _MEMORIA_PROTOTYPES_BTREE_PAGES_DISPATCHERS_FACTORY_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/types/relation.hpp>


#include <memoria/prototypes/btree/pages/dispatchers/defs.hpp>
#include <memoria/prototypes/btree/pages/dispatchers/tools.hpp>

namespace memoria    {
namespace btree      {

template <
    typename LevelList,
    typename Types
>
class NodeFactoryHelper;

template <
    typename Types
>
class NodeFactoryHelper<TypeList<>, Types> {

    typedef typename Types::RootNode            RootNode;
    typedef typename Types::LeafNode            LeafNode;
    typedef typename Types::RootLeafNode        RootLeafNode;
    typedef typename Types::Node                Node;

    typedef typename Types::NodeBase            NodeBase;
    typedef typename Types::NodeBaseG           NodeBaseG;
    typedef typename Types::Allocator           Allocator;

public:
    static NodeBaseG create(Allocator &allocator, Int level, bool root, bool leaf, Int size)
    {
        NodeBaseG node = allocator.createPage(size);
        node->init();

        if (!root && !leaf) {
            node->page_type_hash() = Node::hash();
        }
        else if (!root && leaf) {
            node->page_type_hash() = LeafNode::hash();
        }
        else if (root && !leaf) {
            node->page_type_hash() = RootNode::hash();
        }
        else {
            node->page_type_hash() = RootLeafNode::hash();
        }

        node->level() = level;
        node->set_root(root);
        node->set_leaf(leaf);

        return node;
    }
};


template <
    typename Head,
    typename ... Tail,
    typename Types
>
class NodeFactoryHelper<TypeList<Head, Tail...>, Types> {

    typedef typename Types::NodeBase            NodeBase;
    typedef typename Types::NodeBaseG           NodeBaseG;
    typedef typename Types::Allocator           Allocator;

    static const Int Level = Head::Value;

    typedef typename NodeFilter<ValueOp<LEVEL, EQ, Short, Level>, typename Types::NodeList>::Result LevelList;

    struct BaseWrapper {
        typedef NodeBase Type;
    };

    template <bool Root, bool Leaf>
    struct ListBuilder {
        typedef typename SelectHeadIfNotEmpty <
                typename NodeFilter <
                    And <
                        ValueOp<ROOT, EQ, bool, Root>,
                        ValueOp<LEAF, EQ, bool, Leaf>
                    >,
                    LevelList
                >::Result,
                BaseWrapper
        >::Result                                                               Type;
    };

    typedef typename ListBuilder<false, false>::Type                            Node;
    typedef typename ListBuilder<false, true>::Type                             LeafNode;
    typedef typename ListBuilder<true, false>::Type                             RootNode;
    typedef typename ListBuilder<true, true>::Type                              RootLeafNode;

public:
    static NodeBaseG create(Allocator &allocator, Int level, bool root, bool leaf, Int size)
    {
        if (Level == level)
        {
            NodeBaseG node = allocator.createPage(size);
            node->init();

            if (!root && !leaf)
            {
                node->page_type_hash() = Node::hash();
            }
            else if (!root && leaf)
            {
                node->page_type_hash() = LeafNode::hash();
            }
            else if (root && !leaf)
            {
                node->page_type_hash() = RootNode::hash();
            }
            else
            {
                node->page_type_hash() = RootLeafNode::hash();
            }

            node->level() = level;
            node->set_root(root);
            node->set_leaf(leaf);

            return node;
        }
        else {
            return NodeFactoryHelper<
                        TypeList<Tail...>,
                        Types
                   >::create(allocator, level, root, leaf, size);
        }
    }
};

template <
    typename NodeTL,
    typename Types
>
class NodeFactoryTool: public NodeFactoryHelper <
                                typename LevelListBuilder<NodeTL>::List,
                                Types
                          >
{};


}
}

#endif
