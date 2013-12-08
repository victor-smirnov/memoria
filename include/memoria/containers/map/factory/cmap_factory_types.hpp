
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_CMAP_FACTORY_TYPES_HPP
#define _MEMORIA_CONTAINERS_CMAP_FACTORY_TYPES_HPP


#include <memoria/prototypes/metamap/metamap_factory.hpp>

namespace memoria    {


template <typename Profile, Granularity gr>
struct BTTypes<Profile, memoria::CMap<gr> >: public BTTypes<Profile, MetaMap<1, VLen<gr, BigInt>, BigInt>> {

};



}

#endif
