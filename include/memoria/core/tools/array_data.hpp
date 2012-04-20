
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_ARRAY_DATA_HPP
#define	_MEMORIA_CORE_TOOLS_ARRAY_DATA_HPP


#include <memoria/core/tools/config.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>

#include <vector>

#include <malloc.h>

namespace memoria    {
namespace vapi       {


class ArrayData {
	Int length_;
	UByte* data_;
	bool owner_;
public:
	ArrayData(Int length, void* data, bool owner = false):length_(length), data_(T2T<UByte*>(data)), owner_(owner) {}
	ArrayData(Int length):length_(length), data_(T2T<UByte*>(::malloc(length))), owner_(true) {}

	ArrayData(ArrayData&& other):length_(other.length_), data_(other.data_), owner_(other.owner_)
	{
		other.data_ = NULL;
	}

	ArrayData(const ArrayData& other, bool clone = true):length_(other.length_), owner_(true)
	{
		data_ = (UByte*) ::malloc(length_);

		if (clone)
		{
			CopyBuffer(other.data(), data_, length_);
		}
	}

	~ArrayData() {
		if (owner_) ::free(data_);
	}

	Int size() const {
		return length_;
	}

	const UByte* data() const {
		return data_;
	}

	UByte* data() {
		return data_;
	}

	void Dump(std::ostream& out);
};


}
}



#endif
