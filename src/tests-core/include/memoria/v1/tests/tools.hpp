
// Copyright 2012 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.



#pragma once

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/tools/terminal.hpp>
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/strings/string.hpp>
#include <memoria/v1/core/container/logs.hpp>

#include <memoria/v1/containers/vector/vctr_names.hpp>

#include <memoria/v1/core/packed/tools/packed_allocator_types.hpp>

#include <memoria/v1/tests/tests.hpp>

#include <vector>
#include <fstream>
#include <stdlib.h>


namespace memoria {
namespace v1 {
namespace tests {

using tools::Term;

class PARemover {
    PackedAllocatable* obj_;
public:
    PARemover(PackedAllocatable* obj): obj_(obj) {}
    ~PARemover()
    {
        if (obj_->has_allocator())
        {
            free_system(obj_->allocator());
        }
        else {
            free_system(obj_);
        }
    }
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
std::ostream& operator<<(std::ostream& out, const tests::KVPair<K, V>& pair)
{
    out << pair.key_ << " " << pair.value_;
    return out;
}

template <typename T, typename A>
std::ostream& operator<<(std::ostream& out, const std::vector<T, A>& vec)
{
    for (const T& value : vec)
    {
        out << value << std::endl;
    }
    return out;
}

template <typename A>
std::ostream& operator<<(std::ostream& out, const std::vector<uint8_t, A>& vec)
{
    out << std::hex;

    for (auto& value : vec)
    {
        out << (int32_t)value << std::endl;
    }

    return out;
}



template <typename K, typename V>
std::istream& operator>>(std::istream& in, tests::KVPair<K, V>& pair)
{
    in >> std::skipws;
    in >> pair.key_;
    in >> std::skipws;
    in >> pair.value_;

    return in;
}


template <typename T, typename A>
std::istream& operator>>(std::istream& in, std::vector<T, A>& vec)
{
    T value;

    do {
        in >> value;
        if (in.eof()) break;
        if (in.bad())
        {
            MMA1_THROW(Exception()) << WhatCInfo("Invalid data record format");
        }

        if (in.fail())
        {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Can't read file at pos {}", in.tellg()));
        }

        vec.push_back(value);
    }
    while (true);

    return in;
}

template <typename A>
std::istream& operator>>(std::istream& in, std::vector<uint8_t, A>& vec)
{
    int32_t value;

    do {
        in >> value;
        if (in.eof()) break;
        if (in.bad())
        {
            MMA1_THROW(Exception()) << WhatCInfo("Invalid data record format");
        }
        vec.push_back(value);
    }
    while (true);

    return in;
}

template <typename T, typename A>
void LoadVector(std::vector<T, A>& vec, U16StringRef file_name)
{
    std::fstream file;

//    file.exceptions(std::ifstream::failbit);

    file.open(file_name.to_u8().data(), std::fstream::in);

    if (file.fail() || file.bad()) {
        MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Can't open file: {}", file_name));
    }

    file>>vec;

    file.close();
}

template <typename T, typename A>
void StoreVector(const std::vector<T, A>& vec, U16StringRef file_name)
{
    std::fstream file;

    file.open(file_name.to_u8().data(), std::fstream::out);

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
std::vector<T> createBuffer(int32_t size, T value)
{
    std::vector<T> vec(size);

    T cnt = 0;
    for (auto& item: vec)
    {
        item = cnt++;

        if (cnt == value) cnt = 0;
    }

    return vec;
}

template <typename T>
std::vector<T> createSimpleBuffer(int32_t size, T value)
{
    std::vector<T> vec(size);

    for (auto& item: vec)
    {
        item = value;
    }

    return vec;
}

template <typename T>
std::vector<T> createRandomBuffer(T fill_value, int32_t max_size)
{
    return createBuffer<T>(getNonZeroRandomG(max_size), fill_value);
}






template <typename Allocator>
void check(Allocator& allocator, const char* message,  const char* source)
{
    int32_t level = allocator.logger().level();

    allocator.logger().level() = Logger::_ERROR;

    if (allocator.check())
    {
        allocator.logger().level() = level;

        MMA1_THROW(TestException()) << WhatCInfo(message);
    }

    allocator.logger().level() = level;
}

template <typename Ctr>
void checkCtr(Ctr& ctr, const char* message,  const char* source)
{
    int32_t level = ctr.logger().level();

    ctr.logger().level() = Logger::_ERROR;

    if (ctr.check(NULL))
    {
        ctr.logger().level() = level;
        MMA1_THROW(TestException()) << WhatCInfo(message);
    }

    ctr.logger().level() = level;
}



template <typename Types, typename T>
bool CompareBuffer(Iter<Vector2IterTypes<Types>>& iter, const std::vector<T>& data, int32_t& c)
{
    auto tmp = iter;

    std::vector<T> buf = iter.subVector(data.size());

    iter.skip(data.size());

    for (c = 0; c < (int32_t)data.size(); c++)
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
    int32_t pos = 0;
    if (!CompareBuffer(iter, data, pos))
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"{}: pos={}", err_msg, pos));
    }
}



template <typename Types, typename MemBuffer>
bool CompareBuffer(Iter<Types>& iter, const MemBuffer& data, int32_t& c)
{
    typedef Iter<Types> Iterator;

    c = 0;
    for (size_t i = 0; i != data.size(); i++, iter.next(), c++)
    {
        for (int32_t d = 0; d < Iterator::Indexes; d++)
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
int32_t getUniqueRandom(const std::vector<T, A> &vec)
{
    int32_t value = getRandomG();

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
int64_t getUniqueBIRandom(const std::vector<T, A> &vec, int64_t limit)
{
    int32_t value = getBIRandomG(limit) + 1;

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
        MMA1_THROW(TestException()) << WhatCInfo("Code throws unexpected exception");
    }

    if (!throwsException)
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"Code doesn't throw exception {}", TypeNameFactory<Exception>::name()));
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
        ex.dump(std::cout);
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"Code throws exception {}", TypeNameFactory<Exception>::name()));
    }
    catch (...)
    {
        MMA1_THROW(TestException()) << WhatCInfo("Code throws unexpected exception");
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
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"Code throws unexpected exception: {} {}", ex));
    }
    catch (...)
    {
        MMA1_THROW(TestException()) << WhatCInfo("Code throws unknown exception");
    }
}

template <typename Op>
void AssertTrue(const char* src, const Op& op, const SBuf& msg)
{
    if (!(op))
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"True assertion failed: {} {}", op, msg.str()));
    }
}

template <typename Op>
void AssertTrue(const char* src, const Op& op)
{
    if (!(op))
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"True assertion failed: {}", op));
    }
}


template <typename Op>
void AssertFalse(const char* src, const Op& op)
{
    if (op)
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"False assertion failed: {}", op));
    }
}

template <typename Op>
void AssertFalse(const char* src, const Op& op, const SBuf& msg)
{
    if (op)
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"False assertion failed: {} {}", op, msg.str()));
    }
}


template <typename Op1, typename Op2>
void AssertEQ(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg)
{
    if (!(op1 == op2))
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"EQ assertion failed: {} {} {}", op1, op2, msg.str()));
    }
}

template <typename Op1, typename Op2>
void AssertEQ(const char* src, const Op1& op1, const Op2& op2, const std::function<SBuf ()> msg_fn)
{
    if (!(op1 == op2))
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"EQ assertion failed: {} {} {}", op1, op2, msg_fn().str()));
    }
}

template <typename Op1, typename Op2>
void AssertEQ(const char* src, const Op1& op1, const Op2& op2)
{
    if (!(op1 == op2))
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"EQ assertion failed: {} {}", op1, op2));
    }
}

template <typename Op1, typename Op2>
void AssertEQ(const char* src, const std::vector<Op1>& op1, const std::vector<Op2>& op2)
{
    if (op1.size() != op2.size())
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"EQ size assertion failed: {} {}", op1.size(), op2.size()));
    }
    else {
        for (size_t c = 0; c < op1.size(); c++)
        {
            if (op1[c] != op2[c])
            {
                MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"EQ data assertion failed: {} {} {}", c, op1[c], op2[c]));
            }
        }
    }
}

template <typename Op1, typename Op2>
void AssertLT(const char* src, const Op1& op1, const Op2& op2)
{
    if (!(op1 < op2))
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"LT assertion failed: {} {}", op1, op2));
    }
}

template <typename Op1, typename Op2>
void AssertLT(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg)
{
    if (!(op1 < op2))
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"LT assertion failed: {} {} {}", op1, op2, msg.str()));
    }
}

template <typename Op1, typename Op2>
void AssertLE(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg)
{
    if (!(op1 <= op2))
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"LE assertion failed: {} {} {}", op1, op2, msg.str()));
    }
}

template <typename Op1, typename Op2>
void AssertLE(const char* src, const Op1& op1, const Op2& op2)
{
    if (!(op1 <= op2))
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"LE assertion failed: {} {}", op1, op2));
    }
}

template <typename Op1, typename Op2>
void AssertGT(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg = SBuf())
{
    if (!(op1 > op2))
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"GT assertion failed: {} {} {}", op1, op2, msg.str()));
    }
}

template <typename Op1, typename Op2>
void AssertGE(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg = SBuf())
{
    if (!(op1 >= op2))
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"GE assertion failed: {} {} {}", op1, op2, msg.str()));
    }
}

template <typename Op1, typename Op2>
void AssertNEQ(const char* src, const Op1& op1, const Op2& op2)
{
    if (!(op1 != op2))
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"NEQ assertion failed: {} {}", op1, op2));
    }
}


template <typename Op1, typename Op2>
void AssertNEQ(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg)
{
    if (!(op1 != op2))
    {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"NEQ assertion failed: {} {} {}", op1, op2, msg.str()));
    }
}




}}}
