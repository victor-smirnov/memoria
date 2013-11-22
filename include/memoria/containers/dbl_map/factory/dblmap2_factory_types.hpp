
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_DBLMAP2_FACTORY_TYPES_HPP
#define _MEMORIA_CONTAINERS_DBLMAP2_FACTORY_TYPES_HPP

#include <memoria/containers/dbl_map/walkers/dblmap2_edge_walkers.hpp>
#include <memoria/containers/dbl_map/walkers/dblmap2_find_walkers.hpp>
#include <memoria/containers/dbl_map/walkers/dblmap2_skip_walkers.hpp>
#include <memoria/containers/dbl_map/dblmap_tools.hpp>
#include <memoria/containers/dbl_map/dblmap_names.hpp>

#include <memoria/containers/dbl_map/container/dblmap2_c_base.hpp>
#include <memoria/containers/dbl_map/container/dblmap2_c_api.hpp>
#include <memoria/containers/dbl_map/container/outermap_c_api.hpp>
#include <memoria/containers/dbl_map/container/outermap_c_insert.hpp>
#include <memoria/containers/dbl_map/container/innermap_c_api.hpp>


#include <memoria/containers/dbl_map/dblmap2_iterator.hpp>
#include <memoria/containers/dbl_map/iterator/outermap_i_api.hpp>
#include <memoria/containers/dbl_map/iterator/innermap_i_api.hpp>
#include <memoria/containers/dbl_map/iterator/innermap_i_nav.hpp>
#include <memoria/containers/dbl_map/iterator/dblmap2_i_api.hpp>

#include <memoria/containers/dbl_map/dblmap_names.hpp>

#include <memoria/containers/map/container/map_c_insert.hpp>
#include <memoria/containers/map/container/map_c_remove.hpp>

#include <memoria/containers/map/map_iterator.hpp>
#include <memoria/containers/map/iterator/map_i_api.hpp>
#include <memoria/containers/map/iterator/map_i_nav.hpp>
#include <memoria/containers/map/iterator/mrkmap_i_value.hpp>

#include <memoria/containers/map/map_names.hpp>

#include <memoria/core/packed/map/packed_fse_mark_map.hpp>

#include <memoria/core/types/typehash.hpp>

namespace memoria 	{

namespace dblmap {

template <typename K>
class OuterMap {};

template <typename K, typename V, Int BitsPerMark>
class InnerMap {};

}


template <typename Key, typename Value, Int BitsPerMark>
struct TypeHash<dblmap::InnerMap<Key, Value, BitsPerMark>>:   UIntValue<
    HashHelper<11021, TypeHash<Key>::Value, TypeHash<Value>::Value, BitsPerMark>::Value
> {};

template <typename Key>
struct TypeHash<dblmap::OuterMap<Key>>:   UIntValue<
    HashHelper<11022, TypeHash<Key>::Value>::Value
> {};




template <typename Profile_, typename K, typename V, Int BitsPerMark>
struct CompositeTypes<Profile_, DblMrkMap2<K, V, BitsPerMark> >: public CompositeTypes<Profile_, Composite> {

    typedef Map2<K, MrkMap2<K, V, BitsPerMark>>                                 ContainerTypeName;

    typedef CompositeTypes<Profile_, Composite>                                 Base;

    typedef K																	Key;
    typedef MarkedValue<V>                                                 		Value;

    typedef dblmap::OuterMap<K>													OuterMapName;
    typedef dblmap::InnerMap<K, V, BitsPerMark>									InnerMapName;

    typedef typename MergeLists<
            typename Base::ContainerPartsList,

            dblmap::CtrApi2Name
    >::Result                                                                   CtrList;

    typedef typename MergeLists<
            typename Base::IteratorPartsList,
            dblmap::ItrApi2Name
    >::Result                                                                   IterList;


    template <typename Types_>
    struct CtrBaseFactory {
        typedef dblmap::DblMap2CtrBase<Types_>             	Type;
    };
};







template <typename Profile, typename Key_>
struct BTTypes<Profile, dblmap::OuterMap<Key_> >: public BTTypes<Profile, memoria::BT>
{

    typedef BTTypes<Profile, memoria::BT>                   					Base;

    typedef Key_                                                              	Key;
    typedef BigInt                                                              Value;
    typedef TypeList<Key_>                                                    	KeysList;

    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef dblmap::OuterMapIteratorPrefixCache<Iterator, Container>        Type;
    };

    typedef TypeList<
                // Outer Map
                StreamDescr<
                    PkdFTreeTF,
                    PkdFTreeTF,
                    2 // node & leaf indexes
                >
    >                                                                           StreamDescriptors;

    typedef BalancedTreeMetadata<
                    typename Base::ID,
                    ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


    typedef typename MergeLists<
            typename Base::ContainerPartsList,
            memoria::bt::NodeComprName,

            dblmap::OuterCtrInsertName,
            map::CtrRemoveName,

            memoria::dblmap::OuterCtrApiName
    >::Result                                                                   ContainerPartsList;

    typedef typename MergeLists<
            typename Base::IteratorPartsList,

            map::ItrNavName,

            memoria::dblmap::OuterItrApiName
    >::Result                                                                   IteratorPartsList;


    typedef StaticVector<BigInt, 2>												IOValue;

    typedef IDataSource<IOValue>                                                DataSource;
    typedef IDataTarget<IOValue>                                                DataTarget;


    template <typename Types>
    using FindGEWalker              = dblmap::outer::FindGEWalker<Types>;
    template <typename Types>
    using FindGTWalker              = dblmap::outer::FindGTWalker<Types>;


    template <typename Types>
    using SkipForwardWalker         = dblmap::SkipForwardWalker<Types>;

    template <typename Types>
    using SkipBackwardWalker        = dblmap::SkipBackwardWalker<Types>;


    template <typename Types>
    using NextLeafWalker            = bt::NextLeafWalker<Types>;

    template <typename Types>
    using PrevLeafWalker            = dblmap::PrevLeafWalker<Types>;

    template <typename Types>
    using NextLeafMutistreamWalker  = bt::NextLeafMultistreamWalker<Types>;


    template <typename Types>
    using PrevLeafMutistreamWalker  = bt::PrevLeafMultistreamWalker<Types>;



    template <typename Types>
    using FindBeginWalker           = dblmap::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker             = dblmap::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker          = dblmap::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker            = dblmap::FindREndWalker<Types>;
};



template <typename Profile, typename Key_, typename Value_, Int BitsPerMark_>
struct BTTypes<Profile, dblmap::InnerMap<Key_, Value_, BitsPerMark_> >: public BTTypes<Profile, memoria::BT>
{

    typedef BTTypes<Profile, memoria::BT>                   					Base;

    typedef Key_                                                              	Key;
    typedef MarkedValue<Value_>                                                 Value;

    static const Int BitsPerMark												= BitsPerMark_;

    typedef TypeList<Key_>                                                    	KeysList;

    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef dblmap::InnerMapIteratorPrefixCache<Iterator, Container>       Type;
    };

    typedef TypeList<
                // Inner Map
    			StreamDescr<
    				PkdFTreeTF,
    				dblmap::PackedInnerMarkedMapLeafTF,
    				2, // node indexes
    				1  // leaf indexes
    			>
    >                                                                           StreamDescriptors;

    typedef BalancedTreeMetadata<
                    typename Base::ID,
                    ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


    typedef typename MergeLists<
            typename Base::ContainerPartsList,
            memoria::bt::NodeComprName,

            map::CtrInsertName,
            map::CtrRemoveName,

            memoria::dblmap::InnerCtrApiName
    >::Result                                                                   ContainerPartsList;

    typedef typename MergeLists<
            typename Base::IteratorPartsList,

            //map::ItrNavName,
            map::ItrMrkValueName,

            memoria::dblmap::InnerItrApiName,
            memoria::dblmap::InnerItrNavName
    >::Result                                                                   IteratorPartsList;


    typedef std::pair<StaticVector<BigInt, 1>, Value>							IOValue;

    typedef IDataSource<IOValue>                                                DataSource;
    typedef IDataTarget<IOValue>                                                DataTarget;


    template <typename Types>
    using FindGEWalker              = dblmap::inner::FindGEWalker<Types>;
    template <typename Types>
    using FindGTWalker              = dblmap::inner::FindGTWalker<Types>;


    template <typename Types>
    using SkipForwardWalker         = dblmap::inner::SkipForwardWalker<Types>;

    template <typename Types>
    using SkipBackwardWalker        = dblmap::inner::SkipBackwardWalker<Types>;


    template <typename Types>
    using NextLeafWalker            = bt::NextLeafWalker<Types>;

    template <typename Types>
    using PrevLeafWalker            = dblmap::PrevLeafWalker<Types>;

    template <typename Types>
    using NextLeafMutistreamWalker  = bt::NextLeafMultistreamWalker<Types>;


    template <typename Types>
    using PrevLeafMutistreamWalker  = bt::PrevLeafMultistreamWalker<Types>;



    template <typename Types>
    using FindBeginWalker           = dblmap::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker             = dblmap::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker          = dblmap::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker            = dblmap::FindREndWalker<Types>;
};




}

#endif
