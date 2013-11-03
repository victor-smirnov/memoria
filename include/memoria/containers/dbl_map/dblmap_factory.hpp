
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_DBLMAP_FACTORY_HPP
#define _MEMORIA_CONTAINERS_DBLMAP_FACTORY_HPP



#include <memoria/containers/dbl_map/factory/dblmap_factory_types.hpp>
#include <memoria/containers/dbl_map/factory/dblmrkmap_factory_types.hpp>

namespace memoria 	{



template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, DblMap<Key, Value>, T>: public CtrTF<Profile, memoria::BT, T> {
};


template <typename Profile, typename Key, typename Value, Int BitsPerMark, typename T>
class CtrTF<Profile, DblMrkMap<Key, Value, BitsPerMark>, T>: public CtrTF<Profile, memoria::BT, T> {
};



}

#endif
