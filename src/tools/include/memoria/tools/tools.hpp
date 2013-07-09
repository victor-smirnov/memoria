
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_TOOLS_HPP
#define _MEMORIA_TOOLS_TOOLS_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/strings.hpp>
#include <memoria/core/tools/idata.hpp>
//#include <memoria/core/tools/symbol_sequence.hpp>
#include <memoria/core/container/logs.hpp>

#include <memoria/containers/vector/vector_names.hpp>
#include <memoria/containers/vector_map/vectormap_names.hpp>
#include <memoria/prototypes/sequence/names.hpp>


#include <vector>
#include <fstream>

namespace memoria {

using namespace memoria::vapi;
using namespace memoria::vmap;
using namespace std;


class TestException: public Exception {
public:
    TestException(const char* source, StringRef message): Exception(source, message)         {}
    TestException(const char* source, const SBuf& message): Exception(source, message.str()) {}
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
            throw Exception(MEMORIA_SOURCE, "Invalid data record format");
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

    if (file.fail() || file.bad()) {
    	throw Exception(MEMORIA_SOURCE, "Can't open file: "+file_name);
    }

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
size_t appendToSortedVector(vector<T, A>& vec, const T& value)
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


Int     getRandom();
Int     getRandom(Int max);
void    Seed(Int value);
Int     getSeed();

BigInt  getBIRandom();
BigInt  getBIRandom(BigInt max);
void    SeedBI(BigInt value);
BigInt  getSeedBI();


BigInt  getTimeInMillis();

String FormatTime(BigInt millis);

void Fill(char* buf, int size, char value);

template <typename T>
vector<T> createBuffer(Int size, T value)
{
    vector<T> vec(size);

    T cnt = 0;
    for (auto& item: vec)
    {
        item = cnt++;

        if (cnt == value) cnt = 0;
    }

    return vec;
}

template <typename T>
vector<T> createSimpleBuffer(Int size, T value)
{
    vector<T> vec(size);

    for (auto& item: vec)
    {
        item = value;
    }

    return vec;
}

Int getNonZeroRandom(Int size);

template <typename T>
vector<T> createRandomBuffer(T fill_value, Int max_size)
{
    return createBuffer<T>(getNonZeroRandom(max_size), fill_value);
}






template <typename Allocator>
void check(Allocator& allocator, const char* message,  const char* source)
{
    Int level = allocator.getLogger()->level();

    allocator.getLogger()->level() = Logger::ERROR;

    if (allocator.check())
    {
        allocator.getLogger()->level() = level;

        throw TestException(source, message);
    }

    allocator.getLogger()->level() = level;
}

template <typename Ctr>
void checkCtr(Ctr& ctr, const char* message,  const char* source)
{
    Int level = ctr.logger().level();

    ctr.logger().level() = Logger::ERROR;

    if (ctr.check(NULL))
    {
        ctr.logger().level() = level;
        throw TestException(source, message);
    }

    ctr.logger().level() = level;
}



template <typename Types, typename T>
bool CompareBuffer(Iter<Vector2IterTypes<Types>>& iter, const vector<T>& data, Int& c)
{
    auto tmp = iter;

    vector<T> buf = iter.subVector(data.size());

    iter.skip(data.size());

    for (c = 0; c < (Int)data.size(); c++)
    {
        if (buf[c] != data[c])
        {
            MemBuffer<T> mbuf(&buf[0], buf.size());
            MemBuffer<const T> mdata(&data[0], data.size());

            mbuf.dump(cout);
            mdata.dump(cout);

            return false;
        }
    }

    return true;
}


template <typename BAIterator, typename MemBuffer>
void checkBufferWritten(BAIterator& iter, const MemBuffer& data, const char* err_msg, const char* source)
{
    Int pos = 0;
    if (!CompareBuffer(iter, data, pos))
    {
        throw TestException(source, SBuf()<<err_msg<<": pos="<<pos);
    }
}

template <typename Types, typename Item>
void checkBufferWritten(Iter<VectorMap2IterTypes<Types>>& iter, const vector<Item>& data, const char* err_msg, const char* source)
{
    Int pos = 0;
    if (!CompareBuffer(iter.ba_iter(), data, pos))
    {
        throw TestException(source, SBuf()<<err_msg<<": pos="<<pos);
    }
}


template <typename Types, typename MemBuffer>
bool CompareBuffer(Iter<Types>& iter, const MemBuffer& data, Int& c)
{
    typedef Iter<Types> Iterator;

    c = 0;
    for (size_t i = 0; i != data.size(); i++, iter.next(), c++)
    {
        for (Int d = 0; d < Iterator::Indexes; d++)
        {
            auto value = iter.getRawKey(d);

            if (value != data[c].keys[d])
            {
                return false;
            }
        }
    }

    return true;
}


//template <typename Types, Int Bits>
//bool CompareBuffer(Iter<SequenceIterTypes<Types>>& iter, const SymbolSequence<Bits>& data, Int& c)
//{
////    typedef Iter<Types> Iterator;
//
//    c = 0;
//    for (size_t i = 0; i != data.size(); i++, iter.next(), c++)
//    {
//
//
////            auto value = iter.getRawKey(d);
////
////            if (value != data[c])
////            {
////                return false;
////            }
//
//    }
//
//    return true;
//}


template <typename T, typename A>
Int getUniqueRandom(const vector<T, A> &vec)
{
    Int value = getRandom();

    for (const T& item: vec)
    {
        if (item == value)
        {
            return getUniqueRandom(vec);
        }
    }

    return value;
}


template <typename T, typename A>
BigInt getUniqueBIRandom(const vector<T, A> &vec, BigInt limit)
{
    Int value = getBIRandom(limit);

    for (const T& item: vec)
    {
        if (item == value)
        {
            return getUniqueBIRandom(vec, limit);
        }
    }

    return value;
}


template <typename Exception, typename Functor>
void AssertThrows(const char* src, Functor&& fn)
{
	bool throwsException;

	try {
		fn();
		throwsException = false;
	}
	catch (Exception ex)
	{
		throwsException = true;
	}
	catch (...)
	{
		throw TestException(src, SBuf()<<"Code throws unexpected exception");
	}

	if (!throwsException)
	{
		throw TestException(src, SBuf()<<"Code doesn't throw exception "<<TypeNameFactory<Exception>::name());
	}
}

template <typename Exception, typename Functor>
void AssertDoesntThrowEx(const char* src, Functor&& fn)
{
	try {
		fn();
	}
	catch (Exception ex)
	{
		throw TestException(src, SBuf()<<"Code throws exception "<<TypeNameFactory<Exception>::name());
	}
	catch (...)
	{
		throw TestException(src, SBuf()<<"Code throws unexpected exception");
	}
}

template <typename Functor>
void AssertDoesntThrow(const char* src, Functor&& fn)
{
	try {
		fn();
	}
	catch (...)
	{
		throw TestException(src, SBuf()<<"Code throws exception");
	}
}

template <typename Op>
void AssertTrue(const char* src, const Op& op, const SBuf& msg)
{
	if (!(op))
	{
		throw TestException(src, SBuf()<<"True assertion failed: "<<op<<" "<<msg.str());
	}
}

template <typename Op>
void AssertTrue(const char* src, const Op& op)
{
	if (!(op))
	{
		throw TestException(src, SBuf()<<"True assertion failed: "<<op);
	}
}


template <typename Op>
void AssertFalse(const char* src, const Op& op)
{
	if (op)
	{
		throw TestException(src, SBuf()<<"False assertion failed: "<<op);
	}
}

template <typename Op>
void AssertFalse(const char* src, const Op& op, const SBuf& msg)
{
	if (op)
	{
		throw TestException(src, SBuf()<<"False assertion failed: "<<op<<" "<<msg.str());
	}
}


template <typename Op1, typename Op2>
void AssertEQ(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg)
{
	if (!(op1 == op2))
	{
		throw TestException(src, SBuf()<<"EQ assertion failed: "<<op1<<" "<<op2<<" "<<msg.str());
	}
}

template <typename Op1, typename Op2>
void AssertEQ(const char* src, const Op1& op1, const Op2& op2, const std::function<SBuf ()> msg_fn)
{
	if (!(op1 == op2))
	{
		throw TestException(src, SBuf()<<"EQ assertion failed: "<<op1<<" "<<op2<<" "<<msg_fn().str());
	}
}

template <typename Op1, typename Op2>
void AssertEQ(const char* src, const Op1& op1, const Op2& op2)
{
	if (!(op1 == op2))
	{
		throw TestException(src, SBuf()<<"EQ assertion failed: "<<op1<<" "<<op2);
	}
}

template <typename Op1, typename Op2>
void AssertLT(const char* src, const Op1& op1, const Op2& op2)
{
	if (!(op1 < op2))
	{
		throw TestException(src, SBuf()<<"LT assertion failed: "<<op1<<" "<<op2);
	}
}

template <typename Op1, typename Op2>
void AssertLT(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg)
{
	if (!(op1 < op2))
	{
		throw TestException(src, SBuf()<<"LT assertion failed: "<<op1<<" "<<op2<<" "<<msg.str());
	}
}

template <typename Op1, typename Op2>
void AssertLE(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg)
{
	if (!(op1 <= op2))
	{
		throw TestException(src, SBuf()<<"LE assertion failed: "<<op1<<" "<<op2<<" "<<msg.str());
	}
}

template <typename Op1, typename Op2>
void AssertLE(const char* src, const Op1& op1, const Op2& op2)
{
	if (!(op1 <= op2))
	{
		throw TestException(src, SBuf()<<"LE assertion failed: "<<op1<<" "<<op2);
	}
}

template <typename Op1, typename Op2>
void AssertGT(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg = SBuf())
{
	if (!(op1 > op2))
	{
		throw TestException(src, SBuf()<<"GT assertion failed: "<<op1<<" "<<op2<<" "<<msg.str());
	}
}

template <typename Op1, typename Op2>
void AssertGE(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg = SBuf())
{
	if (!(op1 >= op2))
	{
		throw TestException(src, SBuf()<<"GE assertion failed: "<<op1<<" "<<op2<<" "<<msg.str());
	}
}

template <typename Op1, typename Op2>
void AssertNEQ(const char* src, const Op1& op1, const Op2& op2)
{
	if (!(op1 != op2))
	{
		throw TestException(src, SBuf()<<"NEQ assertion failed: "<<op1<<" "<<op2);
	}
}


template <typename Op1, typename Op2>
void AssertNEQ(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg)
{
	if (!(op1 != op2))
	{
		throw TestException(src, SBuf()<<"NEQ assertion failed: "<<op1<<" "<<op2<<" "<<msg.str());
	}
}


}

#endif
