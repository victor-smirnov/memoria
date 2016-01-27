
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_UUID_H
#define _MEMORIA_CORE_TOOLS_UUID_H


#include <memoria/core/types/types.hpp>

#include <iostream>


namespace memoria    {

class UUID {
	UBigInt hi_;
	UBigInt lo_;
public:
	UUID(): hi_(0), lo_(0) {}

	UBigInt hi() const {
		return hi_;
	}

	UBigInt& hi() {
		return hi_;
	}

	UBigInt lo() const {
		return lo_;
	}

	UBigInt& lo() {
		return lo_;
	}

	bool is_null() const {
		return lo_ == 0 && hi_ == 0;
	}

	bool operator==(const UUID& uuid) const {
		return hi_ == uuid.hi_ && lo_ == uuid.lo_;
	}

	static UUID make_random();
	static UUID parse(const char* in);
};

std::ostream& operator<<(std::ostream& out, const UUID& uuid);
std::istream& operator>>(std::istream& in, UUID& uuid);


}



#endif
