
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_SS_FIND_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_SS_FIND_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::btss::FindName)

	using Types = TypesType;
	using NodeBaseG = typename Types::NodeBaseG;

	using Iterator = typename Base::Iterator;

	using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
	using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
	using BranchDispatcher = typename Types::Pages::BranchDispatcher;



	using BranchNodeEntry 	= typename Types::BranchNodeEntry;
	using Position		= typename Types::Position;

    using SplitFn = std::function<BranchNodeEntry (NodeBaseG&, NodeBaseG&)>;
    using MergeFn = std::function<void (const Position&)>;

    using CtrSizeT = typename Types::CtrSizeT;

    static const Int Streams                                                    = Types::Streams;

    Iterator seek(CtrSizeT position)
    {
    	return self().template seek_stream<0>(position);
    }

    Iterator Begin() {
    	return self().seek(0);
    }

    Iterator End()
    {
    	auto size = self().size();
    	if (size > 0)
    	{
    		return self().seek(size);
    	}
    	else {
    		return self().seek(0);
    	}
    }

    Iterator begin() {
    	return self().Begin();
    }

    Iterator end() {
    	return self().End();
    }

    IterEndMark endm()
    {
        return IterEndMark();
    }

MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::LeafCommonName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS





#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
