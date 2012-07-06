// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_EXAMPLES_VECTOR_MAP_HPP_
#define MEMORIA_EXAMPLES_VECTOR_MAP_HPP_

#include "examples.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {



class VectorMapExample: public SPExampleTask {

private:
	typedef SmallCtrTypeFactory::Factory<VectorMap>::Type 				MapCtr;
	typedef typename MapCtr::Iterator									Iterator;
	typedef typename MapCtr::ID											ID;

public:

	VectorMapExample() :
		SPExampleTask("VectorMap")
	{
		MapCtr::initMetadata();
	}

	virtual ~VectorMapExample() throw () {
	}


	virtual void Run(ostream& out)
	{
		DefaultLogHandlerImpl logHandler(out);

		{
			if (this->btree_random_airity_)
			{
				this->btree_branching_ = 8 + getRandom(100);
				out<<"BTree Branching: "<<this->btree_branching_<<endl;
			}

			Allocator allocator;
			allocator.getLogger()->setHandler(&logHandler);

			MapCtr map(allocator, 1, true);

			for (Int c = 1; c <= 10; c++)
			{
				map[c] = String(" ====== "+toString(c + 1)+" ===== ");
			}

			allocator.commit();

			for (auto iter = map.begin(); iter != map.endm(); iter++)
			{
				out<<iter.key()<<" => "<<(String)iter<<endl;
			}


			auto iter1 = map.begin();
			auto iter2 = iter1;

			iter2++;

			if (iter1 == iter2)
			{
				out<<"Iterators are equal"<<endl;
			}
			else {
				out<<"Iterators are NOT equal"<<endl;
			}

			for (auto& val: map)
			{
				out<<val.key()<<" => "<<(String)val<<endl;
			}

			BigInt sum = 0;
			for (auto& val: map)
			{
				sum += val.key();
			}

			out<<"Sum="<<sum<<endl;
		}
	}
};

}

#endif
