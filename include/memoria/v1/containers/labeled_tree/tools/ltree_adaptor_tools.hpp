
// Copyright 2013 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/tools/isequencedata.hpp>
#include <memoria/v1/core/types.hpp>


namespace memoria {
namespace v1 {
namespace louds     {


template <typename T>
class LoudsSourceAdaptorBase: public ISequenceDataSource<T, 1> {
protected:
    SizeT       start_;
    SizeT       length_;

public:

    LoudsSourceAdaptorBase(int64_t length):
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

    NodeDegreeSourceAdaptor(int64_t degree):
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

    ZeroSourceAdaptor(int64_t length):
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
}}