
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_TOOLS_HPP
#define	_MEMORIA_TOOLS_TOOLS_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/strings.hpp>

#include <vector>
#include <fstream>

namespace memoria {

using namespace memoria::vapi;
using namespace std;


class TestException: public MemoriaException {
public:
	TestException(StringRef source, StringRef message): MemoriaException(source, message) {}
};


template <typename K, typename V>
struct KVPair {
	K key_;
	V value_;

	KVPair() {}
	KVPair(const K& key, const V& value): key_(key), value_(value) {}

	bool operator<(const KVPair<K, V>& pair) const {
		return key_ < pair.key_;
	}
};

template <typename K, typename V>
ostream& operator<<(ostream& out, const memoria::KVPair<K, V>& pair)
{
	out<<pair.key_<<" "<<pair.value_;
	return out;
}

template <typename T, typename A>
ostream& operator<<(ostream& out, const vector<T, A>& vec)
{
	for (const T& value : vec)
	{
		out<<value<<endl;
	}
	return out;
}


template <typename K, typename V>
istream& operator>>(istream& in, memoria::KVPair<K, V>& pair)
{
	in>>skipws;
	in>>pair.key_;
	in>>skipws;
	in>>pair.value_;

	return in;
}


template <typename T, typename A>
istream& operator>>(istream& in, vector<T, A>& vec)
{
	T value;

	do {
		in>>value;
		if (in.eof()) break;
		if (in.bad())
		{
			throw MemoriaException(MEMORIA_SOURCE, "Invalid data record format");
		}

		vec.push_back(value);
	}
	while (true);

	return in;
}



template <typename T, typename A>
void LoadVector(vector<T, A>& vec, StringRef file_name)
{
	fstream file;

	file.open(file_name.c_str(), fstream::in);

	file>>vec;

	file.close();
}

template <typename T, typename A>
void StoreVector(const vector<T, A>& vec, StringRef file_name)
{
	fstream file;

	file.open(file_name.c_str(), fstream::out);

	file<<vec;

	file.close();
}


template <typename T, typename A>
void AppendToSortedVector(vector<T, A>& vec, const T& value)
{
	for (auto i = vec.begin(); i != vec.end(); i++)
	{
		if (value < *i)
		{
			vec.insert(i, value);
			return;
		}
	}

	vec.push_back(value);
}


Int 	GetRandom();
Int 	GetRandom(Int max);
void 	Seed(Int value);

BigInt 	GetBIRandom();
BigInt 	GetBIRandom(BigInt max);
void 	SeedBI(BigInt value);


BigInt	GetTimeInMillis();

String FormatTime(BigInt millis);


#define MEMORIA_TEST_ASSERT(op1, operator_, op2) MEMORIA_TEST_ASSERT_EXPR(op1 operator_ op2, op1, op2)
#define MEMORIA_TEST_ASSERT1(op1, operator_, op2, arg1) MEMORIA_TEST_ASSERT_EXPR1(op1 operator_ op2, op1, op2, arg1)

#define MEMORIA_TEST_ASSERT_EXPR(expr, op1, op2) 																						\
	if (expr) {																															\
		throw TestException(MEMORIA_SOURCE, String("ASSERT FAILURE: ")+#expr+"; "+#op1+"="+ToString(op1)+", "+#op2+"="+ToString(op2));	\
	}


#define MEMORIA_TEST_ASSERT_EXPR1(expr, op1, op2, arg1) 																												\
	if (expr) {																																							\
		throw TestException(MEMORIA_SOURCE, String("ASSERT FAILURE: ")+#expr+"; "+#op1+"="+ToString(op1)+", "+#op2+"="+ToString(op2)+", "+#arg1+"="+ToString(arg1));	\
	}

}

#endif
