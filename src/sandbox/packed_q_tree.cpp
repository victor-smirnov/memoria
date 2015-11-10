
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/core/packed/tree/packed_fse_quick_tree.hpp>
#include <memoria/core/packed/tree/packed_tree_tools.hpp>

#include <iostream>

using namespace memoria;
using namespace std;


int main() {

	try {
		using Tree = PkdFQTree<BigInt>;

		void* block = malloc(4096);
		memset(block, 0, 4096);

		Tree* tree = T2T<Tree*>(block);
		tree->setTopLevelAllocator();

		tree->init(4096, 1);

		for (Int c = 0; c < tree->max_size(); c++)
		{
			tree->values(0)[c] = 3;
		}

		tree->size() = tree->max_size();

		tree->reindex();

		tree->dump();

		cout<<"find: "<<tree->find_ge(0, 1000)<<endl;
	}
	catch (vapi::Exception& ex) {
		cout<<ex<<endl;
	}
}


