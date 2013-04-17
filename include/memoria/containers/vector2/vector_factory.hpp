
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_VECTOR2_FACTORY_HPP
#define _MEMORIA_CONTAINERS_VECTOR2_FACTORY_HPP

#include <memoria/prototypes/balanced_tree/balanced_tree.hpp>
#include <memoria/prototypes/ctr_wrapper/ctrwrapper_factory.hpp>

#include <memoria/containers/vector2/vector_walkers.hpp>
#include <memoria/containers/vector2/vector_tools.hpp>
#include <memoria/containers/vector2/vector_names.hpp>

#include <memoria/containers/vector2/container/vector_c_tools.hpp>
#include <memoria/containers/vector2/container/vector_c_insert.hpp>
#include <memoria/containers/vector2/container/vector_c_remove.hpp>
#include <memoria/containers/vector2/container/vector_c_api.hpp>

#include <memoria/containers/vector2/vector_iterator.hpp>
#include <memoria/containers/vector2/iterator/vector_i_api.hpp>
#include <memoria/containers/vector2/iterator/baltree_i_api.hpp>

#include <memoria/containers/vector2/vector_names.hpp>

namespace memoria    {



template <typename Profile, typename Value_>
struct BalancedTreeTypes<Profile, memoria::Vector<Value_> >: public BalancedTreeTypes<Profile, memoria::BalancedTree> {

    typedef BalancedTreeTypes<Profile, memoria::BalancedTree>                   Base;

    typedef Value_                                                          	Value;
    typedef TypeList<BigInt>                                                  	KeysList;

    static const Int Indexes                                                	= 1;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef ::memoria::mvector2::VectorIteratorPrefixCache<Iterator, Container>               Type;
    };

    typedef TypeList<
    		NonLeafNodeTypes<balanced_tree::TreeMapNode>,
    		LeafNodeTypes<mvector2::TreeLeafNode>
    >																			NodeTypesList;

    typedef TypeList<
        		LeafNodeType<mvector2::TreeLeafNode>,
        		InternalNodeType<TreeMapNode>,
        		RootNodeType<TreeMapNode>,
        		RootLeafNodeType<mvector2::TreeLeafNode>
    >																			DefaultNodeTypesList;

    typedef TypeList<
        		StreamDescr<
        			PackedFSETreeTF,
        			PackedFSEArrayTF,
        			1
        	>
    >																			StreamDescriptors;


    typedef typename MergeLists<
    		typename Base::ContainerPartsList,
    		memoria::mvector2::CtrToolsName,
    		memoria::mvector2::CtrInsertName,
    		memoria::mvector2::CtrRemoveName,
    		memoria::mvector2::CtrApiName
    >::Result                                           						ContainerPartsList;

    typedef typename MergeLists<
    		typename Base::IteratorPartsList,
    		memoria::mvector2::ItrApiName,
    		memoria::mvector2::ItrBaltreeApiName
    >::Result                                           						IteratorPartsList;



    template <typename Types>
    using FindLTWalker 		= ::memoria::mvector2::FindLTForwardWalker<Types>;

    template <typename Types>
    using FindLEWalker 		= ::memoria::mvector2::FindLEWalker<Types>;



    template <typename Types>
    using FindBeginWalker 	= ::memoria::mvector2::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker 	= ::memoria::mvector2::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker 	= ::memoria::mvector2::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker 	= ::memoria::mvector2::FindREndWalker<Types>;
};


template <typename Profile, typename Value, typename T>
class CtrTF<Profile, memoria::Vector<Value>, T>: public CtrTF<Profile, memoria::BalancedTree, T> {
};




//template <typename Profile_, typename Value>
//struct WrapperTypes<Profile_, CtrWrapper<memoria::Vector<Value>>>: WrapperTypes<Profile_, memoria::Vector<Value> > {
//
//	typedef WrapperTypes<Profile_, memoria::Vector<Value>>   			Base;
//
//	typedef typename MergeLists<
//				typename Base::ContainerPartsList,
//				memoria::mvector2::CtrToolsName,
//				memoria::mvector2::CtrInsertName,
//				memoria::mvector2::CtrRemoveName,
//				memoria::mvector2::CtrApiName
//	>::Result                                           CtrList;
//
//	typedef typename MergeLists<
//				typename Base::IteratorPartsList,
//				memoria::mvector2::ItrApiName
//	>::Result                                           IterList;
//
//	typedef VectorProto<Value>           				WrappedCtrName;
//};
//
//
//
//template <typename Profile_, typename Value, typename T>
//class CtrTF<Profile_, memoria::Vector<Value>, T>: public CtrTF<Profile_, CtrWrapper<memoria::Vector<Value>>, T> {
//
//	typedef CtrTF<Profile_, CtrWrapper<memoria::Vector<Value>>, T> 				Base;
//
//public:
//
//    struct Types: Base::Types {
//    	typedef IDataSource<Value>				DataSource;
//
//    	typedef Vector2CtrTypes<Types> 			CtrTypes;
//        typedef Vector2IterTypes<Types> 		IterTypes;
//    };
//
//
//    typedef typename Types::CtrTypes                                            CtrTypes;
//    typedef typename Types::IterTypes                                           IterTypes;
//
//    typedef Ctr<CtrTypes>                                                       Type;
//};



}

#endif
