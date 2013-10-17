
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQ_DENSE_TOOLS_HPP
#define _MEMORIA_CONTAINERS_SEQ_DENSE_TOOLS_HPP

#include <memoria/core/tools/isymbols.hpp>

namespace memoria       {
namespace seq_dense     {



class SequenceSource: public ISource {

    IDataBase* source_;
public:
    SequenceSource(IDataBase* source): source_(source) {}

    virtual Int streams()
    {
        return 1;
    }

    virtual IData* stream(Int stream)
    {
        return source_;
    }

    virtual void newNode(INodeLayoutManager* layout_manager, BigInt* sizes)
    {
        Int allocated[1] = {0};
        Int capacity = layout_manager->getNodeCapacity(allocated, 0);

        sizes[0] = capacity;
    }

    virtual BigInt getTotalNodes(INodeLayoutManager* manager)
    {
        Int sizes[1] = {0};

        SizeT capacity  = manager->getNodeCapacity(sizes, 0);
        SizeT remainder = source_->getRemainder();

        return remainder / capacity + (remainder % capacity ? 1 : 0);
    }
};


class SequenceTarget: public ITarget {

    IDataBase* target_;
public:
    SequenceTarget(IDataBase* target): target_(target) {}

    virtual Int streams()
    {
        return 1;
    }

    virtual IData* stream(Int stream)
    {
        return target_;
    }
};



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
