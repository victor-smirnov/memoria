
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MAPX_FACTORY_HPP
#define _MEMORIA_CONTAINERS_MAPX_FACTORY_HPP

#include <memoria/containers/map/factory/map_factory_types.hpp>


#include <memoria/containers/mapx/mapx_names.hpp>
#include <memoria/containers/mapx/mapx_iterator.hpp>

namespace memoria    {

template <typename Profile, typename Key_, typename Value_>
struct BTTypes<Profile, MapX<Key_, Value_> >: public BTTypes<Profile, memoria::BT> {

};


template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, memoria::MapX<Key, Value>, T>: public CtrTF<Profile, memoria::BT, T> {
    using Base = CtrTF<Profile, memoria::BT, T>;
public:


    struct Types: Base::Types
    {
        typedef MapXCtrTypes<Types>                                             CtrTypes;
        typedef MapXIterTypes<Types>                                            IterTypes;

        typedef PageUpdateManager<CtrTypes>                                     PageUpdateMgr;
    };


    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef Ctr<CtrTypes>                                                       Type;
};








}

#endif
