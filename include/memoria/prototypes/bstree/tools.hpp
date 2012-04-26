
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BSTREE_TOOLS_HPP
#define	_MEMORIA_PROTOTYPES_BSTREE_TOOLS_HPP

#include <memoria/prototypes/btree/tools.hpp>
#include <memoria/core/tools/fixed_vector.hpp>

#include <ostream>

namespace memoria    	{
namespace btree 		{

using namespace memoria::core;


template <typename Key, Int Indexes_>
class Accumulators
{
	typedef Accumulators<Key, Indexes_> MyType;

	Key 		keys_[Indexes_];

public:

	static const Int Indexes = Indexes_;

	Accumulators()
	{
		for (Int c = 0; c < Indexes; c++)
		{
			keys_[c] = 0;
		}
	}

	Accumulators(const MyType& other)
	{
		for (Int c = 0; c < Indexes; c++)
		{
			keys_[c] = other.keys_[c];
		}
	}

	Accumulators(const Key* keys)
	{
		for (Int c = 0; c < Indexes; c++)
		{
			keys_[c] = keys[c];
		}
	}

	const Key* keys() const
	{
		return keys_;
	}

	Key* keys()
	{
		return keys_;
	}



	const Key& key(Int idx) const
	{
		return keys_[idx];
	}

	Key& key(Int idx)
	{
		return keys_[idx];
	}

	const Key& operator[](Int idx) const
	{
		return keys_[idx];
	}

	Key& operator[](Int idx)
	{
		return keys_[idx];
	}

	void Clear()
	{
		for (Int c = 0; c < Indexes; c++)
		{
			keys_[c] = 0;
		}
	}

	bool operator==(const MyType& other) const
	{
		for (Int c = 0; c < Indexes; c++)
		{
			if (keys_[c] != other.keys_[c])
			{
				return false;
			}
		}

		return true;
	}

	bool operator!=(const MyType& other) const
	{
		for (Int c = 0; c < Indexes; c++)
		{
			if (keys_[c] == other.keys_[c])
			{
				return false;
			}
		}

		return true;
	}

	MyType& operator=(const MyType& other)
	{
		for (Int c = 0; c < Indexes; c++)
		{
			keys_[c] = other.keys_[c];
		}

		return *this;
	}

	MyType& operator=(const Key* keys)
	{
		for (Int c = 0; c < Indexes; c++)
		{
			keys_[c] = keys[c];
		}

		return *this;
	}

	MyType& operator+=(const MyType& other)
	{
		for (Int c = 0; c < Indexes; c++)
		{
			keys_[c] += other.keys_[c];
		}

		return *this;
	}

	MyType operator+(const MyType& other) const
	{
		MyType result = *this;

		for (Int c = 0; c < Indexes; c++)
		{
			result.keys_[c] += other.keys_[c];
		}

		return result;
	}

	MyType operator-(const MyType& other) const
	{
		MyType result = *this;

		for (Int c = 0; c < Indexes; c++)
		{
			result.keys_[c] -= other.keys_[c];
		}

		return result;
	}

	MyType operator-() const
	{
		MyType result = *this;

		for (Int c = 0; c < Indexes; c++)
		{
			result.keys_[c] = -keys_[c];
		}

		return result;
	}


	MyType& operator-=(const MyType& other)
	{
		for (Int c = 0; c < Indexes; c++)
		{
			keys_[c] -= other.keys_[c];
		}

		return *this;
	}
};



template <typename Iterator, typename Container>
class BTreeIteratorScalarPrefixCache: public BTreeIteratorCache<Iterator, Container> {
	typedef BTreeIteratorCache<Iterator, Container> Base;
	typedef typename Container::Accumulator 	Accumulator;

	BigInt prefix_;
	BigInt current_;


public:

	BTreeIteratorScalarPrefixCache(): Base(), prefix_(0), current_(0) {}

	const BigInt& prefix(int num = 0) const
	{
		return prefix_;
	}

	Accumulator prefixes() const
	{
		Accumulator a;
		a[0] = prefix_;
		return a;
	}



	void NextKey(bool end)
	{
		prefix_ 	+= current_;
		current_	= 0;
	};

	void PrevKey(bool start)
	{
		prefix_ 	-= current_;
		current_	= 0;
	};

	void Prepare()
	{
		current_ = Base::iterator().GetRawKey(0);
	}

	void Setup(BigInt prefix, Int key_num)
	{
		prefix_ = prefix;
	}

	void InitState()
	{
		typedef typename Iterator::Container::TreePath TreePath;

		BigInt accum = 0;

		const TreePath& path = Base::iterator().path();
		Int 			idx  = Base::iterator().key_idx();

		Int block_num = 0;

		for (Int c = 0; c < path.GetSize(); c++)
		{
			Base::iterator().model().SumKeys(path[c].node(), block_num, 0, idx, accum);
			idx = path[c].parent_idx();
		}

		prefix_ = accum;
	}
};

template <typename Iterator, typename Container>
class BTreeIteratorPrefixCache: public BTreeIteratorCache<Iterator, Container> {
	typedef BTreeIteratorCache<Iterator, Container> 				Base;
	typedef typename Container::Accumulator 	Accumulator;

	Accumulator prefix_;

public:

	BTreeIteratorPrefixCache(): Base(), prefix_() {}

	BigInt& prefix(Int num = 0)
	{
		return prefix_[num];
	}

	const BigInt& prefix(Int num = 0) const
	{
		return prefix_[num];
	}

	BigInt& prefixes()
	{
		return prefix_;
	}

	const BigInt& prefixes() const
	{
		return prefix_;
	}

	void Setup(BigInt prefix, Int key_num)
	{

	}
};




}
}

namespace std {

using namespace memoria::btree;

template <typename Key, Int Indexes>
ostream& operator<<(ostream& out, const Accumulators<Key, Indexes>& accum)
{
	out<<"[";

	for (Int c = 0; c < Indexes; c++)
	{
		out<<accum.keys()[c];

		if (c < Indexes - 1)
		{
			out<<", ";
		}
	}

	out<<"]";

	return out;
}

}


#endif

