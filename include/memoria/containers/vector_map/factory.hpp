
// Copyright Victor Smirnov, Ivan Yurchenko 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_BLOB_MAP_FACTORY_HPP
#define _MEMORIA_MODELS_BLOB_MAP_FACTORY_HPP


#include <memoria/containers/vector_map/container.hpp>
#include <memoria/containers/vector_map/iterator.hpp>

namespace memoria {

template <typename Types>
struct VectorMapCtrTypes: Types {};

template <typename Types>
struct VectorMapIterTypes: Types {};




template <typename Profile, Int Indexes_>
struct BTreeTypes<Profile, memoria::VMSet<Indexes_> >: public BTreeTypes<Profile, memoria::BSTree> {

	typedef BTreeTypes<Profile, memoria::BSTree > 							Base;

	typedef EmptyValue														Value;

	static const Int Indexes												= Indexes_;
};


template <typename Profile, typename T, Int Indexes>
class CtrTF<Profile, memoria::VMSet<Indexes>, T>: public CtrTF<Profile, memoria::BSTree, T> {
};




template <typename Profile_, typename T>
class CtrTF<Profile_, VectorMap, T> {

	typedef CtrTF<Profile_, VectorMap, T> 										MyType;

	typedef typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator	Allocator;

public:

	struct Types {
		typedef Profile_						Profile;
		typedef MyType::Allocator				Allocator;

		typedef VectorMapCtrTypes<Types>		CtrTypes;
		typedef VectorMapIterTypes<Types>		IterTypes;
	};

	typedef typename Types::CtrTypes 											CtrTypes;
	typedef typename Types::IterTypes 											IterTypes;

	typedef Ctr<CtrTypes>														Type;
};


}

#endif
