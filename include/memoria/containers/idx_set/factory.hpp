
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_SET_FACTORY_HPP
#define _MEMORIA_MODELS_IDX_SET_FACTORY_HPP


#include <memoria/containers/idx_map/factory.hpp>

namespace memoria    {



template <typename Profile, Int Indexes>
struct BTreeTypes<Profile, memoria::SumSet<Indexes> >: public BTreeTypes<Profile, memoria::SumMap<Indexes> > {

	typedef BTreeTypes<Profile, memoria::SumSet<Indexes> > 					Base;

	typedef EmptyValue														Value;

	static const bool MapType                                               = MapTypes::Index;
};

template <typename Profile, typename T, Int Indexes>
class CtrTF<Profile, memoria::SumSet<Indexes>, T>: public CtrTF<Profile, memoria::SumMap<Indexes>, T> {
};

}

#endif
