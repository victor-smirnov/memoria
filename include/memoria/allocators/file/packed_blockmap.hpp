
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_ALLOCATORS_FILE_PACKEDBLOCKMAP_HPP
#define _MEMORIA_ALLOCATORS_FILE_PACKEDBLOCKMAP_HPP

#include <memoria/core/container/profile.hpp>
#include <memoria/core/container/init.hpp>
#include <memoria/core/exceptions/memoria.hpp>

#include <memoria/core/packed/tools/packed_allocator.hpp>

#include <memoria/core/tools/bitmap_select.hpp>

#include <fstream>
#include <malloc.h>
#include <string.h>
#include <cstddef>

namespace memoria {

template <typename ID>
class PackedBlockMap: public PackedAllocator {
	typedef PackedAllocator														Base;
public:

	class Metadata {
		UBigInt start_position_;
		UBigInt length_;
		UBigInt allocated_;

		Int 	version_;

	public:
		UBigInt& start_position() {return start_position_;}
		const UBigInt& start_position() const {return start_position_;}

		UBigInt& length() {return length_;}
		const UBigInt& length() const {return length_;}

		UBigInt& allocated() {return allocated_;}
		const UBigInt& allocated() const {return allocated_;}

		Int& version() {return version_;}
		const Int& version() const {return version_;}

		void reset()
		{
			start_position_ = 0;
			length_			= 0;
			allocated_		= 0;
			version_		= 0;
		}
	};

	class IDMapEntry {
		ID 		id_;
		UBigInt position_;
	public:

		IDMapEntry() = default;
		IDMapEntry(ID id, UBigInt position): id_(id), position_(position) {}

		ID& id() {return id_;}
		const ID& id() const {return id_;}

		UBigInt& position() {return position_;}
		const UBigInt& position() const {return position_;}
	};

	enum {METADATA, BITMAP, IDMAP};

	PackedBlockMap() = default;

	static Int empty_size()
	{
		Int allocator_size 	= Base::empty_size(3);
		Int content_size 	= Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

		return allocator_size + content_size;
	}

	void init()
	{
		Base::init(3, empty_size());

		Base::allocate<Metadata>(METADATA)->reset();

		Base::allocateArrayByLength<UBigInt>(BITMAP, 0);
		Base::allocateArrayByLength<IDMapEntry>(IDMAP, 0);
	}

	Metadata* metadata() {
		return Base::template get<Metadata>(METADATA);
	}

	const Metadata* metadata() const {
		return Base::template get<Metadata>(METADATA);
	}

	UBigInt* bitmap() {
		return Base::template get<UBigInt>(BITMAP);
	}

	const UBigInt* bitmap() const {
		return Base::template get<UBigInt>(BITMAP);
	}

	IDMapEntry* entries() {
		return Base::template get<IDMapEntry>(IDMAP);
	}

	const IDMapEntry* entries() const {
		return Base::template get<IDMapEntry>(IDMAP);
	}

	void clear()
	{
		Base::free(BITMAP);
		Base::free(IDMAP);

		metadata()->reset();
	}

	void allocateBlocks(UBigInt start, UBigInt length)
	{
		Metadata* metadata = this->metadata();

		metadata->start_position() 	= start;
		metadata->length()			= length;

		Int bitmap_size = Base::roundUpBitToBytes(length);

		Base::resizeBlock(BITMAP, bitmap_size);
		Base::clear(BITMAP);
	}

	UBigInt allocateBlock()
	{
		UBigInt* bitmap 	= this->bitmap();
		Metadata* metadata 	= this->metadata();

		SelectResult result = Select0FW(bitmap, 0, metadata->length(), 1);

		MEMORIA_ASSERT_TRUE(result.is_found());
		MEMORIA_ASSERT(result.idx(), <, metadata->length());

		SetBit(bitmap, result.idx(), 1);

		UBigInt global_pos = metadata->start_position() + result.idx();

		return global_pos;
	}

	void markAllocated(UBigInt pos)
	{
		UBigInt* bitmap 	= this->bitmap();
		Metadata* metadata 	= this->metadata();

		MEMORIA_ASSERT(pos, <, metadata->length());

		SetBit(bitmap, pos, 1);
	}

};

}

#endif
