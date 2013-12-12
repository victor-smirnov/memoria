
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MAP_FACTORY_TYPES_HPP
#define _MEMORIA_CONTAINERS_MAP_FACTORY_TYPES_HPP

#include <memoria/prototypes/metamap/metamap_factory.hpp>

namespace memoria    {

template <typename Profile, typename Key_, typename Value_>
struct BTTypes<Profile, Map<Key_, Value_> >:
	public BTTypes<Profile, MetaMap<
								1,
								Key_,
								Value_,
								TypeList<LabelDescr<1>, LabelDescr<2>>,
								TypeList<LabelDescr<2>>
							>
	> {

};




}

#endif
