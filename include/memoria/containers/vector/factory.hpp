
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_VECTOR_FACTORY_HPP
#define _MEMORIA_CONTAINERS_VECTOR_FACTORY_HPP

#include <memoria/prototypes/sequence/factory.hpp>


#include <memoria/containers/vector/iterator/vector_i_api.hpp>
#include <memoria/containers/vector/iterator.hpp>
//
#include <memoria/containers/vector/pages/vector_datapage.hpp>
//#include <memoria/containers/vector/pages/metadata.hpp>

//#include <memoria/containers/vector/container/vector_c_api.hpp>
//#include <memoria/containers/vector/container/vector_c_insert.hpp>
//#include <memoria/containers/vector/container/vector_c_remove.hpp>
//#include <memoria/containers/vector/container/vector_c_tools.hpp>
//#include <memoria/containers/vector/container/vector_c_checks.hpp>
//#include <memoria/containers/vector/container/vector_c_find.hpp>
//#include <memoria/containers/vector/container/vector_c_read.hpp>

#include <memoria/containers/vector/names.hpp>
#include <memoria/containers/vector/tools.hpp>

#include <memoria/prototypes/sequence/tools.hpp>


namespace memoria {


//template <
//    typename DataPage_,
//    typename IData_,
//    typename Base
//>
//struct VectorContainerTypes: public Base {
//
//    typedef typename AppendTool<
//                    TypeList<
//                        DataPage_
//                    >,
//                    typename Base::DataPagesList
//    >::Result                                                                   DataPagesList;
//
//    typedef DataPage_                                                           DataPage;
//    typedef PageGuard<DataPage, typename Base::Allocator>                       DataPageG;
//    typedef IData_                                                             	IData;
//};
//
//
//template <typename Profile, typename ElementType_>
//struct BTreeTypes<Profile, memoria::Vector<ElementType_>>: public BTreeTypes<Profile, memoria::BSTree>  {
//
//    typedef IDType                                                              Value;
//    typedef BTreeTypes<Profile, memoria::BSTree>                                Base;
//
//    typedef TypeList<>                                                          DataPagePartsList;
//
//    typedef typename AppendTool<
//    		typename Base::ContainerPartsList,
//    		TypeList<
//    			memoria::mvector::ToolsName,
//    			memoria::mvector::RemoveName,
//    			memoria::mvector::InsertName,
//    			memoria::mvector::ChecksName,
//    			memoria::mvector::ReadName,
//    			memoria::mvector::SeekName,
//    			memoria::mvector::ApiName
//    		>
//    >::Result                                                                   ContainerPartsList;
//
//    typedef typename AppendTool<
//    		typename Base::IteratorPartsList,
//    		TypeList<
//    			memoria::mvector::IteratorContainerAPIName
//    		>
//    >::Result                                                                   IteratorPartsList;
//
//
//    typedef ElementType_                                                        ElementType;
//
//
//    template <typename Iterator, typename Container>
//    struct IteratorCacheFactory {
//    	typedef BTreeIteratorScalarPrefixCache<Iterator, Container> Type;
//    };
//
//    typedef IData<ElementType>                                              	IDataType;
//
//    typedef VectorMetadata<typename Base::ID>                                   Metadata;
//};
//
//
//
//
//
//
//template <typename Profile, typename T, typename ElementType>
//class CtrTF<Profile, memoria::Vector<ElementType>, T>: public CtrTF<Profile, memoria::BSTree, T> {
//
//	typedef CtrTF<Profile, memoria::BSTree, T> 									Base;
//
//public:
//
//	typedef typename Base::ContainerTypes                                       ContainerTypes;
//
//	typedef typename ContainerTypes::DataPagePartsList                          DataPagePartsList;
//
//    MEMORIA_STATIC_ASSERT(IsList<DataPagePartsList>::Value);
//
//
//
//    typedef VectorDataPage<
//    			DataPagePartsList,
//    			ElementType,
//    			memoria::btree::TreePage<
//    				typename ContainerTypes::Allocator::Page
//    			>
//    >                                                                           DataPage_;
//
//
//    struct Types: Base::Types {
//
//    	typedef typename Base::Types                                            Base0;
//
//    	typedef DataPage_                                                       DataPage;
//    	typedef PageGuard<DataPage, typename Base0::Allocator>                  DataPageG;
//    	typedef IData<ElementType>                                              IDataType;
//
//
//    	typedef typename AppendTool<
//    					TypeList<
//    						DataPage_
//    					>,
//    					typename Base0::DataPagesList
//    	>::Result                                                               DataPagesList;
//
//
//    	typedef typename Base0::ContainerPartsList                              CtrList;
//    	typedef typename Base0::IteratorPartsList                               IterList;
//
//    	typedef VectorCtrTypes<Types>                                        	CtrTypes;
//    	typedef VectorIterTypes<Types>                                       	IterTypes;
//
//    	typedef DataPath<
//    			typename Base0::NodeBaseG,
//    			DataPageG
//    	>                                                                       TreePath;
//
//    	typedef typename TreePath::DataItem                                     DataPathItem;
//    };
//
//    typedef typename Types::CtrTypes                                            CtrTypes;
//
//    typedef Ctr<CtrTypes>                                                       Type;
//
//};


template <typename Profile, typename ElementType_>
struct BTreeTypes<Profile, memoria::Vector<ElementType_>>: public BTreeTypes<Profile, memoria::ASequence>  {

	typedef BTreeTypes<Profile, memoria::ASequence> Base;

    typedef typename AppendTool<
    		typename Base::IteratorPartsList,
    		TypeList<
    			memoria::mvector::IteratorContainerAPIName
    		>
    >::Result                                                                   IteratorPartsList;


    typedef ElementType_                                                        ElementType;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
    	typedef BTreeIteratorScalarPrefixCache<Iterator, Container> Type;
    };

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


    	typedef typename AppendTool<
    					TypeList<
    						DataPage_
    					>,
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
