
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_VECTOR_FACTORY_HPP
#define _MEMORIA_CONTAINERS_VECTOR_FACTORY_HPP

#include <memoria/prototypes/sequence/sequence_factory.hpp>

#include <memoria/containers/vector/container/vector_c_tools.hpp>

#include <memoria/containers/vector/iterator/vector_i_api.hpp>
#include <memoria/containers/vector/iterator.hpp>

#include <memoria/containers/vector/pages/vector_datapage.hpp>


#include <memoria/containers/vector/names.hpp>
#include <memoria/containers/vector/tools.hpp>

#include <memoria/prototypes/sequence/tools.hpp>


#include <memoria/containers/vector/vector_walkers.hpp>

namespace memoria {

template <typename Profile, typename ElementType_>
struct BTreeTypes<Profile, memoria::Vector<ElementType_>>: public BTreeTypes<Profile, memoria::ASequence>  {

	typedef BTreeTypes<Profile, memoria::ASequence> Base;

    typedef typename MergeLists<
            	typename Base::ContainerPartsList,
                memoria::mvector::CtrToolsName
    >::Result                                                               	ContainerPartsList;

    typedef typename MergeLists<
    			typename Base::IteratorPartsList,
    			memoria::mvector::IteratorContainerAPIName
    >::Result                                                                   IteratorPartsList;


    typedef ElementType_                                                        ElementType;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
    	typedef BTreeIteratorScalarPrefixCache<Iterator, Container> Type;
    };

//    template <
//    	typename Types,
//    	template <typename, typename> class NodeExtender,
//    	template <typename, typename> class DataExtender
//    >
//    using SkipForwardWalker = sequence::SequenceSkipForwardWalker<Types>;
//
//
//    template <
//    	typename Types,
//    	template <typename, typename> class NodeExtender,
//    	template <typename, typename> class DataExtender
//    >
//    using SkipBackwardWalker = mvector::VectorBackwardWalker<Types>;

};


template <typename Profile, typename T, typename ElementType>
class CtrTF<Profile, memoria::Vector<ElementType>, T>: public CtrTF<Profile, memoria::ASequence, T> {

	typedef CtrTF<Profile, memoria::ASequence, T> 								Base;

public:

	typedef typename Base::ContainerTypes                                       ContainerTypes;

	typedef typename ContainerTypes::DataPagePartsList                          DataPagePartsList;

    MEMORIA_STATIC_ASSERT(IsList<DataPagePartsList>::Value);



    typedef VectorDataPage<
    			DataPagePartsList,
    			ElementType,
    			memoria::btree::TreePage<
    				typename ContainerTypes::Allocator::Page
    			>
    >                                                                           DataPage_;


    struct Types: Base::Types {

    	typedef typename Base::Types                                            Base0;

    	typedef DataPage_                                                       DataPage;
    	typedef PageGuard<DataPage, typename Base0::Allocator>                  DataPageG;

    	typedef IData<ElementType>                                        		IDataType;
    	typedef IDataSource<ElementType>                                        IDataSourceType;
    	typedef IDataTarget<ElementType>                                        IDataTargetType;


    	typedef typename MergeLists<
    						DataPage_,
    						typename Base0::DataPagesList
    	>::Result                                                               DataPagesList;


    	typedef typename Base0::ContainerPartsList                              CtrList;
    	typedef typename Base0::IteratorPartsList                               IterList;

    	typedef VectorCtrTypes<Types>                                        	CtrTypes;
    	typedef VectorIterTypes<Types>                                       	IterTypes;

    	typedef DataPath<
    				typename Base0::NodeBaseG,
    				DataPageG
    	>                                                                       TreePath;

    	typedef typename TreePath::DataItem                                     DataPathItem;
    };

    typedef typename Types::CtrTypes                                            CtrTypes;

    typedef Ctr<CtrTypes>                                                       Type;

};


}

#endif
