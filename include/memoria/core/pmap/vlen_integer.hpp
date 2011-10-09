
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CORE_TOOLS_PMAP_VLEN_INTEGER_HPP_
#define _MEMORIA_CORE_TOOLS_PMAP_VLEN_INTEGER_HPP_

#include <memoria/core/pmap/vlen.hpp>

namespace memoria 	{

template <size_t Size> struct AtomTypeSelectorHelper;

/**
 * Consider the case when using UBigInt as borrow type on
 * a 32bit host does not lead to the significant performance drop.
 */
template<>
struct AtomTypeSelectorHelper<(size_t)8> {
	typedef UInt Type;
	typedef UBigInt BiggerType;
	static const BiggerType BORROW = 0x100000000llu;
};

template<>
struct AtomTypeSelectorHelper<(size_t)4> {
	typedef UShort Type;
	typedef Int BiggerType;
	static const BiggerType BORROW = 0x10000;
};

struct AtomTypeSelector: public AtomTypeSelectorHelper<sizeof(void*)> {};

template <typename AtomType, typename BiggerType, BiggerType BORROW>
class VLenIntegerHelper: public VLengthArray<AtomType> {


	typedef VLengthArray<AtomType> Base;
	typedef VLenIntegerHelper<AtomType, BiggerType, BORROW> Me;
	typedef VLenIntegerHelper<const AtomType, BiggerType, BORROW> ConstMe;

public:
	VLenIntegerHelper(UInt length, void* ptr): Base(length, ptr) {}

	const Me& operator=(const Me& other)
	{
		Base::operator=(other);
		return *this;
	}

	const Me& operator=(const ConstMe& other)
	{
		Base::operator=(other);
		return *this;
	}


	void Clear()
	{
		for (Int c = 0; c < Base::length(); c++)
		{
			Base::operator[](c) = 0;
		}
	}

	bool operator > (const Me& other) const
	{
		int c;
		AtomType v0,v1;
		for (c = Base::length() - 1; c >= 0; c--)
		{
			v0 = Base::operator[](c);
			v1 = other[c];
			if (v0 != v1)
			{
				return v0 > v1;
			}
		}

		return false;
	}

	bool operator > (const ConstMe& other) const
	{
		int c;
		AtomType v0,v1;
		for (c = Base::length() - 1; c >= 0; c--)
		{
			v0 = Base::operator[](c);
			v1 = other[c];
			if (v0 != v1)
			{
				return v0 > v1;
			}
		}

		return false;
	}







	bool operator < (const Me& other) const
	{
		int c;
		AtomType v0,v1;
		for (c = Base::length() - 1; c >= 0; c--)
		{
			v0 = Base::operator[](c);
			v1 = other[c];
			if (v0 != v1)
			{
				return v0 < v1;
			}
		}

		return false;
	}

	bool operator < (const ConstMe& other) const
	{
		int c;
		AtomType v0,v1;
		for (c = Base::length() - 1; c >= 0; c--)
		{
			v0 = Base::operator[](c);
			v1 = other[c];
			if (v0 != v1)
			{
				return v0 < v1;
			}
		}

		return false;
	}






	bool operator <= (const Me& other) const
	{
		int c;
		AtomType v0,v1;
		for (c = Base::length() - 1; c >= 0; c--)
		{
			v0 = Base::operator[](c);
			v1 = other[c];
			if (v0 != v1)
			{
				return v0 < v1;
			}
		}
		return true;
	}

	bool operator <= (const ConstMe& other) const
	{
		int c;
		AtomType v0,v1;
		for (c = Base::length() - 1; c >= 0; c--)
		{
			v0 = Base::operator[](c);
			v1 = other[c];
			if (v0 != v1)
			{
				return v0 < v1;
			}
		}
		return true;
	}





	bool operator >= (const Me& other) const
	{
		int c;
		AtomType v0,v1;
		for (c = Base::length() - 1; c >= 0; c--)
		{
			v0 = Base::operator[](c);
			v1 = other[c];
			if (v0 != v1)
			{
				return v0 > v1;
			}
		}

		return true;
	}

	bool operator >= (const ConstMe& other) const
	{
		int c;
		AtomType v0,v1;
		for (c = Base::length() - 1; c >= 0; c--)
		{
			v0 = Base::operator[](c);
			v1 = other[c];
			if (v0 != v1)
			{
				return v0 > v1;
			}
		}

		return true;
	}





	const Me& operator+=(const Me& other)
	{
		BiggerType carry = 0;

		Me& me = *this;

		for (int c = 0; c < Base::length(); c++)
		{
			BiggerType v0 = me[c];
			BiggerType v1 = other[c];
			BiggerType vv = v0 + v1 + carry;
			me[c] = vv;
			carry = vv >> (sizeof(AtomType) * 8);
		}

		return me;
	}

	const Me& operator+=(const ConstMe& other)
	{
		BiggerType carry = 0;

		Me& me = *this;

		for (int c = 0; c < Base::length(); c++)
		{
			BiggerType v0 = me[c];
			BiggerType v1 = other[c];
			BiggerType vv = v0 + v1 + carry;
			me[c] = vv;
			carry = vv >> (sizeof(AtomType) * 8);
		}

		return me;
	}



	const Me& operator-=(const Me& other)
	{
		Me& me = *this;

		BiggerType carry = 0;
		for (int c = 0; c < Base::length(); c++)
		{
			BiggerType v0 = me[c];
			BiggerType v1 = other[c];
			BiggerType vv;
			BiggerType big;

			if (v0 >= v1 + carry)
			{
				vv = v0 - v1 - carry;
				carry = 0;
			}
			else {
				vv = big + v0 - v1 - carry;
				carry = 1;
			}
			me[c] = vv;
		}
		return me;
	}

	const Me& operator-=(const ConstMe& other)
	{
		Me& me = *this;

		BiggerType carry = 0;
		for (int c = 0; c < Base::length(); c++)
		{
			BiggerType v0 = me[c];
			BiggerType v1 = other[c];
			BiggerType vv;
			BiggerType big;

			if (v0 >= v1 + carry)
			{
				vv = v0 - v1 - carry;
				carry = 0;
			}
			else {
				vv = big + v0 - v1 - carry;
				carry = 1;
			}
			me[c] = vv;
		}
		return me;
	}
};








template <typename AtomType, typename BiggerType, BiggerType BORROW>
class VLenIntegerHelper<const AtomType, BiggerType, BORROW>: public VLengthArray<const AtomType> {

	typedef VLengthArray<const AtomType> Base;
	typedef VLenIntegerHelper<const AtomType, BiggerType, BORROW> Me;
	typedef VLenIntegerHelper<AtomType, BiggerType, BORROW> NonConstMe;



public:
	VLenIntegerHelper(UInt length, const void* ptr): Base(length, ptr) {}


	bool operator > (const Me& other) const
	{
		int c;
		AtomType v0,v1;
		for (c = Base::length() - 1; c >= 0; c--)
		{
			v0 = Base::operator[](c);
			v1 = other[c];
			if (v0 != v1)
			{
				return v0 > v1;
			}
		}

		return false;
	}

	bool operator > (const NonConstMe& other) const
	{
		int c;
		AtomType v0,v1;
		for (c = Base::length() - 1; c >= 0; c--)
		{
			v0 = Base::operator[](c);
			v1 = other[c];
			if (v0 != v1)
			{
				return v0 > v1;
			}
		}

		return false;
	}







	bool operator < (const Me& other) const
	{
		int c;
		AtomType v0,v1;
		for (c = Base::length() - 1; c >= 0; c--)
		{
			v0 = Base::operator[](c);
			v1 = other[c];
			if (v0 != v1)
			{
				return v0 < v1;
			}
		}

		return false;
	}

	bool operator < (const NonConstMe& other) const
	{
		int c;
		AtomType v0,v1;
		for (c = Base::length() - 1; c >= 0; c--)
		{
			v0 = Base::operator[](c);
			v1 = other[c];
			if (v0 != v1)
			{
				return v0 < v1;
			}
		}

		return false;
	}







	bool operator <= (const Me& other) const
	{
		int c;
		AtomType v0,v1;
		for (c = Base::length() - 1; c >= 0; c--)
		{
			v0 = Base::operator[](c);
			v1 = other[c];
			if (v0 != v1)
			{
				return v0 < v1;
			}
		}
		return true;
	}

	bool operator <= (const NonConstMe& other) const
	{
		int c;
		AtomType v0,v1;
		for (c = Base::length() - 1; c >= 0; c--)
		{
			v0 = Base::operator[](c);
			v1 = other[c];
			if (v0 != v1)
			{
				return v0 < v1;
			}
		}
		return true;
	}





	bool operator >= (const Me& other) const
	{
		int c;
		AtomType v0,v1;
		for (c = Base::length() - 1; c >= 0; c--)
		{
			v0 = Base::operator[](c);
			v1 = other[c];
			if (v0 != v1)
			{
				return v0 > v1;
			}
		}

		return true;
	}

	bool operator >= (const NonConstMe& other) const
	{
		int c;
		AtomType v0,v1;
		for (c = Base::length() - 1; c >= 0; c--)
		{
			v0 = Base::operator[](c);
			v1 = other[c];
			if (v0 != v1)
			{
				return v0 > v1;
			}
		}

		return true;
	}
};

class VLenInteger: public VLenIntegerHelper<
	AtomTypeSelector::Type,
	AtomTypeSelector::BiggerType,
	AtomTypeSelector::BORROW
> {
	typedef VLenIntegerHelper<
			AtomTypeSelector::Type,
			AtomTypeSelector::BiggerType,
			AtomTypeSelector::BORROW
		> Base;

public:
	VLenInteger(UInt length, void* ptr): Base(length, ptr) {}
};


}



#endif
