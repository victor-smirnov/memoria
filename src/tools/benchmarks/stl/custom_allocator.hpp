
// Copyright 2012 Victor Smirnov
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


#include <memory>


namespace memoria {
namespace v1 {

using namespace std;

template<typename T = void>
class AllocatorBase {
protected:
    static BigInt count_;
public:
    static BigInt count() {
        return count_;
    }

    static void reset() {
        count_ = 0;
    }
};

template <typename T> BigInt AllocatorBase<T>::count_ = 0;

template <typename T>
class CustomAllocator: public AllocatorBase<> {
    typedef AllocatorBase<> Base;

public:

    typedef T        value_type;

    typedef T*       pointer;
    typedef const T* const_pointer;

    typedef T&       reference;
    typedef const T& const_reference;

    typedef std::size_t    size_type;
    typedef std::ptrdiff_t difference_type;


    template <class U>
    struct rebind {
        typedef CustomAllocator<U> other;
    };

    const_pointer address (const_reference value) const {
        return &value;
    }

    pointer address (reference value) const {
        return &value;
    }


    CustomAllocator() throw()                           {}
    CustomAllocator(const CustomAllocator&) throw()     {}
    template <class U>
    CustomAllocator (const CustomAllocator<U>&) throw() {}
    ~CustomAllocator() throw()                          {}

    size_type max_size () const throw()
    {
        return std::numeric_limits<std::size_t>::max() / sizeof(T);
    }


    pointer allocate (size_type num, const void* = 0)
    {
        Base::count_ += num*sizeof(T);
        pointer ret = (pointer)(::operator new(num*sizeof(T)));
        return ret;
    }

    void deallocate (pointer p, size_type num)
    {
        Base::count_ -= num*sizeof(T);
        ::operator delete((void*)p);
    }

    void construct (pointer p, const T& value)
    {
        new((void*)p)T(value);
    }

    void destroy (pointer p)
    {
        p->~T();
    }
};



template <class T1, class T2>
bool operator!=(const CustomAllocator<T1>&, const CustomAllocator<T2>&) throw()
{
    return false;
}

template <class T1, class T2>
bool operator==(const CustomAllocator<T1>&, const CustomAllocator<T2>&) throw()
{
    return true;
}

}}