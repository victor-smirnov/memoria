
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/core/packed/tree/packed_fse_quick_tree.hpp>
#include <memoria/core/packed/tree/packed_vle_quick_tree.hpp>
#include <memoria/core/packed/tree/packed_vle_dense_tree.hpp>
#include <memoria/core/packed/tree/packed_vle_tree.hpp>
#include <memoria/core/packed/tree/packed_tree_tools.hpp>

#include <memoria/core/tools/i7_codec.hpp>
#include <memoria/core/tools/elias_codec.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/random.hpp>

#include <iostream>

using namespace memoria;
using namespace std;


int main() {

	try {
		constexpr Int Block = 1;
		constexpr Int Blocks = Block + 1;

//		using Tree = PkdVQTree<BigInt, Blocks, UByteI7Codec>;
		using Tree = PkdVDTree<BigInt, Blocks, UByteI7Codec, BigInt, PackedTreeBranchingFactor, 128>;
//		using Tree = PkdFQTree<BigInt, Blocks>;
//		using Tree = PkdVTree<Packed2TreeTypes<BigInt, BigInt, Blocks, UByteI7Codec, PackedTreeBranchingFactor, 128>>;

		using Values = Tree::Values;

		Int block_size = 4096*100000;

		void* block = malloc(block_size);
		memset(block, 0, block_size);

		Tree* tree = T2T<Tree*>(block);
		tree->setTopLevelAllocator();

		Seed(1234);

		tree->init_tl(block_size);

		Int size = 1000000;

		vector<Values> data(size);

		for (auto& v: data) v = Values{1 + getRandomG(300), 1 + getRandomG(300)};

		long t0 = getTimeInMillis();
		tree->_insert(0, size, [&](Int idx){return data[idx];});

		long t1 = getTimeInMillis();

//		tree->dump();


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


		auto max = tree->sum(Block, size);

		cout<<"MAX: "<<max<<endl;

		cout<<tree->sum(0, size)<<endl;
		cout<<tree->sum(1, size)<<endl;

		cout<<"find:    "<<tree->findGEForward(Block, 0, max).idx()<<endl;
		cout<<"find_bw: "<<tree->findGEBackward(Block, size - 1, max).idx()<<endl;


		long t2 = getTimeInMillis();

		cout<<"----------------------FindStart"<<endl;
		for (Int c = 0; c < size; c++)
		{
			auto key = tree->sum(Block, c + 1);
			auto idx = tree->find_ge(Block, key).idx();

			if (idx != c)
			{
				cout<<c<<": "<<key<<"  "<<idx<<endl;
			}
		}

		long t3 = getTimeInMillis();

		cout<<"----------------------FW"<<endl;
		for (Int c = 0; c < size; c++)
		{
			auto key = tree->sum(Block, c, size);
			auto idx = tree->find_ge_fw(Block, c, key).idx();

			if (idx != size - 1) {
				cout<<c<<": "<<key<<"  "<<idx<<endl;
			}
		}

		long t4 = getTimeInMillis();

		cout<<"----------------------BW"<<endl;
		for (Int c = tree->size() - 1; c >= 0; c--)
		{
			auto key = tree->sum(Block, c + 1);

			auto idx = tree->findGEBackward(Block, c, key).idx();
			if (idx != 0) {
				cout<<c<<": "<<key<<"  "<<idx<<endl;
			}
		}

		long t5 = getTimeInMillis();

		vector<BigInt> keys(10000000);
		for (auto& v: keys) v = getRandomG(max);

		long t6 = getTimeInMillis();

		for (auto k: keys)
		{
			auto idx = tree->find_ge(Block, k).idx();
			if (idx == size) {
				cout<<k<<"  "<<idx<<endl;
			}
		}

		long t7 = getTimeInMillis();


		vector<BigInt> keys_s(10000000);
		BigInt sum = 0;

		tree->scan(Block, 0, size, [&](Int c, auto value){
			keys_s[c] = sum + value;
			sum += value;
		});

		long t8 = getTimeInMillis();

		for (auto k: keys_s)
		{
			auto idx = tree->find_ge(0, k).idx();
			if (idx == size) {
				cout<<k<<"  "<<idx<<endl;
			}
		}

		long t9 = getTimeInMillis();


		cout<<"Insert: "<<FormatTime(t1 - t0)<<endl;
		cout<<"Sums: "<<FormatTime(ts2 - ts1)<<endl;
		cout<<"FindStart: "<<FormatTime(t3 - t2)<<endl;
		cout<<"FindFW: "<<FormatTime(t4 - t3)<<endl;
		cout<<"FindBW: "<<FormatTime(t5 - t4)<<endl;

		cout<<"FindRnd: "<<FormatTime(t7 - t6)<<endl;
		cout<<"FindSeq: "<<FormatTime(t9 - t8)<<endl;
	}
	catch (vapi::Exception& ex) {
		cout<<ex.source()<<endl;
		cout<<ex<<endl;
	}
}



