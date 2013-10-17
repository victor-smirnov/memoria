
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_ALLOCATORS_FILE_SUPERBLOCK_HPP
#define _MEMORIA_ALLOCATORS_FILE_SUPERBLOCK_HPP

#include <memoria/core/container/profile.hpp>
#include <memoria/core/container/init.hpp>
#include <memoria/core/exceptions/memoria.hpp>

#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/allocators/file/packed_blockmap.hpp>

#include <fstream>
#include <malloc.h>
#include <string.h>
#include <cstddef>

namespace memoria {

template <
	typename ID
>
class Superblock {

	typedef Superblock<ID>														MyType;

	char magic_[32];
	Int type_hash_;

	Int version_major_;
	Int version_minor_;

	ID blockmap_root_id_;
	ID idmap_root_id_;
	ID rootmap_id_;

	UBigInt backup_list_start_;
	UBigInt backup_list_size_;
	UBigInt superblock_version_;

	Int superblock_size_;
	Int block_size_;

	BigInt name_counter_;
	UBigInt id_counter_;

	UBigInt file_size_;

	UBigInt total_blocks_;
	UBigInt free_blocks_;

	PackedAllocator allocator_;



public:

	typedef PackedBlockMap<ID>													BlockMap;

	typedef TypeList<BlockMap>													ObjectsList;

	typedef typename PackedDispatchersListBuilder<ObjectsList>::Type 			PackedTypesList;
	typedef typename PackedDispatcherTool<PackedTypesList>::Type              	Dispatcher;

	enum {BLOCK_MAP};


	struct InitFn {
		template <Int Idx, typename Obj>
		void stream(Obj*, PackedAllocator* allocator)
		{
			allocator->allocateEmpty<Obj>(Idx);
		}
	};

	void init(Int superblock_size, Int block_size)
	{
		memset(magic_, 0, sizeof(magic_));
		strncpy(magic_, "MEMORIA FILE", sizeof(magic_));

		type_hash_			= 0;

		version_major_ 		= 1;
		version_minor_ 		= 0;

		blockmap_root_id_ 	= ID(0);
		idmap_root_id_ 		= ID(0);
		rootmap_id_ 		= ID(0);

		backup_list_start_	= 0;
		backup_list_size_ 	= 0;
		superblock_version_	= 0;

		superblock_size_ 	= superblock_size;
		block_size_			= block_size;

		name_counter_		= 1000;
		id_counter_			= 1;

		file_size_			= 0;
		total_blocks_		= 0;
		free_blocks_		= 0;

		allocator_.init(superblock_size_ - sizeof(MyType) + sizeof(PackedAllocator), ListSize<ObjectsList>::Value);

		Dispatcher::dispatchAllStatic(InitFn(), &allocator_);
	}

	void assertValid()
	{
		checkMagic();
		checkTypeHash();
		checkVersion();
	}

	void checkMagic()
	{
		if (strncmp(magic_, "MEMORIA FILE", sizeof(magic_)))
		{
			throw Exception(MA_SRC, "Invalid File Descriptor");
		}
	}

	void checkTypeHash()
	{}

	void checkVersion()
	{}

	Int& version_major() {
		return version_major_;
	}

	const Int& version_major() const {
		return version_major_;
	}

	Int& version_minor() {
		return version_minor_;
	}

	const Int& version_minor() const {
		return version_minor_;
	}

	ID& blockmap_root_id() {
		return blockmap_root_id_;
	}

	const ID& blockmap_root_id() const {
		return blockmap_root_id_;
	}

	ID& idmap_root_id() {
		return idmap_root_id_;
	}

	const ID& idmap_root_id() const {
		return idmap_root_id_;
	}

	ID& rootmap_id() {
		return rootmap_id_;
	}

	const ID& rootmap_id() const {
		return rootmap_id_;
	}

	UBigInt& backup_list_start() {
		return backup_list_start_;
	}

	const UBigInt& backup_list_start() const {
		return backup_list_start_;
	}

	UBigInt& backup_list_size() {
		return backup_list_size_;
	}

	const UBigInt& backup_list_size() const {
		return backup_list_size_;
	}

	UBigInt& superblock_version() {
		return superblock_version_;
	}

	const UBigInt& superblock_version() const {
		return superblock_version_;
	}

	const Int& superblock_size() const {
		return superblock_size_;
	}

	const Int& block_size() const {
		return block_size_;
	}

	const BigInt& name_counter() const {
		return name_counter_;
	}

	const UBigInt& id_counter() const {
		return id_counter_;
	}

	UBigInt& file_size() {
		return file_size_;
	}

	const UBigInt& file_size() const {
		return file_size_;
	}

	UBigInt& total_blocks() {
		return total_blocks_;
	}

	const UBigInt& total_blocks() const {
		return total_blocks_;
	}

	UBigInt& free_blocks() {
		return free_blocks_;
	}

	const UBigInt& free_blocks() const {
		return free_blocks_;
	}

	BigInt new_ctr_name()
	{
		return name_counter_++;
	}

	UBigInt new_id()
	{
		return id_counter_++;
	}

	PackedAllocator* allocator() {
		return allocator_;
	}

	const PackedAllocator* allocator() const {
		return allocator_;
	}

	BlockMap* blockmap() {
		return allocator_.template get<BlockMap>(BLOCK_MAP);
	}

	const BlockMap* blockmap() const {
		return allocator_.template get<BlockMap>(BLOCK_MAP);
	}


	void generateDataEvents(IPageDataEventHandler* handler) const
	{
		handler->startGroup("SUPERBLOCK");

		handler->value("MAGIC",   magic_, sizeof(magic_));

		handler->value("TYPE_HASH",   		&type_hash_);

		handler->value("VERSION_MAJOR",   	&version_major_);
		handler->value("VERSION_MINOR",   	&version_minor_);

		handler->value("BLOCKMAP_ROOT_ID",	&blockmap_root_id_);
		handler->value("IDMAP_ROOT_ID",		&idmap_root_id_);
		handler->value("ROOTMAP_ID",		&rootmap_id_);

		handler->value("BACKUP_LIST_START", &backup_list_start_);
		handler->value("BACKUP_LIST_SIZE",  &backup_list_size_);
		handler->value("SUPERBLOCK_VERSION",&superblock_version_);

		handler->value("SUPERBLOCK_SIZE",	&superblock_size_);
		handler->value("BLOCK_SIZE",		&block_size_);

		handler->value("NAME_COUNTER",		&name_counter_);
		handler->value("ID_COUNTER",		&id_counter_);

		handler->value("FILE_SIZE",			&file_size_);

		handler->value("TOTAL_BLOCKS",		&total_blocks_);
		handler->value("FREE_BLOCKS",		&free_blocks_);

		GenerateDataEventsTool<ObjectsList>::generateDataEvents(&allocator_, handler);

		handler->endGroup();
	}

	void serialize(SerializationData& buf) const
	{
		FieldFactory<char>::serialize(buf, magic_, sizeof(magic_));

		FieldFactory<Int>::serialize(buf, type_hash_);
		FieldFactory<Int>::serialize(buf, version_major_);
		FieldFactory<Int>::serialize(buf, version_minor_);

		FieldFactory<ID>::serialize(buf, blockmap_root_id_);
		FieldFactory<ID>::serialize(buf, idmap_root_id_);
		FieldFactory<ID>::serialize(buf, rootmap_id_);

		FieldFactory<UBigInt>::serialize(buf, backup_list_start_);
		FieldFactory<UBigInt>::serialize(buf, backup_list_size_);
		FieldFactory<UBigInt>::serialize(buf, superblock_version_);

		FieldFactory<Int>::serialize(buf, superblock_size_);
		FieldFactory<Int>::serialize(buf, block_size_);

		FieldFactory<BigInt>::serialize(buf, name_counter_);
		FieldFactory<UBigInt>::serialize(buf, id_counter_);

		FieldFactory<UBigInt>::serialize(buf, file_size_);

		FieldFactory<UBigInt>::serialize(buf, total_blocks_);
		FieldFactory<UBigInt>::serialize(buf, free_blocks_);

		SerializeTool<ObjectsList>::serialize(&allocator_, buf);
	}

	void deserializeHeader(DeserializationData& buf)
	{
		FieldFactory<char>::deserialize(buf, magic_, sizeof(magic_));

		FieldFactory<Int>::deserialize(buf, type_hash_);
		FieldFactory<Int>::deserialize(buf, version_major_);
		FieldFactory<Int>::deserialize(buf, version_minor_);

		FieldFactory<ID>::deserialize(buf, blockmap_root_id_);
		FieldFactory<ID>::deserialize(buf, idmap_root_id_);
		FieldFactory<ID>::deserialize(buf, rootmap_id_);

		FieldFactory<UBigInt>::deserialize(buf, backup_list_start_);
		FieldFactory<UBigInt>::deserialize(buf, backup_list_size_);
		FieldFactory<UBigInt>::deserialize(buf, superblock_version_);

		FieldFactory<Int>::deserialize(buf, superblock_size_);
		FieldFactory<Int>::deserialize(buf, block_size_);

		FieldFactory<BigInt>::deserialize(buf, name_counter_);
		FieldFactory<UBigInt>::deserialize(buf, id_counter_);

		FieldFactory<UBigInt>::deserialize(buf, file_size_);

		FieldFactory<UBigInt>::deserialize(buf, total_blocks_);
		FieldFactory<UBigInt>::deserialize(buf, free_blocks_);
	}

	void deserialize(DeserializationData& buf)
	{
		deserializeHeader(buf);

		DeserializeTool<ObjectsList>::deserialize(&allocator_, buf);
	}
};

}

#endif
