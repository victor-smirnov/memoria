
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MISC_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MISC_WALKERS_HPP

#include <memoria/prototypes/bt/walkers/bt_find_walkers.hpp>
#include <memoria/core/packed/tools/packed_tools.hpp>

namespace memoria {
namespace bt1 	  {

template <typename MyType, Int StreamIndex = 0>
class UpWalkerBase {

	struct LeafStreamFn {
		MyType& walker_;

		LeafStreamFn(MyType& walker): walker_(walker) {}

    	template <Int StreamIdx, typename Stream>
    	void stream(const Stream* stream, Int idx)
    	{
    		return walker_.template leafStream<StreamIdx>(stream, idx);
    	}
	};

	struct NonLeafStreamFn {
		MyType& walker_;

		NonLeafStreamFn(MyType& walker): walker_(walker) {}

    	template <Int StreamIdx, typename Stream>
    	void stream(const Stream* stream, Int idx)
    	{
    		return walker_.template nonLeafStream<StreamIdx>(stream, idx);
    	}
	};


public:
	UpWalkerBase() {}

	template <typename NodeTypes>
	void treeNode(const bt::LeafNode<NodeTypes>* node, Int idx)
	{
		node->template processStream<StreamIndex>(LeafStreamFn(self()), idx);
	}

	template <typename NodeTypes>
	void treeNode(const bt::BranchNode<NodeTypes>* node, Int idx)
	{
		node->template processStream<StreamIndex>(NonLeafStreamFn(self()), idx);
	}

	MyType& self() {return *T2T<MyType*>(this);}
	const MyType& self() const {return *T2T<const MyType*>(this);}
};



template <typename MyType, typename ReturnType_ = Int, Int StreamIndex = 0>
struct RtnWalkerBase: RtnPkdHandlerBase<ReturnType_> {

	template <typename Node>
	ReturnType_ treeNode(Node* node, Int idx)
	{
		return node->template processStreamRtn<StreamIndex>(self(), idx);
	}

	MyType& self() {return *T2T<MyType*>(this);}
	const MyType& self() const {return *T2T<const MyType*>(this);}
};



}
}

#endif
