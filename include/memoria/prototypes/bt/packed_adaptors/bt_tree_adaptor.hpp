
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_TREE_ADAPTOR_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_TREE_ADAPTOR_HPP

#include <memoria/prototypes/bt/bt_tools.hpp>

namespace memoria   {
namespace bt1       {

struct DefaultIteratorPrefixFn {
    template <typename StreamType, typename IteratorPrefix>
    void processNonLeafFw(const StreamType* stream, IteratorPrefix& i_prefix, Int start, Int end, Int index, BigInt prefix)
    {
        if (index >= 0 && index < IteratorPrefix::Indexes)
        {
            for (Int c = 0; c < index; c++)
            {
                i_prefix[c] += stream->sum(c, start, end);
            }

            i_prefix[index] += prefix;

            for (Int c = index + 1; c < IteratorPrefix::Indexes; c++)
            {
                i_prefix[c] += stream->sum(c, start, end);
            }
        }
        else {
            for (Int c = 0; c < IteratorPrefix::Indexes; c++)
            {
                i_prefix[c] += stream->sum(c, start, end);
            }
        }
    }




    template <typename StreamType, typename IteratorPrefix>
    void processLeafFw(const StreamType* stream, IteratorPrefix& i_prefix, Int start, Int end, Int index, BigInt prefix)
    {
        for (Int c = 1; c < IteratorPrefix::Indexes; c++)
        {
            if (c != index)
            {
                i_prefix[c] += stream->sum(c - 1, start, end);
            }
        }

        i_prefix[index] += prefix;

        if (index > 0)
        {
            i_prefix[0] += end - start;
        }
    }

    template <typename StreamType, typename IteratorPrefix>
    void processLeafFw(const StreamType* stream, IteratorPrefix& i_prefix, Int start, Int end)
    {
        i_prefix[0] += end - start;

        for (Int c = 1; c < IteratorPrefix::Indexes; c++)
        {
            i_prefix[c] += stream->sum(c - 1, start, end);
        }
    }



    template <typename StreamType, typename IteratorPrefix>
    void processNonLeafBw(const StreamType* stream, IteratorPrefix& i_prefix, Int start, Int end, Int index, BigInt prefix)
    {
        if (index >= 0 && index < IteratorPrefix::Indexes)
        {
            for (Int c = 0; c < index; c++)
            {
                i_prefix[c] -= stream->sum(c, start, end);
            }

            i_prefix[index] -= prefix;

            for (Int c = index + 1; c < IteratorPrefix::Indexes; c++)
            {
                i_prefix[c] -= stream->sum(c, start, end);
            }
        }
        else {
            for (Int c = 0; c < IteratorPrefix::Indexes; c++)
            {
                i_prefix[c] -= stream->sum(c, start, end);
            }
        }
    }

    template <typename StreamType, typename IteratorPrefix>
    void processLeafBw(const StreamType* stream, IteratorPrefix& i_prefix, Int start, Int end, Int index, BigInt prefix)
    {
//      for (Int c = 1; c < IteratorPrefix::Indexes; c++)
//      {
//          if (c != index)
//          {
//              i_prefix[c] -= stream->sum(c - 1, start, end);
//          }
//      }
//
//      i_prefix[index] -= prefix;
//
//      if (index > 0)
//      {
//          i_prefix[0] -= end - start;
//      }

        for (Int c = 1; c < IteratorPrefix::Indexes; c++)
        {
            i_prefix[c] -= stream->sum(c - 1, start, end);
        }

        i_prefix[0] -= end - start;
    }



    template <typename StreamType, typename IteratorPrefix>
    void processLeafBw(const StreamType* stream, IteratorPrefix& i_prefix, Int start, Int end)
    {
        i_prefix[0] -= end - start;

        for (Int c = 1; c < IteratorPrefix::Indexes; c++)
        {
            i_prefix[c] -= stream->sum(c - 1, start, end);
        }
    }

};


}
}

#endif
