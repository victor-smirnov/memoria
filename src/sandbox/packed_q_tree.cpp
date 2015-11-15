
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/core/packed/tree/packed_fse_quick_tree.hpp>
#include <memoria/core/packed/tree/packed_vle_quick_tree.hpp>
#include <memoria/core/packed/tree/packed_tree_tools.hpp>

#include <iostream>

using namespace memoria;
using namespace std;


int main() {

	try {
		using Tree = PkdFQTree<BigInt, 1>;

		Int block_size = 4096*1000;

		void* block = malloc(block_size);
		memset(block, 0, block_size);

		Tree* tree = T2T<Tree*>(block);
		tree->setTopLevelAllocator();

		tree->init(block_size);

		for (Int c = 0; c < tree->max_size(); c++)
		{
			tree->values(0)[c] = 3;
		}

		tree->size() = tree->max_size();

		tree->reindex();

		tree->dump_index();

		auto max = tree->sum(0);

		cout<<"find: "<<tree->find_ge(0, 1523688-6).idx()<<endl;
		cout<<tree->sum(0, 0, tree->size())<<endl;


//		cout<<"----------------------FW"<<endl;
//
//
//
//		for (Int c = 0; c < tree->size(); c++)
//		{
//			Int key = max - c * 3;
//			cout<<c<<": "<<key<<"  "<<tree->find_ge_fw(0, c, key)<<endl;
//		}
//
//		cout<<"----------------------BW"<<endl;
//
//
//
		for (Int c = tree->size() - 1; c > 0; c--)
		{
			Int key = (c + 1) * 3;
			cout<<c<<": "<<key<<"  "<<tree->findGEBackward(0, c, key).idx()<<endl;
		}

		cout<<tree->find_ge_bw(0, 4952, max).idx()<<endl;
	}
	catch (vapi::Exception& ex) {
		cout<<ex<<endl;
	}
}


