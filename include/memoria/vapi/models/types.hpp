
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_MODELS_TYPES_HPP
#define	_MEMORIA_VAPI_MODELS_TYPES_HPP


#include <memoria/core/tools/config.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>

#include <vector>

#include <malloc.h>

namespace memoria    {
namespace vapi       {

template <typename TargetType, typename IDType>
struct ConvertToHelper {
	static BigInt cvt(TargetType type) {
		return (BigInt)type;
	}
};

template <typename IDType>
struct ConvertToHelper<IDType, IDType> {
	static BigInt cvt(IDType type) {
		return (BigInt)type.value();
	}
};

template <typename TargetType, typename IDType>
struct ConvertFromHelper {
	static TargetType cvt(BigInt type) {
		return type;
	}
};

template <typename IDType>
struct ConvertFromHelper<IDType, IDType> {
	static IDType cvt(BigInt type) {
		return IDType(type);
	}
};



struct MEMORIA_API Iterator {
    enum {ITEREND, ITER_EMPTY, ITER_START};
};


struct MEMORIA_API IdxMap {
	enum {LT, LE};
};





struct MEMORIA_API Data {
	virtual Int GetByteSize() const = 0;
};




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
