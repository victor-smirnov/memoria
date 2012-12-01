/*
 * partial_specialization.cpp
 *
 *  Created on: 30.11.2012
 *      Author: developer
 */

#include <typeinfo>
#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/types/list/append.hpp>
#include <memoria/core/types/algo/select.hpp>

using namespace std;
using namespace memoria;
using namespace memoria::vapi;


template <typename T, T ... Data> struct Array {};

//template <typename... List> struct TypeList;

//template <>
//struct TypeList<> {
//	typedef TypeList<> Type;
//};
//
//template <typename Head, typename ... Tail>
//struct TypeList<Head, Tail...> {
//	typedef TypeList<Head, typename TypeList<Tail...>::Type> Type;
//};



template <Int idx, typename Head, typename ... Tail>
struct Get {
	typedef typename Get<idx - 1, Tail... >::Type Type;
};

template <typename Head, typename ... Tail>
struct Get<0, Head, Tail...> {
	typedef Head Type;
};


template <typename T, Int idx, T Head, T ... Tail>
struct GetValue {
	static const T Value = GetValue<T, idx - 1, Tail... >::Value;
};

template <typename T, T Head, T ... Tail>
struct GetValue<T, 0, Head, Tail...> {
	static const T Value = Head;
};



template <
	typename ArrayType,
	Int idx
>
struct GetArrayElement;

typedef Array<UInt, 23234, 452345, 34545345, 3452345> UArray1;



template <
	typename EType,
	EType ... Data,
	Int idx
>
struct GetArrayElement<Array<EType, Data...>, idx> {
	static const EType Value = GetValue<EType, idx, Data...>::Value;
};


namespace md5 {

template <UInt X, UInt Y, UInt Z>
struct FunF {
	static const UInt Value = (X & Y) | ((~X) & Z);
};

template <UInt X, UInt Y, UInt Z>
struct FunG {
	static const UInt Value = (X & Z) | ((~Z) & Y);
};

template <UInt X, UInt Y, UInt Z>
struct FunH {
	static const UInt Value = (X ^ Y ^ Z);
};

template <UInt X, UInt Y, UInt Z>
struct FunI {
	static const UInt Value = Y ^ ((~Z) | X);
};

typedef Array<UInt,
		0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
		0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
		0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
		0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
		0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
		0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
		0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
		0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
		0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
		0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
		0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
		0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
		0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
		0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
		0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
		0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391

		> Md5Inits;

template <UInt Value_, Int N> struct CShl {
	static const UInt Value = (Value_ << N) |
			(Value_ >> (sizeof(UInt) * 8 - N));
};


template <UInt A_, UInt B_, UInt C_, UInt D_>
struct Quad {
	static const UInt A = A_;
	static const UInt B = B_;
	static const UInt C = C_;
	static const UInt D = D_;

	static const UBigInt Value = (((UBigInt)A << 32) | B) ^ (((UBigInt)C << 32) | D);
	static const UInt 	 Value32 = A ^ B ^ C ^ D;
};


template <
	typename Q,
	template <UInt, UInt, UInt> class Fun,
	typename X,
	Int K, Int S, Int I
>
class Block;


template <
	UInt A, UInt B, UInt C, UInt D,
	template <UInt, UInt, UInt> class Fun,
	typename X,
	Int K, Int S, Int I
>
class Block<Quad<A, B, C, D>, Fun, X, K, S, I> {

	static const UInt ValueB = B
			+ CShl<A + Fun<B, C, D>::Value
					 + GetArrayElement<X, K>::Value
					 + GetArrayElement<Md5Inits, I - 1>::Value, S
				  >::Value;

public:
	typedef Quad<D, ValueB, B, A> Result;
};

typedef Quad<0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476> Q0;



template <typename R0, typename X>
class Round1 {
	typedef typename Block<R0, FunF, X, 0,  7, 1>::Result 		R1;
	typedef typename Block<R1, FunF, X, 1, 12, 2>::Result 		R2;
	typedef typename Block<R2, FunF, X, 2, 17, 3>::Result 		R3;
	typedef typename Block<R3, FunF, X, 3, 22, 4>::Result 		R4;

	typedef typename Block<R4, FunF, X, 4,  7, 5>::Result 		R5;
	typedef typename Block<R5, FunF, X, 5, 12, 6>::Result 		R6;
	typedef typename Block<R6, FunF, X, 6, 17, 7>::Result 		R7;
	typedef typename Block<R7, FunF, X, 7, 22, 8>::Result 		R8;

	typedef typename Block<R8,  FunF, X,  8,  7,  9>::Result 	R9;
	typedef typename Block<R9,  FunF, X,  9, 12, 10>::Result 	R10;
	typedef typename Block<R10, FunF, X, 10, 17, 11>::Result 	R11;
	typedef typename Block<R11, FunF, X, 11, 22, 12>::Result 	R12;

	typedef typename Block<R12, FunF, X, 12,  7, 13>::Result 	R13;
	typedef typename Block<R13, FunF, X, 13, 12, 14>::Result 	R14;
	typedef typename Block<R14, FunF, X, 14, 17, 15>::Result 	R15;
	typedef typename Block<R15, FunF, X, 15, 22, 16>::Result 	R16;

public:
	typedef Quad<R0::A + R16::A, R0::B + R16::B, R0::C + R16::C, R0::D + R16::D> 	Result;
};



/*[abcd k s i] a = b + ((a + G(b,c,d) + X[k] + T[i]) <<< s). */
//[ABCD  1 5 17][DABC  6 9 18][CDAB 11 14 19][BCDA  0 20 20]
//[ABCD  5 5 21][DABC 10 9 22][CDAB 15 14 23][BCDA  4 20 24]
//[ABCD  9 5 25][DABC 14 9 26][CDAB  3 14 27][BCDA  8 20 28]
//[ABCD 13 5 29][DABC  2 9 30][CDAB  7 14 31][BCDA 12 20 32]

template <typename R0, typename X>
class Round2 {
	typedef typename Block<R0, FunG, X,  1,  5, 17>::Result 	R1;
	typedef typename Block<R1, FunG, X,  6,  9, 18>::Result 	R2;
	typedef typename Block<R2, FunG, X, 11, 14, 19>::Result 	R3;
	typedef typename Block<R3, FunG, X,  0, 20, 20>::Result 	R4;

	typedef typename Block<R4, FunG, X,  5,  5, 21>::Result 	R5;
	typedef typename Block<R5, FunG, X, 10,  9, 22>::Result 	R6;
	typedef typename Block<R6, FunG, X, 15, 14, 23>::Result 	R7;
	typedef typename Block<R7, FunG, X,  4, 20, 24>::Result 	R8;

	typedef typename Block<R8,  FunG, X,  9,  5, 25>::Result 	R9;
	typedef typename Block<R9,  FunG, X, 14,  9, 26>::Result 	R10;
	typedef typename Block<R10, FunG, X,  3, 14, 27>::Result 	R11;
	typedef typename Block<R11, FunG, X,  8, 20, 28>::Result 	R12;

	typedef typename Block<R12, FunG, X, 13,  5, 29>::Result 	R13;
	typedef typename Block<R13, FunG, X,  2,  9, 30>::Result 	R14;
	typedef typename Block<R14, FunG, X,  7, 14, 31>::Result 	R15;
	typedef typename Block<R15, FunG, X, 12, 20, 32>::Result 	R16;

public:
	typedef Quad<R0::A + R16::A, R0::B + R16::B, R0::C + R16::C, R0::D + R16::D> 	Result;
};


template <typename R0, typename X>
class Round3 {
	typedef typename Block<R0, FunH, X,  5,  4, 33>::Result 	R1;
	typedef typename Block<R1, FunH, X,  8, 11, 34>::Result 	R2;
	typedef typename Block<R2, FunH, X, 11, 16, 35>::Result 	R3;
	typedef typename Block<R3, FunH, X, 14, 23, 36>::Result 	R4;

	typedef typename Block<R4, FunH, X,  1,  4, 37>::Result 	R5;
	typedef typename Block<R5, FunH, X,  4, 11, 38>::Result 	R6;
	typedef typename Block<R6, FunH, X,  7, 16, 39>::Result 	R7;
	typedef typename Block<R7, FunH, X, 10, 23, 40>::Result 	R8;

	typedef typename Block<R8,  FunH, X,  3,  4, 41>::Result 	R9;
	typedef typename Block<R9,  FunH, X,  0, 11, 42>::Result 	R10;
	typedef typename Block<R10, FunH, X,  3, 16, 43>::Result 	R11;
	typedef typename Block<R11, FunH, X,  6, 23, 44>::Result 	R12;

	typedef typename Block<R12, FunH, X,  9,  4, 45>::Result 	R13;
	typedef typename Block<R13, FunH, X, 12, 11, 46>::Result 	R14;
	typedef typename Block<R14, FunH, X, 15, 16, 47>::Result 	R15;
	typedef typename Block<R15, FunH, X,  2, 23, 48>::Result 	R16;

public:
	typedef Quad<R0::A + R16::A, R0::B + R16::B, R0::C + R16::C, R0::D + R16::D> 	Result;
};

template <typename R0, typename X>
class Round4 {
	typedef typename Block<R0, FunH, X,  0,  6, 49>::Result 	R1;
	typedef typename Block<R1, FunH, X,  7, 10, 50>::Result 	R2;
	typedef typename Block<R2, FunH, X, 14, 15, 51>::Result 	R3;
	typedef typename Block<R3, FunH, X,  5, 21, 52>::Result 	R4;

	typedef typename Block<R4, FunH, X, 12,  6, 53>::Result 	R5;
	typedef typename Block<R5, FunH, X,  3, 10, 54>::Result 	R6;
	typedef typename Block<R6, FunH, X, 10, 15, 55>::Result 	R7;
	typedef typename Block<R7, FunH, X,  1, 21, 56>::Result 	R8;

	typedef typename Block<R8,  FunH, X,  8,  6, 57>::Result 	R9;
	typedef typename Block<R9,  FunH, X, 15, 10, 58>::Result 	R10;
	typedef typename Block<R10, FunH, X,  6, 15, 59>::Result 	R11;
	typedef typename Block<R11, FunH, X, 13, 21, 60>::Result 	R12;

	typedef typename Block<R12, FunH, X,  4,  6, 61>::Result 	R13;
	typedef typename Block<R13, FunH, X, 11, 10, 62>::Result 	R14;
	typedef typename Block<R14, FunH, X,  2, 15, 63>::Result 	R15;
	typedef typename Block<R15, FunH, X,  9, 21, 64>::Result 	R16;

public:
	typedef Quad<R0::A + R16::A, R0::B + R16::B, R0::C + R16::C, R0::D + R16::D> 	Result;
};

template <typename Q0, typename X>
class Md5Round {
	typedef typename Round1<Q0, X>::Result R1;
	typedef typename Round2<R1, X>::Result R2;
	typedef typename Round3<R2, X>::Result R3;
	typedef typename Round4<R3, X>::Result R4;

public:
	typedef R4 Result;
};



template <UInt ... List>
class MD5Sum {
	static const Int ListSize = sizeof...(List);

public:

};


}


int main(void) {

	cout<<TypeNameFactory<TypeList<int,long,float,double>>::name()<<endl;

	cout<<TypeNameFactory<Get<3, int,long,float,double>::Type>::name()<<endl;

	cout<<GetArrayElement<md5::Md5Inits, 63>::Value<<endl;

	cout<<md5::Md5Round<md5::Q0, Array<UInt, 1,3,7,2, 1,3,7,2, 1,3,7,2, 1,3,7,2> >::Result::Value<<endl;

	return 0;
}

