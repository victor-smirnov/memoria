
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_TOOLS_HPP
#define	_MEMORIA_TOOLS_TOOLS_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/strings.hpp>
#include <memoria/core/tools/array_data.hpp>
#include <memoria/core/container/logs.hpp>

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

	bool operator==(const K& key) const {
		return key_ == key;
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
size_t AppendToSortedVector(vector<T, A>& vec, const T& value)
{
	size_t cnt = 0;
	for (auto i = vec.begin(); i != vec.end(); i++, cnt++)
	{
		if (value < *i)
		{
			vec.insert(i, value);
			return cnt;
		}
	}

	vec.push_back(value);
	return vec.size();
}


Int 	GetRandom();
Int 	GetRandom(Int max);
void 	Seed(Int value);
Int 	GetSeed();

BigInt 	GetBIRandom();
BigInt 	GetBIRandom(BigInt max);
void 	SeedBI(BigInt value);
BigInt 	GetSeedBI();


BigInt	GetTimeInMillis();

String FormatTime(BigInt millis);

void Fill(char* buf, int size, char value);
ArrayData CreateBuffer(Int size, UByte value);
Int GetNonZeroRandom(Int size);
ArrayData CreateRandomBuffer(UByte fill_value, Int max_size);


template <typename Allocator>
void Check(Allocator& allocator, const char* message,  const char* source)
{
	Int level = allocator.GetLogger()->level();

	allocator.GetLogger()->level() = Logger::ERROR;

	if (allocator.Check())
	{
		allocator.GetLogger()->level() = level;

		throw TestException(source, message);
	}

	allocator.GetLogger()->level() = level;
}

template <typename Ctr>
void CheckCtr(Ctr& ctr, const char* message,  const char* source)
{
	Int level = ctr.logger().level();

	ctr.logger().level() = Logger::ERROR;

	if (ctr.Check(NULL))
	{
		ctr.logger().level() = level;
		throw TestException(source, message);
	}

	ctr.logger().level() = level;
}


template <typename BAIterator>
bool CompareBuffer(BAIterator& iter, ArrayData& data, Int& c)
{
	ArrayData buf(data.size());

	iter.Read(buf);

	const UByte* buf0 = buf.data();
	const UByte* buf1 = data.data();

	for (c = 0; c < data.size(); c++)
	{
		if (buf0[c] != buf1[c])
		{
			return false;
		}
	}

	return true;
}

template <typename BAIterator >
void CheckBufferWritten(BAIterator& iter, ArrayData& data, const char* err_msg, const char* source)
{
	Int pos = 0;
	if (!CompareBuffer(iter, data, pos))
	{
		throw TestException(source, String(err_msg) + ": pos=" + ToString(pos));
	}
}


template <typename Iterator, typename Item>
bool CompareBuffer(Iterator& iter, const vector<Item>& data, Int& c)
{
	c = 0;
	for (auto i = data.begin(); i != data.end(); i++, iter.Next(), c++)
	{
		for (Int d = 0; d < Iterator::Indexes; d++)
		{
			auto value = iter.GetRawKey(d);

			if (value != data[c].keys[d])
			{
				return false;
			}
		}
	}

	return true;
}

template <typename Iterator, typename Item>
void CheckBufferWritten(Iterator& iter, const vector<Item>& data, const char* err_msg, const char* source)
{
	Int pos = 0;
	if (!CompareBuffer(iter, data, pos))
	{
		throw TestException(source, String(err_msg) + ": pos=" + ToString(pos));
	}
}

template <typename T, typename A>
Int GetUniqueRandom(const vector<T, A> &vec)
{
	Int value = GetRandom();

	for (const T& item: vec)
	{
		if (item == value)
		{
			return GetUniqueRandom(vec);
		}
	}

	return value;
}


template <typename T, typename A>
BigInt GetUniqueBIRandom(const vector<T, A> &vec, BigInt limit)
{
	Int value = GetBIRandom(limit);

	for (const T& item: vec)
	{
		if (item == value)
		{
			return GetUniqueBIRandom(vec, limit);
		}
	}

	return value;
}


#define MEMORIA_TEST_THROW_IF(op1, operator_, op2) 		MEMORIA_TEST_THROW_IF_EXPR(op1 operator_ op2, op1, op2)
#define MEMORIA_TEST_THROW_IF_1(op1, operator_, op2, arg1) MEMORIA_TEST_THROW_IF_EXPR1(op1 operator_ op2, op1, op2, arg1)

#define MEMORIA_TEST_THROW_IF_EXPR(expr, op1, op2) 																						\
	if (expr) {																															\
		throw TestException(MEMORIA_SOURCE, String("ASSERT FAILURE: ")+#expr+"; "+#op1+"="+ToString(op1)+", "+#op2+"="+ToString(op2));	\
	}


#define MEMORIA_TEST_THROW_IF_EXPR1(expr, op1, op2, arg1) 																												\
	if (expr) {																																							\
		throw TestException(MEMORIA_SOURCE, String("ASSERT FAILURE: ")+#expr+"; "+#op1+"="+ToString(op1)+", "+#op2+"="+ToString(op2)+", "+#arg1+"="+ToString(arg1));	\
	}

}

#endif
