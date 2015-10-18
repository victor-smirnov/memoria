// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/core/tools/cow_tree/cow_tree.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/random.hpp>

#include <vector>
#include <thread>

using namespace memoria;
using namespace memoria::cow::tree;
using namespace std;





int main(int argc, const char** argv, const char** envp)
{
	CoWTree<BigInt, BigInt> tree;
	try {
		Int size = 1000;

		vector<BigInt> keys;

		BigInt tr0 = getTimeInMillis();

		for (Int c = 0; c < 1000; c++)
		{
			vector<BigInt> lkeys(size);

			for (auto& k: lkeys)
			{
				k = getRandomG();
			}

			auto txn = tree.transaction();

			for (auto c = 0; c < lkeys.size(); c++)
			{
				tree.assign(txn, lkeys[c], c);
			}

			txn.commit();

			keys.insert(keys.end(), lkeys.begin(), lkeys.end());
		}

		cout<<"Random array created in "<<FormatTime(getTimeInMillis() - tr0)<<endl;

		auto txn2 = tree.transaction();

		tree.assign(txn2, 555, 333);

		txn2.commit();

		tree.dump_log();

		cout<<"cleanup snapshots..."<<endl;
		tree.cleanup_snapshots();

		tree.dump_log();

		BigInt found = 0;

		auto sn = tree.snapshot();

		for (auto k: keys) {
			found += tree.find(sn, k);
		}

		found += tree.find(sn, 555);

		cout<<"Found "<<found<<" keys"<<endl;
	}
	catch (std::exception& ex) {
		cout<<ex.what()<<endl;
	}
}
