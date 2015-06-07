
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_WALKER_TOOLS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_WALKER_TOOLS_HPP

#include <memoria/core/types/types.hpp>

namespace memoria {
namespace bt1     {




struct EmptyIteratorPrefixFn {
    template <typename StreamType, typename IteratorPrefix>
    void processNonLeafFw(const StreamType*, IteratorPrefix&, Int start, Int end, Int index, BigInt prefix)
    {}

    template <typename StreamType, typename IteratorPrefix>
    void processLeafFw(const StreamType*, IteratorPrefix&, Int start, Int end, Int index, BigInt prefix)
    {}

    template <typename StreamType, typename IteratorPrefix>
    void processLeafFw(const StreamType*, IteratorPrefix&, Int start, Int end)
    {}



    template <typename StreamType, typename IteratorPrefix>
    void processNonLeafBw(const StreamType*, IteratorPrefix&, Int start, Int end, Int index, BigInt prefix)
    {}

    template <typename StreamType, typename IteratorPrefix>
    void processLeafBw(const StreamType*, IteratorPrefix&, Int start, Int end, Int index, BigInt prefix)
    {}

    template <typename StreamType, typename IteratorPrefix>
    void processLeafBw(const StreamType*, IteratorPrefix&, Int start, Int end)
    {}
};


}
}

#endif
