
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_SET_FACTORY_HPP
#define _MEMORIA_MODELS_IDX_SET_FACTORY_HPP


#include <memoria/containers/idx_map/factory.hpp>

namespace memoria    {



template <typename Profile, Int Indexes>
struct BTreeTypes<Profile, memoria::IdxSet<Indexes> >: public BTreeTypes<Profile, memoria::IdxMap<Indexes> > {

	typedef BTreeTypes<Profile, memoria::IdxSet<Indexes> > 					Base;

	typedef EmptyValue														Value;

	static const bool MapType                                               = MapTypes::Index;
};

template <typename Profile, typename T, Int Indexes>
class CtrTF<Profile, memoria::IdxSet<Indexes>, T>: public CtrTF<Profile, memoria::IdxMap<Indexes>, T> {
};

}

#endif
