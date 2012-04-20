
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CORE_API_METADATA_MAP_HPP
#define _MEMORIA_CORE_API_METADATA_MAP_HPP

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/metadata/map.hpp>
#include <memoria/core/vapi/metadata/group.hpp>
#include <memoria/core/vapi/metadata/typed_field.hpp>
#include <memoria/core/vapi/metadata/typed_id.hpp>


namespace memoria { namespace vapi {


template <typename MapType>
class MapMetadataImpl: public MetadataGroupImplT<MapMetadata> {
	typedef MapMetadataImpl<MapType> 				Me;
	typedef MetadataGroupImplT<MapMetadata> 		Base;
public:

	MapMetadataImpl(const MetadataList &content, FieldMetadata* key_metadata, FieldMetadata* index_key_metadata, FieldMetadata* value_metadata, Int ptr, Int abi_ptr):
		Base("MAP", content, 0, Metadata::MAP),
		key_metadata_(key_metadata),
		index_key_metadata_(index_key_metadata),
		value_metadata_(value_metadata),
		ptr_(ptr),
		abi_ptr_(abi_ptr)
	{}

	virtual Int GetSize(const void* mem) const
	{
		const MapType* map = T2T<const MapType*>(mem);
		return map->size();
	}

	virtual Int GetIndexSize(const void* mem) const
	{
		const MapType* map = T2T<const MapType*>(mem);
		return map->max_size();
	}

	virtual Int GetBlocks() const {
		return MapType::Blocks;
	}

	virtual Int GetKeySize() const {
		return sizeof(typename MapType::Key);
	}

	virtual Int GetIndexKeySize() const {
		return sizeof(typename MapType::IndexKey);
	}

	virtual Int GetValueSize() const {
		return MapType::GetValueSize();
	}

	virtual bool IsSet() const {
		return GetValueSize() == 0;
	}

    virtual FieldMetadata* GetIndexField(const void* mem, Int block_num, Int idx)
    {
    	const MapType* map = T2T<const MapType*>(mem);
    	Int offset = map->GetIndexKeyBlockOffset(block_num) + idx * GetIndexKeySize();

    	index_key_metadata_->Configure(ptr_ + offset, abi_ptr_ + offset);

    	return index_key_metadata_;
    }

    virtual FieldMetadata* GetKeyField(const void* mem, Int block_num, Int idx)
    {
    	const MapType* map = T2T<const MapType*>(mem);
    	Int offset = map->GetKeyBlockOffset(block_num) + idx * GetKeySize();

    	key_metadata_->Configure(ptr_ + offset, abi_ptr_ + offset);

    	return key_metadata_;
    }

    virtual FieldMetadata* GetValueField(const void* mem, Int idx)
    {
    	const MapType* map = T2T<const MapType*>(mem);
    	Int offset = map->GetValueBlockOffset() + idx * GetValueSize();

    	value_metadata_->Configure(ptr_ + offset, abi_ptr_ + offset);

    	return value_metadata_;
    }

private:

	FieldMetadata* key_metadata_;
	FieldMetadata* index_key_metadata_;
	FieldMetadata* value_metadata_;

	Int ptr_;
	Int abi_ptr_;
};









}}


#endif
