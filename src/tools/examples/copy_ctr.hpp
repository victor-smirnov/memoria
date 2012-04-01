// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_EXAMPLES_COPY_CTR_HPP_
#define MEMORIA_EXAMPLES_COPY_CTR_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/examples.hpp>
#include <memoria/tools/tools.hpp>

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

struct CopyCtrParams: public ExampleTaskParams {

	CopyCtrParams(): ExampleTaskParams("CopyCtr") {}
};


class CopyCtrExample: public SPExampleTask {
public:
	typedef KVPair<BigInt, BigInt> Pair;

private:
	typedef vector<Pair> PairVector;
	typedef SmallCtrTypeFactory::Factory<VectorMap>::Type 			MapCtr;
	typedef typename MapCtr::Iterator								Iterator;
	typedef typename MapCtr::ID										ID;


	PairVector pairs;
	PairVector pairs_sorted;

public:

	CopyCtrExample() :
		SPExampleTask(new CopyCtrParams())
	{
		MapCtr::Init();
	}

	virtual ~CopyCtrExample() throw () {
	}

	MapCtr CreateCtr(Allocator& allocator, BigInt name)
	{
		return MapCtr(allocator, name, true);
	}

	MapCtr CreateCtr1(Allocator& allocator, BigInt name)
	{
		MapCtr map = CreateCtr(allocator, name);

		map[123456] = 10;

		return map;
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


			MapCtr map(CreateCtr1(allocator, 1));

			cout<<"Map has been created"<<endl;

			MapCtr map2 = map;

			cout<<"Map2 has been created as a copy of Map"<<endl;

			cout<<"About to reinitialize Map2"<<endl;

			map2 = CreateCtr(allocator, 2);

			cout<<"Map2 has been reinitialized"<<endl;

			for (Int c = 1; c <= 10; c++)
			{
				map[c] = c;
			}

			allocator.commit();

		}

		cout<<endl<<"Done. CtrCounters="<<CtrRefCounters<<" "<<CtrUnrefCounters<<endl<<endl;
	}
};

}

#endif
