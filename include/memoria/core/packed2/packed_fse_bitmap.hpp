
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_FSE_BITMAP_HPP_
#define MEMORIA_CORE_PACKED_FSE_BITMAP_HPP_

#include <memoria/core/packed2/packed_allocator.hpp>
#include <memoria/core/tools/accessors.hpp>

namespace memoria {



template <
	Int BitsPerSymbol_,
	typename V = UBigInt,
	typename Allocator_ = EmptyAllocator
>
struct PackedFSEBitmapTypes {

	static const Int 		BitsPerSymbol			= BitsPerSymbol_;

	typedef V               Value;
    typedef Allocator_  	Allocator;
};



template <typename Types_>
class PackedFSEBitmap: public PackedAllocatable {

	typedef PackedAllocatable													Base;

public:
	static const UInt VERSION               									= 1;

	typedef Types_																Types;
	typedef PackedFSESequence<Types>               								MyType;

	typedef typename Types::Allocator											Allocator;
	typedef typename Types::Value												Value;

	static const Int BitsPerSymbol												= Types::BitsPerSymbol;

private:

	Int size_;
	Int max_size_;

	Value buffer_[];

public:
	PackedFSEBitmap() {}

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
	void initByBlock(Int block_size)
	{
		size_ = 0;
		max_size_   = block_size/sizeof(Value);
	}

	void initSizes(Int max)
	{
		size_       = 0;
		max_size_   = max;
	}

	BitmapAccessor<Value*, Value, BitsPerSymbol>
	operator[](Int idx) {
		return BitmapAccessor<Value*, Value, BitsPerSymbol>(buffer_, idx);
	}

	BitmapAccessor<const Value*, Value, BitsPerSymbol>
	operator[](Int idx) const {
		return BitmapAccessor<const Value*, Value, BitsPerSymbol>(buffer_, idx);
	}

	Value* data() {
		return buffer_;
	}

	const Value* data() const {
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

		dumpSequence<Value>(out, size_, BitsPerSymbol, [&](Int pos) -> Value {
			return values_[pos];
		});
	}
};


}


#endif
