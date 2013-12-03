
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
class NoRtnLeveledNodeWalkerBase {

	struct LeafStreamFn {
		MyType& walker_;

		LeafStreamFn(MyType& walker): walker_(walker) {}

    	template <Int StreamIdx, typename Stream, typename... Args>
    	void stream(const Stream* stream, Args&&... args)
    	{
    		walker_.template leafStream<StreamIdx>(stream, args...);
    	}
	};

	struct NonLeafStreamFn {
		MyType& walker_;

		NonLeafStreamFn(MyType& walker): walker_(walker) {}

    	template <Int StreamIdx, typename Stream, typename... Args>
    	void stream(const Stream* stream, Args&&... args)
    	{
    		walker_.template nonLeafStream<StreamIdx>(stream, args...);
    	}
	};


public:


	template <typename NodeTypes, typename... Args>
	void treeNode(const bt::LeafNode<NodeTypes>* node, Args&&... args)
	{
		node->template processStream<StreamIndex>(LeafStreamFn(self()), args...);
	}

	template <typename NodeTypes, typename... Args>
	void treeNode(const bt::BranchNode<NodeTypes>* node, Args&&... args)
	{
		node->template processStream<StreamIndex>(NonLeafStreamFn(self()), args...);
	}

	MyType& self() {return *T2T<MyType*>(this);}
	const MyType& self() const {return *T2T<const MyType*>(this);}
};





template <typename MyType, typename ReturnType_ = Int, Int StreamIndex = 0>
class RtnLeveledNodeWalkerBase: public RtnPkdHandlerBase<ReturnType_> {

	struct LeafStreamFn: RtnPkdHandlerBase<ReturnType_> {
		MyType& walker_;

		LeafStreamFn(MyType& walker): walker_(walker) {}

    	template <Int StreamIdx, typename Stream, typename... Args>
    	ReturnType_ stream(const Stream* stream, Args&&... args)
    	{
    		return walker_.template leafStream<StreamIdx>(stream, args...);
    	}
	};

	struct NonLeafStreamFn: RtnPkdHandlerBase<ReturnType_> {
		MyType& walker_;

		NonLeafStreamFn(MyType& walker): walker_(walker) {}

    	template <Int StreamIdx, typename Stream, typename... Args>
    	ReturnType_ stream(const Stream* stream, Args&&... args)
    	{
    		return walker_.template nonLeafStream<StreamIdx>(stream, args...);
    	}
	};


public:

	template <typename NodeTypes, typename... Args>
	ReturnType_ treeNode(const bt::LeafNode<NodeTypes>* node, Args&&... args)
	{
		return node->template processStreamRtn<StreamIndex>(LeafStreamFn(self()), args...);
	}

	template <typename NodeTypes, typename... Args>
	ReturnType_ treeNode(const bt::BranchNode<NodeTypes>* node, Args&&... args)
	{
		return node->template processStreamRtn<StreamIndex>(NonLeafStreamFn(self()), args...);
	}

	MyType& self() {return *T2T<MyType*>(this);}
	const MyType& self() const {return *T2T<const MyType*>(this);}
};




template <typename MyType, typename ReturnType_ = Int, Int StreamIndex = 0>
struct RtnNodeWalkerBase: RtnPkdHandlerBase<ReturnType_> {

	template <typename Node, typename... Args>
	ReturnType_ treeNode(Node* node, Args&&... args)
	{
		return node->template processStreamRtn<StreamIndex>(self(), args...);
	}

	MyType& self() {return *T2T<MyType*>(this);}
	const MyType& self() const {return *T2T<const MyType*>(this);}
};


template <typename MyType, Int StreamIndex = 0>
struct NoRtnNodeWalkerBase {

	template <typename Node, typename... Args>
	void treeNode(Node* node, Args&&... args)
	{
		node->template processStream<StreamIndex>(self(), args...);
	}

	MyType& self() {return *T2T<MyType*>(this);}
	const MyType& self() const {return *T2T<const MyType*>(this);}
};





}
}

#endif
