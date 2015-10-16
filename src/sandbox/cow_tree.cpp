// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/core/tools/cow_tree/cow_tree.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/random.hpp>

#include <vector>

using namespace memoria;
using namespace memoria::cow::tree;
using namespace std;

int main(int argc, const char** argv, const char** envp)
{
	CoWTree<BigInt, BigInt> tree;

	Int size = 3000000;

	vector<BigInt> keys(size);

	BigInt tr0 = getTimeInMillis();

//	SeedBI(0xaa123bb213ce);

	for (auto& k: keys)
	{
		k = getBIRandomG();
	}

	cout<<"Random array created in "<<FormatTime(getTimeInMillis() - tr0)<<endl;

	BigInt t0 = getTimeInMillis();

	for (int c = 0; c < size; c++)
	{
		tree.assign(keys[c], c);
	}

	cout<<"Tree created in: "<<FormatTime(getTimeInMillis() - t0)<<endl;

	cout<<keys.size()<<" "<<tree.size()<<endl;

	for (Int c = 0; c < size; c++)
	{
		auto key = keys[c];

		auto op = tree.find(0, key + 1);

		if (op) {
			cout<<"False found! "<<key<<" "<<op<<endl;
		}
	}

	for (Int c = 0; c < size; c++)
	{
		auto key = keys[c];

		auto op = tree.find(0, key);

		if (op) {
			if (op.value() != c)
			{
				cout<<"Incorrect key: "<<op.value()<<" "<<c<<endl;
			}
		}
		else {
			cout<<"Not found! "<<key<<endl;
		}
	}


	Int cnt = 0;
	for (auto iter = tree.begin(); !iter.is_end(); iter++)
	{
		cnt++;
	}

	cout<<"FW Cnt = "<<cnt<<endl;

	cnt = 0;

	for (auto iter = tree.rbegin(); !iter.is_start(); iter--)
	{
		cnt++;
	}

	cout<<"BW Cnt = "<<cnt<<endl;

	long td0 = getTimeInMillis();

	for (const auto& k: keys)
	{
		if (tree.remove(k))
		{
//			cout<<"Removed "<<k<<endl;
		}
		else {
			cout<<"Didn't remove "<<k<<endl;
		}

	}

	cout<<"After insertion = "<<tree.size()<<", time = "<<FormatTime(getTimeInMillis() - td0)<<endl;


//	for (auto iter = tree.rbegin(); !iter.is_start(); iter--)
//	{
//		cout<<"Iter: "<<iter.idx()<<": "<<iter.key()<<" = "<<iter.value()<<endl;
//	}
}
