
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQ_DENSE_FACTORY_HPP
#define _MEMORIA_CONTAINERS_SEQ_DENSE_FACTORY_HPP

#include <memoria/containers/vector/factory.hpp>

#include <memoria/containers/seq_dense/names.hpp>

#include <memoria/containers/seq_dense/container/bvdense_c_insert.hpp>

namespace memoria {

template <typename Profile>
struct BTreeTypes<Profile, memoria::BitVector<true>>: public BTreeTypes<Profile, memoria::Vector<BigInt>> {

    typedef BTreeTypes<Profile, memoria::Vector<BigInt>>              			Base;

    static const Int Indexes                                                    = 2;

    typedef typename AppendTool<
        		typename Base::ContainerPartsList,
        		TypeList<
        			memoria::seq_dense::CtrInsertName
        		>
    >::Result                                                                   ContainerPartsList;


};



template <typename Profile, typename T>
class CtrTF<Profile, memoria::BitVector<true>, T>: public CtrTF<Profile, memoria::Vector<BigInt>, T> {

	typedef CtrTF<Profile, memoria::Vector<BigInt>, T> 							Base;

public:

	struct Types: Base::Types
	{
		typedef SequenceCtrTypes<Types>                    	CtrTypes;
		typedef SequenceIterTypes<Types>                   	IterTypes;
	};

	typedef typename Types::CtrTypes                                            CtrTypes;

	typedef Ctr<CtrTypes>                                                       Type;
};

}

#endif
