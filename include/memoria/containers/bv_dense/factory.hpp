
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_BV_SPARSE_FACTORY_HPP
#define _MEMORIA_CONTAINERS_BV_SPARSE_FACTORY_HPP

#include <memoria/containers/vector/factory.hpp>

#include <memoria/containers/bv_dense/names.hpp>

#include <memoria/containers/bv_dense/container/bvdense_c_insert.hpp>

namespace memoria {

template <typename Profile>
struct BTreeTypes<Profile, memoria::BitVector<false>>: public BTreeTypes<Profile, memoria::Vector<BigInt>> {

    typedef BTreeTypes<Profile, memoria::Vector<BigInt>>              			Base;

    static const Int Indexes                                                    = 2;

    typedef typename AppendTool<
        		typename Base::ContainerPartsList,
        		TypeList<
        			memoria::bv_dense::CtrInsertName
        		>
    >::Result                                                                   ContainerPartsList;


};



template <typename Profile, typename T>
class CtrTF<Profile, memoria::BitVector<false>, T>: public CtrTF<Profile, memoria::Vector<BigInt>, T> {

	typedef CtrTF<Profile, memoria::Vector<BigInt>, T> 							Base;

public:

	struct Types: Base::Types
	{
		typedef BitVectorCtrTypes<Types>                    CtrTypes;
		typedef BitVectorIterTypes<Types>                   IterTypes;
	};

	typedef typename Types::CtrTypes                                            CtrTypes;

	typedef Ctr<CtrTypes>                                                       Type;
};

}

#endif
