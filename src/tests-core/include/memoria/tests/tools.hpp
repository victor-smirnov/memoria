
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

#include <memoria/core/types.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/terminal.hpp>
#include <memoria/core/tools/random.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/strings/string.hpp>

#include <memoria/containers/vector/vctr_names.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>

#include <memoria/tests/tests.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/datatypes/datatypes.hpp>
#include <memoria/core/tools/random.hpp>


#include <vector>
#include <fstream>
#include <stdlib.h>


namespace memoria {
namespace tests {

template <typename DataType> struct DTTCxxValueType;

template <>
struct DTTCxxValueType<Varchar>: HasType<U8String>{};

template <>
struct DTTCxxValueType<UUID>: HasType<UUID>{};

template <>
struct DTTCxxValueType<UTinyInt>: HasType<uint8_t>{};

template <typename DataType> struct DTTestTools;

template <>
struct DTTestTools<UUID> {
    static UUID generate_random() noexcept {
        uint64_t hi_val = static_cast<uint64_t>(getBIRandomG());
        uint64_t lo_val = static_cast<uint64_t>(getBIRandomG());
        return UUID(hi_val, lo_val);
    }
};

template <>
struct DTTestTools<Varchar> {
    static U8String generate_random() noexcept {
        return create_random_string(16);
    }
};

template <>
struct DTTestTools<UTinyInt> {
    static uint8_t generate_random() noexcept {
        return static_cast<uint8_t>(getRandomG(256));
    }
};

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
            MMA_THROW(Exception()) << WhatCInfo("Invalid data record format");
        }

        if (in.fail())
        {
            MMA_THROW(Exception()) << WhatInfo(format_u8("Can't read file at pos {}", in.tellg()));
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
            MMA_THROW(Exception()) << WhatCInfo("Invalid data record format");
        }
        vec.push_back(value);
    }
    while (true);

    return in;
}

template <typename T, typename A>
void LoadVector(std::vector<T, A>& vec, U8StringRef file_name)
{
    std::fstream file;

//    file.exceptions(std::ifstream::failbit);

    file.open(file_name.to_u8().data(), std::fstream::in);

    if (file.fail() || file.bad()) {
        MMA_THROW(Exception()) << WhatInfo(format_u8("Can't open file: {}", file_name));
    }

    file>>vec;

    file.close();
}

template <typename T, typename A>
void StoreVector(const std::vector<T, A>& vec, U8StringRef file_name)
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






template <typename StorePtr>
void check(StorePtr store, const char* message,  const char* source)
{
    store->check([&](CheckSeverity sv, const hermes::HermesCtr& doc){
        MEMORIA_MAKE_GENERIC_ERROR("Container check failuer: {}", doc.to_pretty_string()).do_throw();
    });
}

template <typename Ctr>
void checkCtr(Ctr& ctr, const char* message,  const char* source)
{
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
        MMA_THROW(TestException()) << WhatCInfo("Code throws unexpected exception");
    }

    if (!throwsException)
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("Code doesn't throw exception {}", TypeNameFactory<Exception>::name()));
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
        MMA_THROW(TestException()) << WhatInfo(format_u8("Code throws exception {}", TypeNameFactory<Exception>::name()));
    }
    catch (...)
    {
        MMA_THROW(TestException()) << WhatCInfo("Code throws unexpected exception");
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
        MMA_THROW(TestException()) << WhatInfo(format_u8("Code throws unexpected exception: {} {}", ex));
    }
    catch (...)
    {
        MMA_THROW(TestException()) << WhatCInfo("Code throws unknown exception");
    }
}

template <typename Op>
void AssertTrue(const char* src, const Op& op, const SBuf& msg)
{
    if (!(op))
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("True assertion failed: {} {}", op, msg.str()));
    }
}

template <typename Op>
void AssertTrue(const char* src, const Op& op)
{
    if (!(op))
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("True assertion failed: {}", op));
    }
}


template <typename Op>
void AssertFalse(const char* src, const Op& op)
{
    if (op)
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("False assertion failed: {}", op));
    }
}

template <typename Op>
void AssertFalse(const char* src, const Op& op, const SBuf& msg)
{
    if (op)
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("False assertion failed: {} {}", op, msg.str()));
    }
}


template <typename Op1, typename Op2>
void AssertEQ(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg)
{
    if (!(op1 == op2))
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("EQ assertion failed: {} {} {}", op1, op2, msg.str()));
    }
}

template <typename Op1, typename Op2>
void AssertEQ(const char* src, const Op1& op1, const Op2& op2, const std::function<SBuf ()> msg_fn)
{
    if (!(op1 == op2))
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("EQ assertion failed: {} {} {}", op1, op2, msg_fn().str()));
    }
}

template <typename Op1, typename Op2>
void AssertEQ(const char* src, const Op1& op1, const Op2& op2)
{
    if (!(op1 == op2))
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("EQ assertion failed: {} {}", op1, op2));
    }
}

template <typename Op1, typename Op2>
void AssertEQ(const char* src, const std::vector<Op1>& op1, const std::vector<Op2>& op2)
{
    if (op1.size() != op2.size())
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("EQ size assertion failed: {} {}", op1.size(), op2.size()));
    }
    else {
        for (size_t c = 0; c < op1.size(); c++)
        {
            if (op1[c] != op2[c])
            {
                MMA_THROW(TestException()) << WhatInfo(format_u8("EQ data assertion failed: {} {} {}", c, op1[c], op2[c]));
            }
        }
    }
}

template <typename Op1, typename Op2>
void AssertSpansEQ(const char* src, Span<const Op1> op1, Span<const Op2> op2)
{
    if (op1.size() != op2.size())
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("EQ size assertion failed: {} {}", op1.size(), op2.size()));
    }
    else {
        for (size_t c = 0; c < op1.size(); c++)
        {
            if (op1[c] != op2[c])
            {
                MMA_THROW(TestException()) << WhatInfo(format_u8("EQ data assertion failed: {} {} {}", c, op1[c], op2[c]));
            }
        }
    }
}

template <typename Op1, typename Op2>
void AssertLT(const char* src, const Op1& op1, const Op2& op2)
{
    if (!(op1 < op2))
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("LT assertion failed: {} {}", op1, op2));
    }
}

template <typename Op1, typename Op2>
void AssertLT(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg)
{
    if (!(op1 < op2))
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("LT assertion failed: {} {} {}", op1, op2, msg.str()));
    }
}

template <typename Op1, typename Op2>
void AssertLE(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg)
{
    if (!(op1 <= op2))
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("LE assertion failed: {} {} {}", op1, op2, msg.str()));
    }
}

template <typename Op1, typename Op2>
void AssertLE(const char* src, const Op1& op1, const Op2& op2)
{
    if (!(op1 <= op2))
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("LE assertion failed: {} {}", op1, op2));
    }
}

template <typename Op1, typename Op2>
void AssertGT(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg = SBuf())
{
    if (!(op1 > op2))
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("GT assertion failed: {} {} {}", op1, op2, msg.str()));
    }
}

template <typename Op1, typename Op2>
void AssertGE(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg = SBuf())
{
    if (!(op1 >= op2))
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("GE assertion failed: {} {} {}", op1, op2, msg.str()));
    }
}

template <typename Op1, typename Op2>
void AssertNEQ(const char* src, const Op1& op1, const Op2& op2)
{
    if (!(op1 != op2))
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("NEQ assertion failed: {} {}", op1, op2));
    }
}


template <typename Op1, typename Op2>
void AssertNEQ(const char* src, const Op1& op1, const Op2& op2, const SBuf& msg)
{
    if (!(op1 != op2))
    {
        MMA_THROW(TestException()) << WhatInfo(format_u8("NEQ assertion failed: {} {} {}", op1, op2, msg.str()));
    }
}




}}
