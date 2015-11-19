
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/core/packed/tree/packed_fse_quick_tree.hpp>
#include <memoria/core/packed/tree/packed_vle_quick_tree.hpp>
#include <memoria/core/packed/tree/packed_tree_tools.hpp>

#include <memoria/core/tools/i7_codec.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/random.hpp>

#include <iostream>

using namespace memoria;
using namespace std;


int main() {

	try {
		using Tree = PkdVQTree<BigInt, 1, UByteI7Codec>;

		using Values = Tree::Values;

		Int block_size = 4096*100000;

		void* block = malloc(block_size);
		memset(block, 0, block_size);

		Tree* tree = T2T<Tree*>(block);
		tree->setTopLevelAllocator();

		long t0 = getTimeInMillis();

		Seed(1234);

		tree->init_tl(block_size);

		Int size = 100;

		vector<Values> data(size);

		for (auto& v: data) v = Values{1 + getRandomG(300)};

		tree->_insert(0, size, [&](Int idx){return data[idx];});

		long t1 = getTimeInMillis();

		tree->dump();


		long ts1 = getTimeInMillis();
		cout<<"----------------------Sums"<<endl;
//		for (Int c = 1; c < tree->size(); c += 1)
//		{
//			auto sum0 = tree->sum(0, c);
//			auto sum1 = tree->plain_sum(0, c);
//
//			if (sum0 != sum1)
//			{
//				cout<<c<<": "<<sum0<<"  "<<sum1<<endl;
//			}
//		}
		long ts2 = getTimeInMillis();


		auto max = tree->sum(0, size);

		cout<<"MAX: "<<max<<endl;

		cout<<"find:    "<<tree->find_ge_fw(0, 0, max).idx()<<endl;
//		cout<<"find_bw: "<<tree->findGEBackward(0, tree->size() - 2, max).idx()<<endl;
		cout<<"find:    "<<tree->find_ge(0, max).idx()<<endl;




		long t2 = getTimeInMillis();

		cout<<"----------------------FindStart"<<endl;
		for (Int c = 0; c < size; c++)
		{
			auto key = tree->sum(0, c + 1);
			auto idx = tree->find_ge(0, key).idx();

			if (idx != c)
			{
				cout<<c<<": "<<key<<"  "<<idx<<endl;
			}
		}

		long t3 = getTimeInMillis();

		cout<<"----------------------FW"<<endl;
		for (Int c = 0; c < size; c++)
		{
			auto key = tree->sum(0, c, size);
			auto idx = tree->find_ge_fw(0, c, key).idx();

			if (idx != size - 1) {
				cout<<c<<": "<<key<<"  "<<idx<<endl;
			}
		}

		long t4 = getTimeInMillis();

		cout<<"----------------------BW"<<endl;
		for (Int c = tree->size() - 1; c >= 0; c--)
		{
			auto key = tree->sum(0, c + 1);

			auto idx = tree->findGEBackward(0, c, key).idx();
			if (idx != 0) {
				cout<<c<<": "<<key<<"  "<<idx<<endl;
			}
		}

		long t5 = getTimeInMillis();

		cout<<"Insert: "<<FormatTime(t1 - t0)<<endl;
		cout<<"Sums: "<<FormatTime(ts2 - ts1)<<endl;
		cout<<"FindStart: "<<FormatTime(t3 - t2)<<endl;
		cout<<"FindFW: "<<FormatTime(t4 - t3)<<endl;
		cout<<"FindBW: "<<FormatTime(t5 - t4)<<endl;
	}
	catch (vapi::Exception& ex) {
		cout<<ex.source()<<endl;
		cout<<ex<<endl;
	}
}


