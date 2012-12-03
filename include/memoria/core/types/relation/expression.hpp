
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_RELATION_EXPRESSION_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_RELATION_EXPRESSION_HPP

#include <memoria/core/types/typemap.hpp>


namespace memoria    {


enum CompareOps {EQ, GT, GTE, LT, LTE, NE};

template <Int Name_, CompareOps Op_, typename T, T Value_>
struct ValueOp {
    static const Int Name           = Name_;
    typedef T                       Type;
    static const CompareOps Op      = Op_;
    static const T Value            = Value_;
};

struct TrueOp {};

template <Int Name_, typename T>
struct TypeOp {
    static const Int Name           = Name_;
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

template<Int Name, CompareOps Op, typename T, T Value>
struct IsExpression<ValueOp<Name, Op, T, Value> >: ConstValue<bool, true> {};

template<Int Name, typename T>
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


template <typename Metadata, Int Name, CompareOps Op, typename Type, Type ExValue, typename Record>
class Evaluator<Metadata, ValueOp<Name, Op, Type, ExValue>, Record> {

    typedef typename Metadata::template Provider<Name, Record>::Value           ColumnValue;

    static const bool EqVal     =  ColumnValue::Value == ExValue;
    static const bool GTVal     =  ColumnValue::Value >  ExValue;
    static const bool GTEVal    =  ColumnValue::Value >= ExValue;
    static const bool LTVal     =  ColumnValue::Value <  ExValue;
    static const bool LTEVal    =  ColumnValue::Value <= ExValue;
    static const bool NEVal     =  ColumnValue::Value != ExValue;

    static const Int OpIdx = Op;

    typedef typename Select<
                OpIdx,
                TypeList<
                    ConstValue<Int, EqVal>,
                    ConstValue<Int, GTVal>,
                    ConstValue<Int, GTEVal>,
                    ConstValue<Int, LTVal>,
                    ConstValue<Int, LTEVal>,
                    ConstValue<Int, NEVal>
                >
    >::Result                                                                   SelectResult;


public:
    static const bool Value = SelectResult::Value;
};

template <typename Metadata, Int Name, typename Type, typename Record>
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

}

#endif
