
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_FSE_ARRAYHPP_
#define MEMORIA_CORE_PACKED_FSE_ARRAYHPP_

#include <memoria/core/packed2/packed_allocator_types.hpp>
#include <memoria/core/tools/accessors.hpp>

namespace memoria {



template <
	typename V,
	typename Allocator_ = PackedAllocator
>
struct PackedFSEArrayTypes {
    typedef V               Value;
    typedef Allocator_  	Allocator;
};



template <typename Types_>
class PackedFSEArray: public PackedAllocatable {

	typedef PackedAllocatable													Base;

public:
	static const UInt VERSION               									= 1;

	typedef Types_																Types;
	typedef PackedFSEArray<Types>               								MyType;

	typedef typename Types::Allocator											Allocator;
	typedef typename Types::Value												Value;


private:

	Int size_;
	Int max_size_;

	Value buffer_[];

public:
	PackedFSEArray() {}

	void setAllocatorOffset(const void* allocator)
	{
		const char* my_ptr = T2T<const char*>(this);
		const char* alc_ptr = T2T<const char*>(allocator);
		size_t diff = T2T<size_t>(my_ptr - alc_ptr);
		Base::allocator_offset() = diff;
	}

	Int& size() {return size_;}
	const Int& size() const {return size_;}

public:
	void init(Int block_size)
	{
		size_ = 0;
		max_size_   = block_size / sizeof(Value);
	}


	void initSizes(Int max)
	{
		size_       = 0;
		max_size_   = max;
	}

	Value& operator[](Int idx) {
		return buffer_[idx];
	}

	const Value& operator[](Int idx) const {
		return buffer_[idx];
	}

	Value& value(Int idx) {
		return buffer_[idx];
	}

	const Value& value(Int idx) const {
		return buffer_[idx];
	}

	Value* data() {
		return buffer_;
	}

	const Value* data() const {
		return buffer_;
	}

	Value* values() {
		return buffer_;
	}

	const Value* values() const {
		return buffer_;
	}

	// ==================================== Dump =========================================== //


	void dump(std::ostream& out = cout) const
	{
		out<<"size_       = "<<size_<<endl;
		out<<"max_size_   = "<<max_size_<<endl;
		out<<endl;

		out<<"Data:"<<endl;

		const Value* values_ = buffer_;

		dumpArray<Value>(out, size_, [&](Int pos) -> Value {
			return values_[pos];
		});
	}
};


}


#endif