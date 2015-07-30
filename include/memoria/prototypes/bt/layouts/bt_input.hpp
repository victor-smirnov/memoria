
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_PROTOTYPES_BT_INPUT_HPP_
#define MEMORIA_PROTOTYPES_BT_INPUT_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>

#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/packed/tools/packed_malloc.hpp>
#include <memoria/core/exceptions/memoria.hpp>

#include <cstdlib>
#include <tuple>

namespace memoria 	{
namespace bt 		{

template <typename Types>
struct AbstractInputProvider {
	using Position = typename Types::Position;
	using NodeBaseG = typename Types::NodeBaseG;

	virtual bool hasData() = 0;
	virtual Position fill(NodeBaseG& leaf, const Position& from)	= 0;
};

}
}

#endif
