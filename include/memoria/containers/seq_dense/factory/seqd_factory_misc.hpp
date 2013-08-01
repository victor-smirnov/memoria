
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_FACTORY_FACTORY_MISC_HPP
#define _MEMORIA_CONTAINERS_SEQD_FACTORY_FACTORY_MISC_HPP



#include <memoria/core/packed/packed_fse_searchable_seq.hpp>
#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/core/packed/tree/packed_vle_tree.hpp>


namespace memoria {


template <typename Types, Int StreamIdx>
struct PackedFSESeqTF {

	typedef typename Types::Value												Value;
	typedef typename Types::Key                                                 Key;

	typedef typename SelectByIndexTool<
			StreamIdx,
			typename Types::StreamDescriptors
	>::Result																	Descriptor;

	static const Int BitsPerSymbol = Types::BitsPerSymbol;

	typedef typename PkdFSSeqTF<BitsPerSymbol>::Type				SequenceTypes;

	typedef PkdFSSeq<SequenceTypes> Type;
};



}

#endif
