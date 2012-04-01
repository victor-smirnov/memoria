// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_EXAMPLES_MAP_HPP_
#define MEMORIA_EXAMPLES_MAP_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/examples.hpp>
#include <memoria/tools/tools.hpp>

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

struct MapParams: public ExampleTaskParams {

	MapParams(): ExampleTaskParams("Map") {}
};


class MapExample: public SPExampleTask {
public:
	typedef KVPair<BigInt, BigInt> Pair;

private:
	typedef vector<Pair> PairVector;
	typedef SmallCtrTypeFactory::Factory<Map1>::Type 				MapCtr;
	typedef typename MapCtr::Iterator								Iterator;
	typedef typename MapCtr::ID										ID;


	PairVector pairs;
	PairVector pairs_sorted;

public:

	MapExample() :
		SPExampleTask(new MapParams())
	{
		MapCtr::Init();
	}

	virtual ~MapExample() throw () {
	}


	virtual void Run(ostream& out)
	{
		DefaultLogHandlerImpl logHandler(out);

		CreateCtrParams* task_params = GetParameters<CreateCtrParams>();
		{
			if (task_params->btree_random_airity_)
			{
				task_params->btree_airity_ = 8 + GetRandom(100);
				out<<"BTree Airity: "<<task_params->btree_airity_<<endl;
			}

			Allocator allocator;
			allocator.GetLogger()->SetHandler(&logHandler);

			MapCtr map(allocator, 1, true);


			for (Int c = 0; c < 10; c++)
			{
				map[c] = c + 1;
			}

			allocator.commit();

			for (auto iter = map.begin(); iter != map.endm(); iter++)
			{
				out<<iter.key()<<" => "<<iter.value()<<endl;
			}


			auto iter1 = map.begin();
			auto iter2 = iter1;

			if (iter1 == iter2)
			{
				out<<"Iterators are equal"<<endl;
			}

			for (auto& val: map)
			{
				out<<val.key()<<" => "<<(BigInt)val<<endl;
			}

			BigInt sum = 0;
			for (auto& val: map)
			{
				sum += val;
			}

			out<<"Sum="<<sum<<endl;
		}
	}
};

}

#endif
