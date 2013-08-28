
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQ_DENSE_TOOLS_HPP
#define _MEMORIA_CONTAINERS_SEQ_DENSE_TOOLS_HPP

namespace memoria       {
namespace seq_dense     {


template <typename Iterator, typename Container>
class SequenceIteratorCache: public bt::BTreeIteratorCache<Iterator, Container> {

    typedef bt::BTreeIteratorCache<Iterator, Container>                         Base;

    BigInt pos_ = 0;

public:

    SequenceIteratorCache(): Base() {}

    BigInt pos() const
    {
        return pos_;
    }

    void setup(BigInt pos)
    {
        pos_    = pos;
    }

    void add(BigInt pos)
    {
        pos_    += pos;
    }

    void sub(BigInt pos)
    {
        pos_    -= pos;
    }
};



}
}

#endif
