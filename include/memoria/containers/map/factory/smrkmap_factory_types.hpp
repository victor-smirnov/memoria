
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SMRKMAP2_FACTORY_TYPES_HPP
#define _MEMORIA_CONTAINERS_SMRKMAP2_FACTORY_TYPES_HPP

#include <memoria/prototypes/bt/bt_factory.hpp>
#include <memoria/prototypes/ctr_wrapper/ctrwrapper_factory.hpp>

#include <memoria/containers/map/map_walkers.hpp>
#include <memoria/containers/map/map_tools.hpp>

#include <memoria/containers/map/container/map_c_insert.hpp>
#include <memoria/containers/map/container/map_c_remove.hpp>
#include <memoria/containers/map/container/map_c_api.hpp>

#include <memoria/containers/map/map_iterator.hpp>
#include <memoria/containers/map/iterator/map_i_api.hpp>
#include <memoria/containers/map/iterator/map_i_nav.hpp>
#include <memoria/containers/map/iterator/map_i_value.hpp>

#include <memoria/containers/map/map_names.hpp>

#include <memoria/prototypes/metamap/metamap_factory.hpp>

namespace memoria    {

template <typename Profile, typename Key_, typename Value_, Int BitsPerLabel>
struct BTTypes<Profile, SMrkMap<Key_, Value_, BitsPerLabel> >:
	public BTTypes<Profile, MetaMap<
								1,
								Key_,
								Value_,
								TypeList<>,
								TypeList<LabelDescr<2>>
							>
	> {

};




}

#endif
