
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SMRKMAP_FACTORY_HPP
#define _MEMORIA_CONTAINERS_SMRKMAP_FACTORY_HPP

#include <memoria/containers/smrk_map/factory/smrkmap_factory_types.hpp>

namespace memoria    {

template <typename Profile, typename Key, typename Value, Int BitsPerMark, typename T>
class CtrTF<Profile, SMrkMap<Key, Value, BitsPerMark>, T>: public CtrTF<Profile, memoria::BT, T> {
};


}

#endif