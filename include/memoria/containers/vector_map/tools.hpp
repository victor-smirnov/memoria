
// Copyright Victor Smirnov, Ivan Yurchenko 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_VECTOR_MAP_TOOLS_HPP
#define _MEMORIA_CONTAINERS_VECTOR_MAP_TOOLS_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/containers/vector_map/names.hpp>
#include <memoria/prototypes/sequence/tools.hpp>

#include <memoria/core/types/selector.hpp>

#include <ostream>

namespace memoria {

using namespace std;


template <typename IDataWrapper>
class VectorMapDataWrapper {
    IDataWrapper wrapper_;

public:

    VectorMapDataWrapper(const IDataWrapper& wrapper): wrapper_(wrapper) {}
    VectorMapDataWrapper(IDataWrapper&& wrapper): wrapper_(wrapper) {}
    VectorMapDataWrapper(VectorMapDataWrapper<IDataWrapper>&& other): wrapper_(std::move(other.wrapper_)) {}

    VectorMapDataWrapper(const VectorMapDataWrapper<IDataWrapper>& other): wrapper_(other.wrapper_) {}

    IDataWrapper& wrapper() {
        return wrapper_;
    }
};

template <typename Types, typename = IsVectorMap<Types, Any, Byte>>
void AssignToItem(Iter<VectorMapIterTypes<Types>>& iter, const char* str)
{
    MemBuffer<const Byte> data(str, str != nullptr ? strlen(str) : 0);
    iter.setValue(data);
}

template <typename Types, typename = IsVectorMap<Types, Any, Byte>>
void AssignToItem(Iter<VectorMapIterTypes<Types>>& iter, StringRef str)
{
    MemBuffer<const Byte> data(str.c_str(), str.size());
    iter.setValue(data);
}

template <typename Types>
void AssignToItem(Iter<VectorMapIterTypes<Types>>& iter, const vector<typename Types::Value>& value)
{
    iter.setValue(value);
}

template <typename Types, typename Wrapper>
void AssignToItem(Iter<VectorMapIterTypes<Types>>& iter, VectorMapDataWrapper<Wrapper>&& wrapper)
{
    iter.update(wrapper.wrapper());
    wrapper.wrapper().reset();
}

template <typename Types, typename = IsVectorMap<Types, Any, Byte>>
ostream& operator<<(ostream& out, Iter<VectorMapIterTypes<Types>>& iter)
{
    OStreamDataWrapper<Byte> wrapper(out, iter.size());
    iter.read(wrapper);
    return out;
}

template <typename Types, typename = IsVectorMap<Types, Any, Byte>>
Iter<VectorMapIterTypes<Types>>& operator<<(Iter<VectorMapIterTypes<Types>>& iter, const char* str)
{
    if (str)
    {
        MemBuffer<const Byte> data(str, str != nullptr ? strlen(str) : 0);
        iter.insert(data);
    }
    return iter;
}


template <typename Types, typename = IsVectorMap<Types, Any, Byte>>
Iter<VectorMapIterTypes<Types>>& operator<<(Iter<VectorMapIterTypes<Types>>& iter, StringRef str)
{
    if (str.size() > 0)
    {
        MemBuffer<const Byte> data(str.c_str(), str.size());
        iter.insert(data);
    }
    return iter;
}

template <typename Types, typename = IsVectorMap<Types, Any, Byte>>
Iter<VectorMapIterTypes<Types>>& operator<<(Iter<VectorMapIterTypes<Types>>& iter, const SBuf& buf)
{
    String str = buf.str();

    if (str.size() > 0)
    {
        MemBuffer<const Byte> data(str.c_str(), str.size());
        iter.insert(data);
    }
    return iter;
}


}



#endif
