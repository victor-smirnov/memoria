
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_MODELS_TYPES_HPP
#define	_MEMORIA_VAPI_MODELS_TYPES_HPP


#include <memoria/core/tools/config.hpp>
#include <memoria/core/types/types.hpp>

#include <vector>

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
    enum {ITER_EOF, ITER_EMPTY, ITER_BOF};
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
public:
	ArrayData(Int length, void* data):length_(length), data_(T2T<UByte*>(data)) {}

	Int size() const {
		return length_;
	}

	const UByte* data() const {
		return data_;
	}

	UByte* data() {
		return data_;
	}
};


}
}



#endif
