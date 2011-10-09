/*
 * tools.hpp
 *
 *  Created on: 17.02.2011
 *      Author: Victor
 */

#ifndef MEMOTIA_TESTS_TOOLS_HPP_
#define MEMOTIA_TESTS_TOOLS_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/id.hpp>

#include <vector>
#include <set>
#include <map>
#include <algorithm>

using namespace std;
using namespace memoria;
using namespace memoria::vapi;

struct KVPair {
    BigInt key_, value_;

    KVPair(BigInt key, BigInt value): key_(key), value_(value) {}

    KVPair(){}
};

typedef vector<KVPair> PairVector;



struct KIDPair {
    BigInt	key_;
    IDValue	value_;

    KIDPair(BigInt key, const IDValue &value): key_(key), value_(value) {}

    KIDPair(){}
};

typedef vector<KIDPair> IDPairVector;

template <typename KVPair1>
bool KVPairSortPredicate(const KVPair1& d1, const KVPair1& d2)
{
  return d1.key_ < d2.key_;
}

template <typename PairVector1>
void sort(PairVector1& v) {
	std::sort(v.begin(), v.end(), KVPairSortPredicate<typename PairVector1::value_type>);
}


#endif
