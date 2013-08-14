
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MAP_FACTORY_HPP
#define _MEMORIA_CONTAINERS_MAP_FACTORY_HPP

#include <memoria/containers/map/factory/map_factory_types.hpp>
#include <memoria/containers/map/factory/cmap_factory_types.hpp>

namespace memoria    {


template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, memoria::Map<Key, Value>, T>: public CtrTF<Profile, memoria::BT, T> {
};

template <typename Profile, Granularity gr, typename T>
class CtrTF<Profile, memoria::CMap<gr>, T>: public CtrTF<Profile, memoria::BT, T> {
};


}

#endif
