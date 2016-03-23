
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/tools/isequencedata.hpp>
#include <memoria/v1/core/types/types.hpp>


namespace memoria   {
namespace louds     {


template <typename T>
class LoudsSourceAdaptorBase: public ISequenceDataSource<T, 1> {
protected:
    SizeT       start_;
    SizeT       length_;

public:

    LoudsSourceAdaptorBase(BigInt length):
        start_(0),
        length_(length)
    {}



    virtual ~LoudsSourceAdaptorBase() throw () {}

    virtual SizeT skip(SizeT length)
    {
        if (start_ + length <= length_)
        {
            start_ += length;
            return length;
        }

        SizeT distance = length_ - start_;
        start_ = length_;
        return distance;
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

    SizeT size() const {
        return getSize();
    }

    virtual void reset()
    {
        start_ = 0;
    }
};





template <typename T>
class NodeDegreeSourceAdaptor: public LoudsSourceAdaptorBase<T> {

    typedef LoudsSourceAdaptorBase<T> Base;

public:

    NodeDegreeSourceAdaptor(BigInt degree):
        Base(degree + 1)
    {}

    virtual ~NodeDegreeSourceAdaptor() throw () {}

    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
        FillOne(buffer, start, start + length);

        SizeT result = Base::skip(length);

        if (Base::getRemainder() == 0)
        {
            SetBit(buffer, start + length - 1, 0);
        }

        return result;
    }
};


template <typename T>
class ZeroSourceAdaptor: public LoudsSourceAdaptorBase<T> {

    typedef LoudsSourceAdaptorBase<T> Base;

public:

    ZeroSourceAdaptor(BigInt length):
        Base(length)
    {}

    virtual ~ZeroSourceAdaptor() throw () {}

    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
        FillZero(buffer, start, start + length);
        return Base::skip(length);
    }
};



}
}
