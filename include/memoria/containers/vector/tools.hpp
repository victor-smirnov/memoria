
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ARRAY_TOOLS_HPP
#define _MEMORIA_MODELS_ARRAY_TOOLS_HPP

namespace memoria    {


template <typename Iter, typename T>
class IDataAdater: public IData<T> {
	Iter iter_;
	BigInt length_;
public:
	IDataAdater(const Iter& iter, BigInt length): iter_(iter), length_(length) {}


	virtual SizeT getSize() const {
		return length_;
	}

	virtual void  setSize(SizeT size) {}

	virtual SizeT put(const T* buffer, SizeT start, SizeT length) {return 0;}

	virtual SizeT get(T* buffer, SizeT start, SizeT length) const
	{
		return length;
	}
};



//namespace models     {
//namespace array      {
//
//struct CountData {
//
//};
//
//template <typename CountData, Int Indexes = 1>
//class BufferContentDescriptor {
//
//    CountData base_prefix_;
//    BigInt length_;
//    BigInt offset_;
//    BigInt start_;
//
//    BigInt indexes_[Indexes];
//
//public:
//
//    BufferContentDescriptor(): length_(0), offset_(0), start_(0)
//    {
//        for (Int c = 0; c < Indexes; c++)
//        {
//            indexes_[c] = 0;
//        }
//    }
//
//    BigInt* indexes() {
//        return indexes_;
//    }
//
//    const BigInt* indexes() const {
//        return indexes_;
//    }
//
//    const CountData& base_prefix() const {
//        return base_prefix_;
//    }
//
//    CountData& base_prefix() {
//        return base_prefix_;
//    }
//
//    const BigInt& length() const {
//        return length_;
//    }
//
//    BigInt& length() {
//        return length_;
//    }
//
//    const BigInt& offset() const {
//        return offset_;
//    }
//
//    BigInt& offset() {
//        return offset_;
//    }
//
//    const BigInt& start() const {
//        return start_;
//    }
//
//    BigInt& start() {
//        return start_;
//    }
//};
//
//
//
//}
//}
}

#endif
