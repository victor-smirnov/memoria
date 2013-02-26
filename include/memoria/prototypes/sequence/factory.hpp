
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_SEQUENCE_FACTORY_HPP
#define _MEMORIA_PROTOTYPES_SEQUENCE_FACTORY_HPP

#include <memoria/prototypes/bstree/factory.hpp>

#include <memoria/prototypes/sequence/names.hpp>
#include <memoria/prototypes/sequence/sequence_walkers.hpp>

#include <memoria/prototypes/sequence/container/seq_c_insert.hpp>
#include <memoria/prototypes/sequence/container/seq_c_find.hpp>
#include <memoria/prototypes/sequence/container/seq_c_remove.hpp>
#include <memoria/prototypes/sequence/container/seq_c_tools.hpp>
#include <memoria/prototypes/sequence/container/seq_c_checks.hpp>

#include <memoria/prototypes/sequence/pages/seq_datapage.hpp>
#include <memoria/prototypes/sequence/pages/metadata.hpp>

#include <memoria/prototypes/sequence/iterator.hpp>

#include <memoria/prototypes/sequence/iterator/seq_i_api.hpp>

#include <memoria/core/tools/isequencedata.hpp>

namespace memoria {

template <
    typename DataPage_,
    typename IData_,
    typename Base
>
struct SequenceContainerTypes: public Base {

    typedef typename MergeLists<
                       DataPage_,
                       typename Base::DataPagesList
    >::Result                                                                   DataPagesList;

    typedef DataPage_                                                           DataPage;
    typedef PageGuard<DataPage, typename Base::Allocator>                       DataPageG;
    typedef IData_                                                             	IData;
};


template <typename Profile>
struct BTreeTypes<Profile, memoria::ASequence>: public BTreeTypes<Profile, memoria::BSTree>  {

    typedef IDType                                                              Value;
    typedef BTreeTypes<Profile, memoria::BSTree>                                Base;

    typedef TypeList<>                                                          DataPagePartsList;

    typedef typename MergeLists<
    			typename Base::ContainerPartsList,
    			memoria::sequence::CtrToolsName,
    			memoria::sequence::CtrRemoveName,
    			memoria::sequence::CtrInsertName,
    			memoria::sequence::CtrChecksName,
    			memoria::sequence::CtrFindName
    >::Result                                                                   ContainerPartsList;

    typedef typename MergeLists<
    			typename Base::IteratorPartsList,
    			memoria::sequence::IterAPIName
    >::Result                                                                   IteratorPartsList;


    typedef UBigInt                                                        		ElementType;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
    	typedef BTreeIteratorScalarPrefixCache<Iterator, Container> Type;
    };

    typedef SequenceMetadata<typename Base::ID>                                 Metadata;


    template <typename Types>
    using FindLTWalker 		= ::memoria::sequence::FindLTWalker<Types>;

    template <typename Types>
    using FindLEWalker 		= ::memoria::sequence::FindLEWalker<Types>;


    template <typename Types>
    using FindBeginWalker 	= ::memoria::sequence::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker 	= ::memoria::sequence::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker 	= ::memoria::sequence::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker 	= ::memoria::sequence::FindREndWalker<Types>;


    template <
    	typename Types,
    	template <typename, typename> class NodeExtender,
    	template <typename, typename> class DataExtender,
    	typename ExtenderState
    >
    using SkipForwardWalker = sequence::SequenceSkipForwardWalker<Types, NodeExtender, DataExtender, ExtenderState>;


    template <
    	typename Types,
    	template <typename, typename> class NodeExtender,
    	template <typename, typename> class DataExtender,
    	typename ExtenderState
    >
    using SkipBackwardWalker = sequence::SequenceSkipBackwardWalker<Types, NodeExtender, DataExtender, ExtenderState>;
};


template <typename Profile, typename T>
class CtrTF<Profile, memoria::ASequence, T>: public CtrTF<Profile, memoria::BSTree, T> {

};


}

#endif
