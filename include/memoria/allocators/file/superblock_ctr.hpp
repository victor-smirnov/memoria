
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_ALLOCATORS_FILE_SUPERBLOCK_CTR_HPP
#define _MEMORIA_ALLOCATORS_FILE_SUPERBLOCK_CTR_HPP

#include <memoria/core/container/profile.hpp>
#include <memoria/core/container/init.hpp>

#include <memoria/allocators/file/superblock.hpp>
#include <memoria/allocators/file/packed_blockmap.hpp>

#include <memoria/core/tools/assert.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/isymbols.hpp>

#include <fstream>
#include <memory>
#include <malloc.h>

namespace memoria {

template <typename ID>
class SuperblockCtr {
public:
	typedef Superblock<ID>														SuperblockType;
	typedef std::unique_ptr<SuperblockType,  void (*)(void*)>					SuperblockPtrType;
	typedef std::unique_ptr<char, void (*)(void*)>								IOBufferPtr;

private:
	filebuf& file_;
	Int block_size_;
	Int superblock_size_;

	SuperblockPtrType superblock_;
	SuperblockPtrType updated_;

	IOBufferPtr io_buffer_;

public:

	// Load existing file
	SuperblockCtr(std::filebuf& file):
		file_(file),
		block_size_(0),
		superblock_size_(0),
		superblock_(nullptr, free),
		updated_(nullptr, free),
		io_buffer_(nullptr, free)
	{
		SuperblockType superblockHeader;

		loadHeader(superblockHeader, 0);

		superblockHeader.assertValid();

		superblock_size_ 	= superblockHeader.superblock_size();
		block_size_ 		= superblockHeader.block_size();

		superblock_ = SuperblockPtrType(T2T<SuperblockType*>(malloc(superblock_size_)), free);
		updated_ 	= SuperblockPtrType(T2T<SuperblockType*>(malloc(superblock_size_)), free);

		io_buffer_ 	= IOBufferPtr(T2T<char*>(malloc(superblock_size_)), free);

		load(superblock_, 0);
		load(updated_, superblock_size_);

		MEMORIA_ASSERT_TRUE(superblock_->superblock_version() == updated_->superblock_version());
	}

	// Fill newly created file
	SuperblockCtr(std::filebuf& file, Int block_size):
		file_(file),
		superblock_(nullptr, free),
		updated_(nullptr, free),
		io_buffer_(nullptr, free)
	{
		block_size_ 		= block_size;
		superblock_size_	= block_size_ / 2;

		superblock_ = SuperblockPtrType(T2T<SuperblockType*>(malloc(superblock_size_)), free);
		updated_ 	= SuperblockPtrType(T2T<SuperblockType*>(malloc(superblock_size_)), free);

		io_buffer_ 	= IOBufferPtr(T2T<char*>(malloc(superblock_size_)), free);

		superblock_->init(superblock_size_, block_size_);
		updated_->init(superblock_size_, block_size_);
	}

	Int superblock_size() const
	{
		return superblock_size_;
	}

	Int block_size() const
	{
		return block_size_;
	}


	bool is_updated() const
	{
		return superblock_->superblock_version() != updated_->superblock_version();
	}

	void update()
	{
		if (!is_updated())
		{
			updated_->superblock_version()++;
		}
	}

	void storeBackup()
	{
		store(updated_, superblock_size_);
	}

	void storeSuperblock()
	{
		CopyByteBuffer(updated_.get(), superblock_.get(), superblock_size_);

		store(superblock_, 0);
	}

	const ID& blockmap_root_id() const
	{
		return updated_->blockmap_root_id();
	}

	void set_blockmap_root_id(const ID& id)
	{
		update();
		updated_->blockmap_root_id() = id;
	}

	const ID& idmap_root_id() const
	{
		return updated_->idmap_root_id();
	}

	void set_idmap_root_id(const ID& id)
	{
		update();
		updated_->idmap_root_id() = id;
	}

	const ID& rootmap_id() const
	{
		return updated_->rootmap_id();
	}

	void set_rootmap_id(const ID& id)
	{
		update();
		updated_->rootmap_id() = id;
	}

	void set_backup_list(UBigInt list_head, UBigInt list_size)
	{
		update();

		updated_->backup_list_start() 	= list_head;
		updated_->backup_list_size() 	= list_size;
	}

	BigInt new_ctr_name()
	{
		update();
		return updated_->new_ctr_name();
	}

	UBigInt new_id()
	{
		update();
		return updated_->new_id();
	}

	UBigInt free_blocks() const
	{
		return updated_->free_blocks();
	}

	UBigInt total_blocks() const
	{
		return updated_->total_blocks();
	}

	void setMainBlockMap(UBigInt total_blocks, UBigInt free_blocks)
	{
		update();

		updated_->total_blocks() 	= total_blocks;
		updated_->free_blocks() 	= free_blocks;
	}

	UBigInt temporary_allocated_blocks() const
	{
		return updated_->blockmap()->metadata()->allocated();
	}

	void setTemporaryBlockMap(UBigInt start, UBigInt length)
	{
		update();

		updated_->blockmap()->allocateBlocks(start, length);
	}

	const UBigInt* temporary_blockmap() const
	{
		return updated_->blockmap()->bitmap();
	}

	SymbolsBuffer<1, const UBigInt> temporary_blockmap_buffer() const
	{
		return SymbolsBuffer<1, const UBigInt>(temporary_blockmap(), updated_->blockmap()->metadata()->length());
	}

	bool use_temporary_allocator() const
	{
		return updated_->blockmap()->metadata()->allocated() > 0;
	}

	void updateMainBlockMapMetadata()
	{
		update();

		auto metadata = updated_->blockmap()->metadata();

		auto length 	= metadata->length();
		auto allocated 	= metadata->allocated();

		updated_->total_blocks() 	+= length;
		updated_->free_blocks() 	+= length - allocated;
	}

	void clearTemporaryBlockMap()
	{
		update();

		updated_->blockmap()->clear();
	}

	void markAllocated(UBigInt idx)
	{
		update();

		updated_->blockmap()->markAllocated(idx);
	}

	UBigInt allocateTemporaryBlock()
	{
		return updated_->blockmap()->allocateBlock();
	}

	UBigInt file_size() const
	{
		return updated_->file_size();
	}

	void enlargeFile(UBigInt delta)
	{
		updated_->file_size() += delta;
	}

	void shrinkFile(UBigInt delta)
	{
		updated_->file_size() -= delta;
	}

protected:

	void load(SuperblockPtrType& block, std::streamsize pos)
	{
		auto tpos = file_.pubseekpos(pos);
		MEMORIA_ASSERT(tpos, ==, pos);

		memset(block.get(), 0, superblock_size_);

		auto size = file_.sgetn(io_buffer_.get(), superblock_size_);
		MEMORIA_ASSERT_TRUE(size == (std::streamsize)superblock_size_);

		DeserializationData data;
		data.buf = io_buffer_.get();

		block->deserialize(data);
	}

	void loadHeader(SuperblockType& block, std::streamsize pos)
	{
		auto tpos = file_.pubseekpos(pos);
		MEMORIA_ASSERT(tpos, ==, pos);

		const std::size_t HEADER_SIZE  = sizeof(SuperblockType);

		char buffer[HEADER_SIZE];

		memset(buffer, 0, sizeof(buffer));

		auto size = file_.sgetn(buffer, HEADER_SIZE);
		MEMORIA_ASSERT_TRUE(size == HEADER_SIZE);

		DeserializationData data;
		data.buf = buffer;

		block.deserializeHeader(data);
	}

	void store(const SuperblockPtrType& block, std::streamsize pos)
	{
		auto tpos = file_.pubseekpos(pos);
		MEMORIA_ASSERT(tpos, ==, pos);

		memset(io_buffer_.get(), 0, superblock_size_);

		SerializationData data;
		data.buf = io_buffer_.get();

		block->serialize(data);

		auto size = file_.sputn(io_buffer_.get(), superblock_size_);
		MEMORIA_ASSERT_TRUE(size == (std::streamsize)superblock_size_);

		Int status = file_.pubsync();
		MEMORIA_ASSERT_TRUE(status == 0);
	}

	void storeSuperblocks()
	{
		store(updated_, superblock_size_);

		CopyByteBuffer(updated_.get(), superblock_.get(), superblock_size_);

		store(superblock_, 0);
	}
};

}

#endif
