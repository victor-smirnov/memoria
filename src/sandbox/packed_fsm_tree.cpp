
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/core/packed/tree/fse_max/packed_fse_max_tree.hpp>

#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/uuid.hpp>


#include <memoria/core/packed/tools/packed_struct_ptrs.hpp>

#include <iostream>

#include <cstddef>

using namespace memoria;
using namespace std;





int main() {

	try {
		constexpr Int Block = 0;
		constexpr Int Blocks = Block + 1;


		using Tree = PkdFMTreeT<UUID, Blocks>;

//		using Values = Tree::Values;

		Int block_size = 4096*100000;

		auto tree = MakeSharedPackedStructByBlock<Tree>(block_size);

		Int size = 2048;



		vector<UUID> data(size);

		for (auto& v: data) v = UUID::make_random();

		auto data_s = data;

		std::sort(data_s.begin(), data_s.end());

//		long t0 = getTimeInMillis();
		tree->_insert(0, size, [&](Int block, Int idx){return data_s[idx];});

//		long t1 = getTimeInMillis();

//		tree->dump();

		cout<<"Find test: "<<endl;

//		for (auto& v: data)
//		{
//			auto result = tree->find_ge(Block, v);
//
////			cout<<v<<" -- "<<result.idx()<<" -- "<<data_s[result.idx()]<<endl;
//
//			MEMORIA_ASSERT(v, ==, data_s[result.idx()]);
//		}

		auto nv = UUID::make_random();

		auto result = tree->find_ge(Block, nv);

		if (result.idx() >= size)
		{
			cout<<nv<<" -- "<<result.idx()<<endl;
		}
		else {
			cout<<nv<<" -- "<<result.idx()<<" "<<data_s[result.idx()]<<endl;
			cout<<(nv < data_s[result.idx()])<<endl;
			cout<<(nv == data_s[result.idx()])<<endl;
			cout<<(nv > data_s[result.idx() - 1])<<endl;
		}

	}
	catch (PackedOOMException& ex) {
		cout<<ex.source()<<endl;
		cout<<ex<<endl;
	}
	catch (vapi::Exception& ex) {
		cout<<ex.source()<<endl;
		cout<<ex<<endl;
	}
}



