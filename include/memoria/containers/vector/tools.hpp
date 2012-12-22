
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ARRAY_TOOLS_HPP
#define _MEMORIA_MODELS_ARRAY_TOOLS_HPP

#include <memoria/containers/vector/names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/tools/idata.hpp>

namespace memoria    {


template <typename Iterator>
class IDataAdapter: public IData<typename Iterator::ElementType> {
    typedef typename Iterator::ElementType T;

    Iterator    iter_;
    BigInt      length_;

    BigInt      start_  = 0;

public:
    IDataAdapter(IDataAdapter<Iterator>&& other):
        iter_(std::move(other.iter_)), length_ (other.length_), start_(other.start_) {}

    IDataAdapter(const IDataAdapter<Iterator>& other):
            iter_(other.iter_), length_ (other.length_), start_(other.start_) {}

    IDataAdapter(const Iterator& iter, BigInt length): iter_(iter)
    {
        BigInt pos  = iter_.pos();
        BigInt size = iter_.model().size();

        if (length != -1 && (pos + length <= size))
        {
            length_ = length;
        }
        else {
            length_ = size - pos;
        }
    }

    virtual SizeT skip(SizeT length)
    {
        BigInt delta = iter_.skip(length);
        start_ += delta;
        return delta;
    }

    virtual SizeT getStart() const
    {
        return start_;
    }

    virtual SizeT getRemainder() const
    {
        return length_ - start_;
    }

    virtual SizeT getSize() const
    {
        return length_;
    }

    virtual SizeT put(const T* buffer, SizeT length)
    {
        return 0;
    }

    virtual SizeT get(T* buffer, SizeT length) const
    {
        auto& data  = iter_.data();
        Int pos     = iter_.dataPos();
        BigInt size = data->getMaxCapacity() - pos;

        if (length > size)
        {
            length = size;
        }

        CopyBuffer(data->addr(pos), buffer, length);

        return length;
    }

    virtual void reset()
    {
        iter_.skip(-start_);
        start_ = 0;
    }
};


template <typename Types, typename T>
Ctr<VectorCtrTypes<Types>>& operator<<(Ctr<VectorCtrTypes<Types>>& ctr, const T& value)
{
    auto iter = ctr.End();
    VariableRef<const T> ref(value);
    iter.insert(ref);
    return ctr;
}

template <typename Types>
ostream& operator<<(ostream& out, const Iter<VectorIterTypes<Types>>& iter)
{
    out<<iter.element();
    return out;
}



template <typename Types>
void UpdateVector(Iter<VectorIterTypes<Types>>& iter, const std::vector<typename Types::ElementType>& source)
{
    typedef Iter<VectorIterTypes<Types>>    IterType;
    typedef typename Types::ElementType     ElementType;

    IterType tmp = iter;

    const MemBuffer<const ElementType> src(&source[0], source.size());

    tmp.update(src);
}


}

#endif
