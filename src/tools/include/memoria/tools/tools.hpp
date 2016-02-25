
// Copyright Victor Smirnov 2012+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_TOOLS_HPP
#define _MEMORIA_TOOLS_TOOLS_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/terminal.hpp>
#include <memoria/core/tools/random.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/container/logs.hpp>

#include <memoria/containers/vector/vctr_names.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>

#include <vector>
#include <fstream>

#include <malloc.h>
#include <memoria/core/tools/strings/strings.hpp>

namespace memoria {

using namespace memoria::vapi;
using tools::Term;

class PARemover {
    PackedAllocatable* obj_;
public:
    PARemover(PackedAllocatable* obj): obj_(obj) {}
    ~PARemover()
    {
        if (obj_->has_allocator())
        {
            free(obj_->allocator());
        }
        else {
            free(obj_);
        }
    }
};

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
std::ostream& operator<<(std::ostream& out, const memoria::KVPair<K, V>& pair)
{
    out<<pair.key_<<" "<<pair.value_;
    return out;
}

template <typename T, typename A>
std::ostream& operator<<(std::ostream& out, const vector<T, A>& vec)
{
    for (const T& value : vec)
    {
        out<<value<<endl;
    }
    return out;
}

template <typename A>
std::ostream& operator<<(std::ostream& out, const std::vector<UByte, A>& vec)
{
    out<<hex;

    for (auto& value : vec)
    {
        out<<(Int)value<<endl;
    }

    return out;
}



template <typename K, typename V>
istream& operator>>(std::istream& in, memoria::KVPair<K, V>& pair)
{
    in>>skipws;
    in>>pair.key_;
    in>>skipws;
    in>>pair.value_;

    return in;
}


template <typename T, typename A>
istream& operator>>(std::istream& in, std::vector<T, A>& vec)
{
    T value;

    do {
        in>>value;
        if (in.eof()) break;
        if (in.bad())
        {
            throw Exception(MEMORIA_SOURCE, "Invalid data record format");
        }

        if (in.fail())
        {
        	throw Exception(MEMORIA_SOURCE, SBuf()<< "Can't read file at pos "<<in.tellg());
        }

        vec.push_back(value);
    }
    while (true);

    return in;
}

template <typename A>
istream& operator>>(std::istream& in, std::vector<UByte, A>& vec)
{
    Int value;

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
void LoadVector(std::vector<T, A>& vec, StringRef file_name)
{
    std::fstream file;

//    file.exceptions(std::ifstream::failbit);

    file.open(file_name.c_str(), std::fstream::in);

    if (file.fail() || file.bad()) {
        throw Exception(MEMORIA_SOURCE, "Can't open file: "+file_name);
    }

    file>>vec;

    file.close();
}

template <typename T, typename A>
void StoreVector(const std::vector<T, A>& vec, StringRef file_name)
{
    std::fstream file;

    file.open(file_name.c_str(), std::fstream::out);

    file<<vec;

    file.close();
}


template <typename T, typename A>
size_t appendToSortedVector(std::vector<T, A>& vec, const T& value)
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

template <typename T>
vector<T> createRandomBuffer(T fill_value, Int max_size)
{
    return createBuffer<T>(getNonZeroRandomG(max_size), fill_value);
}






template <typename Allocator>
void check(const std::shared_ptr<Allocator>& allocator, const char* message,  const char* source)
{
    Int level = allocator->logger().level();

    allocator->logger().level() = Logger::ERROR;

    if (allocator->check())
    {
        allocator->logger().level() = level;

        throw TestException(source, message);
    }

    allocator->logger().level() = level;
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




template <typename T, typename A>
Int getUniqueRandom(const vector<T, A> &vec)
{
    Int value = getRandomG();

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
    Int value = getBIRandomG(limit) + 1;

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
    catch (Exception& ex)
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
    catch (Exception& ex)
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
    catch (Exception& ex)
    {
        throw TestException(src, SBuf()<<"Code throws unexpected exception: "<<ex.source()<<" "<<ex);
    }
    catch (...)
    {
        throw TestException(src, SBuf()<<"Code throws unknown exception");
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
