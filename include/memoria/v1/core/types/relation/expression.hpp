
// Copyright 2011 Victor Smirnov
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

#include <memoria/v1/core/types/typemap.hpp>


namespace memoria {
namespace v1 {


enum CompareOps {EQ, GT, GTE, LT, LTE, NE};

template <int32_t Name_, CompareOps Op_, typename T, T Value_>
struct ValueOp {
    static const int32_t Name           = Name_;
    typedef T                       Type;
    static const CompareOps Op      = Op_;
    static const T Value            = Value_;
};

struct TrueOp {};

template <int32_t Name_, typename T>
struct TypeOp {
    static const int32_t Name           = Name_;
    typedef T                       Type;
};

template <typename T1, typename T2>
struct And {
    typedef T1 Type1;
    typedef T2 Type2;
};

template <typename T1, typename T2>
struct Or {
    typedef T1 Type1;
    typedef T2 Type2;
};

template <typename T1, typename T2>
struct Xor {
    typedef T1 Type1;
    typedef T2 Type2;
};

template <typename T>
struct Not {
    typedef T Type;
};



template<typename Type>
struct IsExpression: ConstValue<bool, false> {};

template<typename T1, typename T2>
struct IsExpression<And<T1, T2> >: ConstValue<bool, true> {};

template<typename T1, typename T2>
struct IsExpression<Or<T1, T2> >: ConstValue<bool, true> {};

template<typename T1, typename T2>
struct IsExpression<Xor<T1, T2> >: ConstValue<bool, true> {};

template<typename T>
struct IsExpression<Not<T> >: ConstValue<bool, true> {};

template<int32_t Name, CompareOps Op, typename T, T Value>
struct IsExpression<ValueOp<Name, Op, T, Value> >: ConstValue<bool, true> {};

template<int32_t Name, typename T>
struct IsExpression<TypeOp<Name, T> >: ConstValue<bool, true> {};

template<>
struct IsExpression<TrueOp>: ConstValue<bool, true> {};






template <typename Metadata, typename Expr, typename Record> class Evaluator;

template <typename Metadata, typename Type1, typename Type2, typename Record>
class Evaluator<Metadata, And<Type1, Type2>, Record> {
public:
    static const bool Value =   Evaluator<Metadata, Type1, Record>::Value &&
                                Evaluator<Metadata, Type2, Record>::Value;
};


template <typename Metadata, typename Type1, typename Type2, typename Record>
class Evaluator<Metadata, Or<Type1, Type2>, Record> {
public:
    static const bool Value =   Evaluator<Metadata, Type1, Record>::Value ||
                                Evaluator<Metadata, Type2, Record>::Value;
};

template <typename Metadata, typename Type1, typename Type2, typename Record>
class Evaluator<Metadata, Xor<Type1, Type2>, Record> {
public:
    static const bool Value =   Evaluator<Metadata, Type1, Record>::Value ^
                                Evaluator<Metadata, Type2, Record>::Value;
};

template <typename Metadata, typename Type, typename Record>
class Evaluator<Metadata, Not<Type>, Record> {
public:
    static const bool Value =  !Evaluator<Metadata, Type, Record>::Value;
};


template <typename Metadata, int32_t Name, CompareOps Op, typename Type, Type ExValue, typename Record>
class Evaluator<Metadata, ValueOp<Name, Op, Type, ExValue>, Record> {

    typedef typename Metadata::template Provider<Name, Record>::Value           ColumnValue;

    static const bool EqVal     =  ColumnValue::Value == ExValue;
    static const bool GTVal     =  ColumnValue::Value >  ExValue;
    static const bool GTEVal    =  ColumnValue::Value >= ExValue;
    static const bool LTVal     =  ColumnValue::Value <  ExValue;
    static const bool LTEVal    =  ColumnValue::Value <= ExValue;
    static const bool NEVal     =  ColumnValue::Value != ExValue;

    static const int32_t OpIdx = Op;

    using SelectResult = Select<
                OpIdx,
                TypeList<
                    ConstValue<int32_t, EqVal>,
                    ConstValue<int32_t, GTVal>,
                    ConstValue<int32_t, GTEVal>,
                    ConstValue<int32_t, LTVal>,
                    ConstValue<int32_t, LTEVal>,
                    ConstValue<int32_t, NEVal>
                >
    >;


public:
    static const bool Value = SelectResult::Value;
};

template <typename Metadata, int32_t Name, typename Type, typename Record>
class Evaluator<Metadata, TypeOp<Name, Type>, Record> {

    typedef typename Metadata::template Provider<Name, Record>::Value::Value    ColumnValue;
    
public:
    static const bool Value = IfTypesEqual<Type, ColumnValue>::Value;
};

template <typename Metadata, typename Record>
class Evaluator<Metadata, TrueOp, Record> {
public:
    static const bool Value = true;
};

}}