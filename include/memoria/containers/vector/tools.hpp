
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ARRAY_TOOLS_HPP
#define _MEMORIA_MODELS_ARRAY_TOOLS_HPP

namespace memoria    {


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
