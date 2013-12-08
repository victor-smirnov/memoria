
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SMRKMAP2_FACTORY_TYPES_HPP
#define _MEMORIA_CONTAINERS_SMRKMAP2_FACTORY_TYPES_HPP


#include <memoria/prototypes/metamap/metamap_factory.hpp>

namespace memoria    {

template <typename Profile, typename Key_, typename Value_, Int BitsPerLabel>
struct BTTypes<Profile, SMrkMap<Key_, Value_, BitsPerLabel> >:
	public BTTypes<Profile, MetaMap<
								1,
								Key_,
								Value_,
								TypeList<LabelDescr<2>>
							>
	> {

};




}

#endif
