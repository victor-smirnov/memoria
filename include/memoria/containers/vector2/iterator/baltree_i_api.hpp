
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINER_VECTOR2_ITERATOR_API2_HPP
#define _MEMORIA_CONTAINER_VECTOR2_ITERATOR_API2_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>

#include <memoria/containers/vector2/vector_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria {




MEMORIA_ITERATOR_PART_BEGIN(memoria::mvector2::ItrBaltreeApiName)

	typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::TreePath                                             TreePath;

    typedef typename Base::Container::Value                                     Value;
    typedef typename Base::Container::Key                                       Key;
    typedef typename Base::Container::Element                                   Element;
    typedef typename Base::Container::Accumulator                               Accumulator;
    typedef typename Base::Container                                            Container;

    bool operator++() {
    	return self().nextKey();
    }

    bool operator--() {
    	return self().prevKey();
    }

    bool operator++(int) {
    	return self().nextKey();
    }

    bool operator--(int) {
    	return self().prevKey();
    }

    BigInt operator+=(BigInt size)
	{
    	return self().skipFw(size);
    }

    BigInt operator-=(BigInt size)
    {
    	return self().skipBw(size);
    }


    bool nextKey();
    bool prevKey();

    bool hasNextKey();
    bool hasPrevKey();


MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::mvector2::ItrBaltreeApiName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS


M_PARAMS
bool M_TYPE::nextKey()
{
    auto& self = this->self();

	if (!self.isEnd())
    {
        if (self.key_idx() < self.page()->children_count() - 1)
        {
            self.cache().Prepare();

            self.key_idx()++;

            self.keyNum()++;

            self.cache().nextKey(false);

            return true;
        }
        else {
            self.cache().Prepare();

            bool has_next_leaf = self.nextLeaf();
            if (has_next_leaf)
            {
                self.key_idx() = 0;

                self.cache().nextKey(false);
            }
            else {
                self.key_idx() = self.page()->children_count();

                self.cache().nextKey(true);
            }

            self.keyNum()++;

            return has_next_leaf;
        }
    }
    else {
        return false;
    }
}

M_PARAMS
bool M_TYPE::hasNextKey()
{
	auto& self = this->self();

	if (!self.isEnd())
    {
        if (self.key_idx() < self.page()->children_count() - 1)
        {
            return true;
        }
        else {
            return self.hasNextLeaf();
        }
    }
    else {
        return false;
    }
}



M_PARAMS
bool M_TYPE::prevKey()
{
	auto& self = this->self();

    if (self.key_idx() > 0)
    {
        self.key_idx()--;
        self.keyNum()--;

        self.cache().Prepare();
        self.cache().prevKey(false);

        return true;
    }
    else {
        bool has_prev_leaf = self.prevLeaf();

        if (has_prev_leaf)
        {
            self.key_idx() = self.page()->children_count() - 1;
            self.keyNum()--;

            self.cache().Prepare();
            self.cache().prevKey(false);
        }
        else {
            self.key_idx() = -1;

            self.model().finishPathStep(self.path(), self.key_idx());

            self.cache().Prepare();
            self.cache().prevKey(true);
        }

        return has_prev_leaf;
    }
}


M_PARAMS
bool M_TYPE::hasPrevKey()
{
	auto& self = this->self();

    if (self.key_idx() > 0)
    {
        return true;
    }
    else {
        return self.hasPrevLeaf();
    }
}


}

#undef M_TYPE
#undef M_PARAMS




#endif
