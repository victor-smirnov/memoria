
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_TOOLS_ANY_HPP_
#define MEMORIA_CORE_TOOLS_ANY_HPP_

#include <typeinfo>

namespace memoria {

template <typename> struct Any2Type;

namespace vapi {

const char* ExtractMemoriaPath(const char*);

}

class AnyCastException {
	const char* source_;
	const char* type_name_;
	Int 		any_type_;
public:
	AnyCastException(const char* source, const char* name, Int any_type):source_(source), type_name_(name), any_type_(any_type) {}

	const char* Source() const {
		return source_;
	}

	const char* TypeName() const {
		return type_name_;
	}

	Int AnyType() const {
		return any_type_;
	}
};


struct Any {
	enum Type {UNDEFINED, BYTE, UBYTE, SHORT, USHORT, INT, UINT, BIGINT, UBIGINT, FLOAT, DOUBLE, LDOUBLE, PCHAR};
private:

	Type type_;

	union {
		Byte 		b_value;
		UByte 		ub_value;
		Short 		s_value;
		UShort 		us_value;
		Int			i_value;
		UInt		ui_value;
		BigInt		bi_value;
		UBigInt		ubi_value;
		float		f_value;
		double		d_value;
		long double	ld_value;

		const char*	pc_value;



		operator Byte() const {
			return b_value;
		}
		operator UByte() const {
			return ub_value;
		}

		operator Short() const {
			return s_value;
		}
		operator UShort() const {
			return us_value;
		}

		operator Int() const {
			return i_value;
		}
		operator UInt() const {
			return ui_value;
		}

		operator BigInt() const {
			return bi_value;
		}
		operator UBigInt() const {
			return ubi_value;
		}

		operator float() const {
			return f_value;
		}
		operator double() const {
			return d_value;
		}
		operator long double() const {
			return ld_value;
		}

		operator const char*() const {
			return pc_value;
		}

		void operator=(Byte value) {
			b_value = value;
		}
		void operator=(UByte value) {
			ub_value = value;
		}

		void operator=(Short value) {
			s_value = value;
		}
		void operator=(UShort value) {
			us_value = value;
		}

		void operator=(Int value) {
			i_value = value;
		}
		void operator=(UInt value) {
			ui_value = value;
		}

		void operator=(BigInt value) {
			bi_value = value;
		}
		void operator=(UBigInt value) {
			ubi_value = value;
		}

		void operator=(float value) {
			f_value = value;
		}

		void operator=(double value) {
			d_value = value;
		}

		void operator=(long double value) {
			ld_value = value;
		}

		void operator=(const char* value) {
			pc_value = value;
		}

	} value_;

public:



	Any(): type_(UNDEFINED) {}

	Any(Byte value): type_(BYTE) {value_.b_value = value;}
	Any(UByte value): type_(UBYTE) {value_.ub_value = value;}
	Any(Short value): type_(SHORT) {value_.s_value = value;}
	Any(UShort value): type_(USHORT) {value_.us_value = value;}
	Any(Int value): type_(INT) {value_.i_value = value;}
	Any(UInt value): type_(UINT) {value_.ui_value = value;}
	Any(BigInt value): type_(BIGINT) {value_.bi_value = value;}
	Any(UBigInt value): type_(UBIGINT) {value_.ubi_value = value;}
	Any(float value): type_(FLOAT) {value_.f_value = value;}
	Any(double value): type_(DOUBLE) {value_.d_value = value;}
	Any(long double value): type_(LDOUBLE) {value_.ld_value = value;}
	Any(const char* value): type_(PCHAR) {value_.pc_value = value;}

	template <typename T>
	operator T() const
	{
		if (type_ == Any2Type<T>::Value)
		{
			return (T)value_;
		}
		else {
			throw AnyCastException(MEMORIA_SOURCE, typeid(T).name(), type_);
		}
	}

	template <typename T>
	Any& operator=(const T& value)
	{
		type_ 	= (Type)Any2Type<T>::Value;
		value_	= value;

		return *this;
	}

	Type type() const {
		return type_;
	}
};



template<> struct Any2Type<Byte>: TypeCode<Any::BYTE> 		{};
template<> struct Any2Type<UByte>: TypeCode<Any::UBYTE> 	{};

template<> struct Any2Type<Short>: TypeCode<Any::SHORT> 	{};
template<> struct Any2Type<UShort>: TypeCode<Any::USHORT> 	{};

template<> struct Any2Type<Int>: TypeCode<Any::INT> 		{};
template<> struct Any2Type<UInt>: TypeCode<Any::UINT> 		{};

template<> struct Any2Type<BigInt>: TypeCode<Any::BIGINT> 	{};
template<> struct Any2Type<UBigInt>: TypeCode<Any::UBIGINT> {};

template<> struct Any2Type<float>: TypeCode<Any::FLOAT> {};
template<> struct Any2Type<double>: TypeCode<Any::DOUBLE> {};
template<> struct Any2Type<long double>: TypeCode<Any::LDOUBLE> {};

template<> struct Any2Type<const char*>: TypeCode<Any::PCHAR> {};

}

namespace std {

using namespace memoria;

ostream& operator<<(ostream& out, const Any& any);

}


#endif

