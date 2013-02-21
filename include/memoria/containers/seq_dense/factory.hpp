
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQDENSE_FACTORY_HPP
#define _MEMORIA_CONTAINERS_SEQDENSE_FACTORY_HPP

#include <memoria/containers/seq_dense/names.hpp>
#include <memoria/containers/seq_dense/tools.hpp>

#include <memoria/containers/seq_dense/container/seq_c_checks.hpp>
#include <memoria/containers/seq_dense/container/seq_c_tools.hpp>
#include <memoria/containers/seq_dense/container/seq_c_find.hpp>

#include <memoria/containers/seq_dense/iterator/seq_i_api.hpp>

#include <memoria/prototypes/sequence/factory.hpp>

namespace memoria {


template <typename Profile, Int BitsPerSymbol_>
struct BTreeTypes<Profile, memoria::Sequence<BitsPerSymbol_, true>>: public BTreeTypes<Profile, memoria::ASequence>  {

	typedef BTreeTypes<Profile, memoria::ASequence> Base;

    typedef typename MergeLists<
            	typename Base::ContainerPartsList,
            	memoria::seq_dense::CtrChecksName,
                memoria::seq_dense::CtrToolsName,
                memoria::seq_dense::CtrFindName
    >::Result                                                           ContainerPartsList;

	typedef typename MergeLists<
				typename Base::IteratorPartsList,
				memoria::seq_dense::IterAPIName
	>::Result                                                           IteratorPartsList;

	static const Int BitsPerSymbol 										= BitsPerSymbol_;
    static const Int Indexes											= 1 + (1 << BitsPerSymbol);
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


    	typedef typename MergeLists<
    						DataPage_,
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

}

#endif
