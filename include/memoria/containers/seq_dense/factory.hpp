
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQDENSE_FACTORY_HPP
#define _MEMORIA_CONTAINERS_SEQDENSE_FACTORY_HPP

#include <memoria/containers/seq_dense/names.hpp>

#include <memoria/containers/seq_dense/iterator/seq_i_api.hpp>

#include <memoria/prototypes/sequence/factory.hpp>

namespace memoria {


template <typename Profile, Int BitsPerSymbol_>
struct BTreeTypes<Profile, memoria::Sequence<BitsPerSymbol_, true>>: public BTreeTypes<Profile, memoria::ASequence>  {

	typedef BTreeTypes<Profile, memoria::ASequence> Base;

	typedef typename AppendTool<
			typename Base::IteratorPartsList,
			TypeList<
				memoria::seq_dense::IterAPIName
			>
	>::Result                                                                   IteratorPartsList;

	static const Int BitsPerSymbol 												= BitsPerSymbol_;

};






template <typename Profile, typename T, Int BitsPerSymbol>
class CtrTF<Profile, memoria::Sequence<BitsPerSymbol, true>, T>: public CtrTF<Profile, memoria::ASequence, T> {

	typedef CtrTF<Profile, memoria::ASequence, T> 								Base;

public:

	typedef typename Base::ContainerTypes                                       ContainerTypes;

	typedef typename ContainerTypes::DataPagePartsList                          DataPagePartsList;

    MEMORIA_STATIC_ASSERT(IsList<DataPagePartsList>::Value);


    typedef SequenceDataPage<
    			DataPagePartsList,
    			UInt,
    			typename Base::Types::ElementType,
    			BitsPerSymbol,
    			memoria::btree::TreePage<
    				typename Base::ContainerTypes::Allocator::Page
    			>
    >                                                                           DataPage_;

    typedef typename Base::Types::ElementType ElementType;

    struct Types: Base::Types {

    	typedef typename Base::Types                                            Base0;

    	typedef DataPage_                                                       DataPage;
    	typedef PageGuard<DataPage, typename Base0::Allocator>                  DataPageG;
    	typedef ISequenceDataSource<ElementType, BitsPerSymbol>                 IDataSourceType;
    	typedef ISequenceDataTarget<ElementType, BitsPerSymbol>                 IDataTargetType;


    	typedef typename AppendTool<
    					TypeList<
    						DataPage_
    					>,
    					typename Base0::DataPagesList
    	>::Result                                                               DataPagesList;


    	typedef typename Base0::ContainerPartsList                              CtrList;
    	typedef typename Base0::IteratorPartsList                               IterList;

    	typedef SequenceCtrTypes<Types>                                        	CtrTypes;
    	typedef SequenceIterTypes<Types>                                       	IterTypes;

    	typedef DataPath<
    			typename Base0::NodeBaseG,
    			DataPageG
    	>                                                                       TreePath;

    	typedef typename TreePath::DataItem                                     DataPathItem;
    };

    typedef typename Types::CtrTypes                                            CtrTypes;

    typedef Ctr<CtrTypes>                                                       Type;

};


//template <typename Profile, Int BitsPerSymbol>
//struct BTreeTypes<Profile, memoria::Sequence<BitsPerSymbol, true>>: public BTreeTypes<Profile, memoria::BSTree<UBigInt>> {
//
//    typedef BTreeTypes<Profile, memoria::Vector<UBigInt>>              			Base;
//
//    static const Int Indexes = 1 + (BitsPerSymbol == 1 ? 1 : 1<<BitsPerSymbol);
//
//    typedef UBigInt																ElementType;
//
//    typedef typename AppendTool<
//        		typename Base::ContainerPartsList,
//        		TypeList<
//        			memoria::seq_dense::CtrFindName,
//        			memoria::seq_dense::CtrInsertName,
//        			memoria::seq_dense::CtrRemoveName
//        		>
//    >::Result                                                                   ContainerPartsList;
//
//    typedef typename AppendTool<
//            		typename Base::IteratorPartsList,
//            		TypeList<
//            			memoria::seq_dense::IterAPIName
//            		>
//    >::Result                                                                   IteratorPartsList;
//
//    template <typename Iterator, typename Container>
//    struct IteratorCacheFactory {
//    	typedef BTreeIteratorPrefixCache<Iterator, Container> Type;
//    };
//
//    typedef ISequenceDataSource<ElementType, BitsPerSymbol>                     IDataType;
//
//    typedef SequenceMetadata<typename Base::ID>                                 Metadata;
//
//};
//
//
//
//template <typename Profile, typename T, Int BitsPerSymbol>
//class CtrTF<Profile, memoria::Sequence<BitsPerSymbol, true>, T>: public CtrTF<Profile, memoria::Vector<UBigInt>, T> {
//
//	typedef CtrTF<Profile, memoria::Vector<UBigInt>, T> 						Base;
//
//	typedef typename Base::ContainerTypes::DataPagePartsList                    DataPagePartsList;
//
//
//
//    typedef SequenceDataPage<
//    			DataPagePartsList,
//    			UInt,
//    			typename Base::Types::ElementType,
//    			BitsPerSymbol,
//    			memoria::btree::TreePage<
//    				typename Base::ContainerTypes::Allocator::Page
//    			>
//    >                                                                           DataPage_;
//
//
//public:
//
//	struct Types: Base::Types
//	{
//		typedef DataPage_                                                       DataPage;
//		typedef PageGuard<DataPage, typename Base::Types::Allocator>            DataPageG;
//		typedef ISequenceDataSource<
//					typename Base::Types::ElementType,
//					BitsPerSymbol
//		>                        												IDataType;
//
//
//		typedef typename AppendTool<
//					TypeList<
//						DataPage_
//					>,
//					typename Base::Types::DataPagesList
//		>::Result                                                               DataPagesList;
//
//
//		typedef DataPath<
//					typename Base::Types::NodeBaseG,
//					DataPageG
//		>                                                                       TreePath;
//
//		typedef typename TreePath::DataItem                                     DataPathItem;
//
//
//		typedef SequenceCtrTypes<Types>                    	CtrTypes;
//		typedef SequenceIterTypes<Types>                   	IterTypes;
//	};
//
//	typedef typename Types::CtrTypes                                            CtrTypes;
//
//	typedef Ctr<CtrTypes>                                                       Type;
//};

}

#endif
