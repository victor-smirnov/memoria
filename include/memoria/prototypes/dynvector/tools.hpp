
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_TOOLS_HPP
#define _MEMORIA_PROTOTYPES_DYNVECTOR_TOOLS_HPP

#include <memoria/prototypes/btree/tools.hpp>

namespace memoria       {
namespace btree         {

using namespace memoria::core;

template <typename NodePage, typename DataPage, Int Size = 16>
class DataPath: public NodePath<NodePage, Size> {

    typedef NodePath<NodePage, Size>                                    Base;
    typedef DataPath<NodePage, DataPage, Size>                          MyType;

public:

    typedef TreePathItem<DataPage>                                      DataItem;
private:

    DataItem data_;

public:
    DataPath(): Base() {}

    DataPath(const MyType& other): Base(other)
    {
        data_ = other.data_;
    }

    DataPath(MyType&& other): Base(std::move(other))
    {
        data_ = std::move(other.data_);
    }

    MyType& operator=(const MyType& other)
    {
        Base::operator=(other);

        data_ = other.data_;

        return *this;
    }

    MyType& operator=(MyType&& other)
    {
        Base::operator=(std::move(other));

        data_ = std::move(other.data_);

        return *this;
    }

    DataItem& data()
    {
        return data_;
    }

    const DataItem& data() const
    {
        return data_;
    }

    void moveRight(Int level, Int from, Int count)
    {
        if (level >= 0)
        {
            typename Base::PathItem& item = Base::operator[](level);
            if (item.parent_idx() >= from)
            {
                item.parent_idx() += count;
            }
        }
        else if (data_.parent_idx() >= from)
        {
            data_.parent_idx() += count;
        }
    }

    void moveLeft(Int level, Int from, Int count)
    {
        if (level >= 0)
        {
            typename Base::PathItem& item = Base::operator[](level);

            if (item.parent_idx() >= from)
            {
                item.parent_idx() -= count;
            }
            else if (item.parent_idx() >= from)
            {
                for (Int c = level - 1; c >= 0; c--)
                {
                    Base::operator[](c).clear();
                }

                data_.clear();
            }
        }
        else if (data_.parent_idx() >= from)
        {
            data_.parent_idx() -= count;
        }
        else if (data_.parent_idx() >= from)
        {
            data_.clear();
        }
    }
};

}
}


#endif
