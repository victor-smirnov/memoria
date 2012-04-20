
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_API_METADATA_FIELD_HPP
#define _MEMORIA_CORE_API_METADATA_FIELD_HPP

#include <memoria/metadata/field.hpp>
#include <memoria/core/vapi/metadata/metadata.hpp>

#include <memoria/core/tools/bitmap.hpp>


namespace memoria { namespace vapi {



template <typename Interface>
class FieldMetadataImplT: public MetadataImplT<Interface> {
	typedef FieldMetadataImplT<Interface> 	Me;
	typedef MetadataImplT<Interface> 		Base;

	typedef typename Interface::Code2TypeMap Code2TypeMap;

public:

	FieldMetadataImplT(Int ptr, Int abi_ptr, Byte type, const string &name, const Int offset, const Int limit, Int item_size) :
		Base(name, type), ptr_(ptr), abi_ptr_(abi_ptr), offset_(offset), limit_(limit), item_size_(item_size) {
		SetupValueType();
	}

	FieldMetadataImplT(Int ptr, Int abi_ptr, Byte type, const string &name, const Int limit, Int item_size) :
		Base(name, type), ptr_(ptr), abi_ptr_(abi_ptr), offset_(0), limit_(limit), item_size_(item_size) {
		SetupValueType();
	}

	FieldMetadataImplT(Int ptr, Int abi_ptr, Byte type, const string &name, Int item_size) :
		Base(name, type), ptr_(ptr), abi_ptr_(abi_ptr), offset_(0), limit_(item_size), item_size_(item_size) {
		SetupValueType();
	}

	virtual Int Ptr() const {
		return ptr_;
	}

	virtual Int AbiPtr() const {
		return abi_ptr_;
	}

	virtual Int Count() const {
		return (limit_ - offset_) / Size();
	}

	virtual Int Size() const {
		return item_size_;
	}

	virtual Int Offset() const {
		return offset_;
	}

	virtual Int Limit() const {
		return limit_;
	}

	virtual const void *ValuePtr(const void *mem, Int idx) const {
		return ValuePtr(mem, idx, ptr_);
	}

	virtual void *ValuePtr(void *mem, Int idx) const {
		return ValuePtr(mem, idx, ptr_);
	}

	virtual const void *ValuePtrAbi(const void *mem, Int idx) const {
		return ValuePtr(mem, idx, abi_ptr_);
	}

	virtual void *ValuePtrAbi(void *mem, Int idx) const {
		return ValuePtr(mem, idx, abi_ptr_);
	}

	virtual const void *BitmapPtr(const void *mem) const {
		return BitmapPtr(mem, ptr_);
	}

	virtual void *BitmapPtr(void *mem) const {
		return BitmapPtr(mem, ptr_);
	}

	virtual const void *BitmapPtrAbi(const void *mem) const {
		return BitmapPtr(mem, abi_ptr_);
	}

	virtual void *BitmapPtrAbi(void *mem) const {
		return BitmapPtr(mem, abi_ptr_);
	}

	virtual void SetValue(void *mem, const void *value, Int idx) const {
		return SetValue(mem, value, idx, ptr_);
	}

	virtual void SetValueAbi(void *mem, const void *value, Int idx) const {
		return SetValue(mem, value, idx, abi_ptr_);
	}

	virtual void GetValue(const void *mem, void *value, Int idx) const {
		return GetValue(mem, value, idx, ptr_);
	}

	virtual void GetValueAbi(const void *mem, void *value, Int idx) const {
		return GetValue(mem, value, idx, abi_ptr_);
	}

	virtual BigInt GetBits(const void *mem, Int idx, Int nbits) const {
		return GetBits(mem, idx, nbits, ptr_);
	}

	virtual BigInt GetBitsAbi(const void *mem, Int idx, Int nbits) const {
		return GetBits(mem, idx, nbits, abi_ptr_);
	}

	virtual void SetBits(void *mem, BigInt bits, Int idx, Int nbits) const {
		SetBits(mem, bits, idx, nbits, ptr_);
	}

	virtual void SetBitsAbi(void *mem, BigInt bits, Int idx, Int nbits) const {
		SetBits(mem, bits, idx, nbits, abi_ptr_);
	}

	virtual void* CreateValueHolder();


	virtual void Configure(Int ptr, Int abi_ptr)
	{
		this->ptr_ 		= ptr;
		this->abi_ptr_	= abi_ptr;
	}


private:

    void SetBits(void *mem, BigInt bits, Int idx, Int nbits, Int ptr) const;
    BigInt GetBits(const void *mem, Int idx, Int nbits, Int ptr) const;

    void GetValue(const void *mem, void *value, Int idx, Int ptr) const;
    void SetValue(void *mem, const void *value, Int idx, Int ptr) const;

    const void *BitmapPtr(const void *mem, Int ptr) const {
        return static_cast<const char*>(mem) + ptr;
    }

    void *BitmapPtr(void *mem, Int ptr) const {
        return static_cast<char*>(mem) + ptr;
    }

    const void *ValuePtr(const void *mem, Int idx, Int ptr) const {
        return static_cast<const char*>(mem) + ptr + Offset() + idx * Size();
    }

    void *ValuePtr(void *mem, Int idx, Int ptr) const {
        return static_cast<char*>(mem) + ptr + Offset() + idx * Size();
    }

    void SetupValueType();

    Int  ptr_;
    Int  abi_ptr_;
    const Int  offset_;
    const Int  limit_;
    const Int  item_size_;
};


typedef FieldMetadataImplT<memoria::vapi::FieldMetadata> 		FieldMetadataImpl;



template <typename Interface>
void FieldMetadataImplT<Interface>::SetValue(void *mem, const void *value, Int idx, Int ptr) const {
    if (idx < Count())
    {
        char *buffer = static_cast<char*> (mem) + ptr + Offset() + Size() * idx;
        const char *val = static_cast<const char*> (value);
        memoria::CopyBuffer(val, buffer, Size());
    }
    else {
        throw new BoundsException(MEMORIA_SOURCE, "ArrayField index is out of bounds.", idx, 0, Count());
    }
}

template <typename Interface>
void FieldMetadataImplT<Interface>::GetValue(const void *mem, void *value, Int idx, Int ptr) const {
    if (idx < Count()) {
        const char *buffer = (static_cast<const char*> (mem) + ptr + Offset() + Size() * idx);
        char *val = static_cast<char*> (value);
        memoria::CopyBuffer(buffer, val, Size());
    } else {
        throw new BoundsException(MEMORIA_SOURCE, "ArrayField index is out of bounds.", idx, 0, Count());
    }
}

template <typename Interface>
BigInt FieldMetadataImplT<Interface>::GetBits(const void *mem, Int idx, Int nbits, Int ptr) const {
    const void *tmp = static_cast<const char*> (mem) + ptr;
    const Int *buffer = static_cast<const Int*> (tmp);
    Int top_range = idx + Offset() + nbits;
    Int bit_size = top_range <= Limit() ? nbits : nbits - (top_range - Limit());
    return memoria::GetBits(buffer, idx, bit_size);
}

template <typename Interface>
void FieldMetadataImplT<Interface>::SetBits(void *mem, BigInt bits, Int idx, Int nbits, Int ptr) const {
    void *tmp = static_cast<char*> (mem) + ptr;
    Int *buffer = static_cast<Int*> (tmp);
    Int top_range = idx + Offset() + nbits;
    Int bit_size = top_range <= Limit() ? nbits : nbits - (top_range - Limit());
    memoria::SetBits(buffer, idx, Offset(), bit_size);
}


template <typename Interface>
void* FieldMetadataImplT<Interface>::CreateValueHolder() {
	throw MemoriaException(MEMORIA_SOURCE, "The method's implementation is broken");
}


template <typename Interface>
void FieldMetadataImplT<Interface>::SetupValueType() {
}

}}


#endif
