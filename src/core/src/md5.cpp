
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/metadata/tools.hpp>
#include <memoria/core/tools/array_data.hpp>
#include <memoria/core/tools/md5.hpp>

#include <iostream>

namespace memoria {

using namespace std;



void MD5Hash::add(UInt value)
{
	if (ptr_ == BUFFER_SIZE) {
		compute();
		ptr_ = 0;
		for (UInt& value: K_) value = 0;
	}

	K_[ptr_++] = value;
	dirty_ = true;
}

void MD5Hash::compute()
{
	if (dirty_)
	{
		Quad tmp = Q0_;

		round1();
		round2();
		round3();
		round4();

		Q0_.add(tmp);

		dirty_ = false;
	}
}




void MD5Hash::round1()
{
	Block(FunF, 0,  7, 1);
	Block(FunF, 1, 12, 2);
	Block(FunF, 2, 17, 3);
	Block(FunF, 3, 22, 4);

	Block(FunF, 4,  7, 5);
	Block(FunF, 5, 12, 6);
	Block(FunF, 6, 17, 7);
	Block(FunF, 7, 22, 8);

	Block(FunF,  8,  7,  9);
	Block(FunF,  9, 12, 10);
	Block(FunF, 10, 17, 11);
	Block(FunF, 11, 22, 12);

	Block(FunF, 12,  7, 13);
	Block(FunF, 13, 12, 14);
	Block(FunF, 14, 17, 15);
	Block(FunF, 15, 22, 16);
}

void MD5Hash::round2()
{
	Block(FunG,  1,  5, 17);
	Block(FunG,  6,  9, 18);
	Block(FunG, 11, 14, 19);
	Block(FunG,  0, 20, 20);

	Block(FunG,  5,  5, 21);
	Block(FunG, 10,  9, 22);
	Block(FunG, 15, 14, 23);
	Block(FunG,  4, 20, 24);

	Block(FunG,  9,  5, 25);
	Block(FunG, 14,  9, 26);
	Block(FunG,  3, 14, 27);
	Block(FunG,  8, 20, 28);

	Block(FunG, 13,  5, 29);
	Block(FunG,  2,  9, 30);
	Block(FunG,  7, 14, 31);
	Block(FunG, 12, 20, 32);
}

void MD5Hash::round3()
{
	Block(FunH,  5,  4, 33);
	Block(FunH,  8, 11, 34);
	Block(FunH, 11, 16, 35);
	Block(FunH, 14, 23, 36);

	Block(FunH,  1,  4, 37);
	Block(FunH,  4, 11, 38);
	Block(FunH,  7, 16, 39);
	Block(FunH, 10, 23, 40);

	Block(FunH,  3,  4, 41);
	Block(FunH,  0, 11, 42);
	Block(FunH,  3, 16, 43);
	Block(FunH,  6, 23, 44);

	Block(FunH,  9,  4, 45);
	Block(FunH, 12, 11, 46);
	Block(FunH, 15, 16, 47);
	Block(FunH,  2, 23, 48);
}

void MD5Hash::round4()
{
	Block(FunI,  0,  6, 49);
	Block(FunI,  7, 10, 50);
	Block(FunI, 14, 15, 51);
	Block(FunI,  5, 21, 52);

	Block(FunI, 12,  6, 53);
	Block(FunI,  3, 10, 54);
	Block(FunI, 10, 15, 55);
	Block(FunI,  1, 21, 56);

	Block(FunI,  8,  6, 57);
	Block(FunI, 15, 10, 58);
	Block(FunI,  6, 15, 59);
	Block(FunI, 13, 21, 60);

	Block(FunI,  4,  6, 61);
	Block(FunI, 11, 10, 62);
	Block(FunI,  2, 15, 63);
	Block(FunI,  9, 21, 64);
}

void MD5Hash::Block(FunPtr Fun, Int k, Int s, Int i)
{
	UInt ValueB = Q0_.B_ + CShl(Q0_.A_ + Fun(Q0_.B_, Q0_.C_, Q0_.D_) + getBufferValue(k) + getMd5Constant(i), s);

	Q0_ = Quad(Q0_.D_, ValueB, Q0_.B_, Q0_.C_);
}

void MD5Hash::dumpState() {
	cout<<Q0_.A_<<" "<<Q0_.B_<<" "<<Q0_.C_<<" "<<Q0_.D_<<endl;
}





const UInt MD5Hash::X[64] = {
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
};


}
