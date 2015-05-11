
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_PROTOTYPES_BT_LAYOUT_SINGLE_HPP_
#define MEMORIA_PROTOTYPES_BT_LAYOUT_SINGLE_HPP_

#include <memoria/core/types/types.hpp>

namespace memoria 	{
namespace bt 		{

template <typename Ctr, typename Types>
struct SingleStreamLayout {
	using NodeBaseG 	= typename Types::NodeBaseG;
	using InputBuffer	= typename Types::InputBuffer;

	static Int insert(Ctr& self, NodeBaseG& node, Int idx, const InputBuffer* data)
	{
		Int capacity = self.getStreamCapacity(node, 0);

		if (capacity >= data->size())
		{

		}
	}
};

}
}






#endif
