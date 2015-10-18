
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef INCLUDE_MEMORIA_CORE_TOOLS_COW_SHARED_PTR_HPP_
#define INCLUDE_MEMORIA_CORE_TOOLS_COW_SHARED_PTR_HPP_

#include <memoria/core/types/types.hpp>

#include <memoria/core/tools/optional.hpp>
#include <memoria/core/tools/cow_tree/cow_tree_node.hpp>

#include <memoria/core/tools/static_array.hpp>

#include <vector>
#include <mutex>
#include <type_traits>

/**
 * Serializable Copy-on-write B+Tree. Shared Resource Handle.
 */

namespace memoria 	{
namespace cow 		{
namespace tree 		{

template <typename Handle>
class LocalRefCnt {
	Int refs_ = 0;
	Handle* handle_;

public:
	LocalRefCnt(Handle* handle): handle_(handle)
	{}

	void ref() {
		refs_++;
	}

	void unref()
	{
		if (--refs_ == 0)
		{
			handle_->unref();
		}
	}

	Handle* get() {
		return handle_;
	}

	const Handle* get() const {
		return handle_;
	}
};

template <typename ObjType, typename Handle>
class CoWSharedPtr {
public:
	using PtrType 			= CoWSharedPtr<ObjType, Handle>;
	using LocalRefCntHandle = LocalRefCnt<Handle>;

protected:
	LocalRefCntHandle* handle_;

protected:
	CoWSharedPtr(Handle* handle): handle_(new LocalRefCntHandle(handle))
	{
		handle_->ref();
	}

	CoWSharedPtr(): handle_(nullptr)
	{}

	CoWSharedPtr(const PtrType& other): handle_(other.handle_)
	{
		handle_->ref();
	}

	CoWSharedPtr(PtrType&& other): handle_(other.handle_)
	{
		other.handle_ = nullptr;
	}

	~CoWSharedPtr()
	{
		if (handle_) handle_->unref();
	}
public:

	ObjType& operator=(const ObjType& other)
	{
		if (handle_) handle_->unref();

		handle_ = other.handle_;
		handle_->ref();

		return (ObjType&)*this;
	}

	ObjType& operator=(ObjType&& other)
	{
		if (handle_) handle_->unref();

		handle_ = other.handle_;
		other.handle_ = nullptr;
		return (ObjType&) *this;
	}

	bool operator==(const PtrType& other) const
	{
		return (handle_ == nullptr && other.handle_ == nullptr)  ||
				(handle_ != nullptr && other.handle_ != nullptr && (*handle_) == (*other.handle_));
	}

	bool operator!=(const PtrType& other) const
	{
		return handle_ != nullptr && other.handle_ != nullptr && (*handle_) != (*other.handle_);
	}

	ObjType new_tread()
	{
		return ObjType(handle_);
	}

	Handle* get() {
		return handle_->get();
	}

	const Handle* get() const {
		return handle_->get();
	}
};

}
}
}

#endif
