
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_TL_TOOLS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_TL_TOOLS_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/core/container/container.hpp>



namespace memoria {
namespace bttl    {




template <typename Iterator, typename Container>
class BTTLIteratorPrefixCache: public bt::BTree2IteratorPrefixCache<Iterator, Container> {

	using Base 		= bt::BTree2IteratorPrefixCache<Iterator, Container>;
	using Position 	= typename Container::Types::Position;

	Position data_size_;
	Position data_pos_;

public:
	using MyType = BTTLIteratorPrefixCache<Iterator, Container>;

	Position& data_size() {
		return data_size_;
	}

	const Position& data_size() const {
		return data_size_;
	}

	Position& data_pos() {
		return data_pos_;
	}

	const Position& data_pos() const {
		return data_pos_;
	}

    bool operator==(const MyType& other) const
    {
    	return data_size_ == other.data_size_ && Base::operator==(other);
    }

    bool operator!=(const MyType& other) const
	{
    	return data_size_ != other.data_size_ || Base::operator!=(other);
    }
};



template <
    typename I, typename C
>
std::ostream& operator<<(std::ostream& out, const BTTLIteratorPrefixCache<I, C>& cache)
{
    out<<"BTTLIteratorPrefixCache[";
    out<<"Branch prefixes: "<<cache.prefixes()<<", Leaf Prefixes: "<<cache.leaf_prefixes();
//    out<<", Size Prefixes: "<<cache.size_prefix();
//    out<<", Data Size: "<<cache.data_size();
//    out<<", Data Pos: "<<cache.data_pos();
    out<<"]";

    return out;
}



}
}

#endif
