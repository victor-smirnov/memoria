
// Copyright Victor Smirnov, Ivan Yurchenko 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_BLOB_MAP_CONTAINER_CONTAINER_HPP
#define _MEMORIA_CONTAINERS_BLOB_MAP_CONTAINER_CONTAINER_HPP

#include <memoria/core/container/container.hpp>


namespace memoria {

template <typename Types> struct VectorMapCtrTypes;
template <typename Types> struct VectorMapIterTypes;

template <typename Types>
class Ctr<VectorMapCtrTypes<Types> >
{
public:
	typedef Ctr<VectorMapCtrTypes<Types> >						MyType;
	typedef typename Types::Profile								Profile;
	typedef typename Types::Allocator 							Allocator;
	typedef typename Types::Allocator::CtrShared 				CtrShared;
	typedef typename Types::Allocator::PageG 					PageG;
	typedef typename Allocator::Page							Page;
	typedef typename Page::ID									ID;

	typedef IParentCtrInterface<typename Types::Allocator>		ParentCtrInterface;

	typedef typename CtrTF<Profile, SumSet2, SumSet2>::Type		IdxSet;
	typedef typename CtrTF<Profile, Vector,	 Vector>::Type		ByteArray;

	static const Int IS_Indexes									= IdxSet::Indexes;
	static const Int BA_Indexes									= ByteArray::Indexes;

	typedef typename IdxSet::Key								Key;
	typedef typename IdxSet::Value								ISValue;

private:
	Allocator&	allocator_;

	const char* model_type_name_;

	Logger logger_;
	static Logger class_logger_;


	bool 		debug_;

	ParentCtrInterface* parent_ctr_;

	ByteArray	array_;
	IdxSet		set_;

	static ContainerMetadata* 	reflection_;

public:
	typedef Iter<VectorMapIterTypes<Types> >						Iterator;


	Ctr(Allocator &allocator, BigInt name, bool create = false):
		allocator_(allocator),
		model_type_name_("memoria::BlobMap"),
		logger_(model_type_name_, Logger::DERIVED, &class_logger_),
		debug_(false),
		parent_ctr_(NULL),
		array_(allocator, name, create, "memoria::ByteArray"),
		set_(&array_, 0, create, "memoria::IdxSet")
	{ }

	Ctr(Allocator &allocator, const ID& root_id):
		allocator_(allocator),
		model_type_name_("memoria::BlobMap"),
		logger_(model_type_name_, Logger::DERIVED, &class_logger_),
		debug_(false),
		parent_ctr_(NULL),
		array_(allocator, root_id, "memoria::ByteArray"),
		set_(&array_, get_ctr_root(allocator, root_id, 0), "memoria::IdxSet")
	{ }

	Ctr(const MyType& other):
		allocator_(other.allocator_),
		model_type_name_("memoria::BlobMap"),
		logger_(other.logger_),
		debug_(other.debug_),
		parent_ctr_(other.parent_ctr_),
		array_(allocator_, other.array_.root(), "memoria::ByteArray"),
		set_(&array_, other.set_.root(), "memoria::IdxSet")
	{}

	Ctr(MyType&& other):
		allocator_(other.allocator_),
		model_type_name_(other.model_type_name_),
		logger_(other.logger_),
		debug_(other.debug_),
		parent_ctr_(other.parent_ctr_),
		array_(std::move(other.array_), NULL),
		set_(std::move(other.set_), &array_)
	{ }

	Ctr(MyType&& other, ParentCtrInterface* parent):
		allocator_(other.allocator_),
		model_type_name_(other.model_type_name_),
		logger_(other.logger_),
		debug_(other.debug_),
		parent_ctr_(other.parent_ctr_),
		array_(std::move(other.array_), parent),
		set_(std::move(other.set_), &array_)
	{ }

	//Public API goes here

	IdxSet& set() {
		return set_;
	}

	ByteArray& array() {
		return array_;
	}

	Iterator Begin()
	{
		return Iterator(*me(), set_.Begin(), array_.Begin());
	}


	Iterator End()
	{
		auto is_iter = set_.End();
		if (!is_iter.IsEnd())
		{
			auto ba_iter = array_.End();

			ba_iter.Skip(-is_iter.GetRawKey(1));

			return Iterator(*me(), is_iter, ba_iter);
		}
		else {
			return Iterator(*me(), is_iter, array_.End());
		}
	}

	Iterator Find(BigInt lob_id)
	{
		auto is_iter = set_.FindLT(lob_id, 0, false);
		auto ba_iter = array_.Seek(is_iter.prefix(1));
		return Iterator(*me(), is_iter, ba_iter);
	}

	Iterator Create()
	{
		auto is_iter = set_.End();

		Key keys[IS_Indexes];

		for (Key& k: keys)
		{
			k = 0;
		}

		keys[0] = 1;

		set_.InsertEntry(is_iter, keys, ISValue());

		auto ba_iter = array_.End();
		return Iterator(*me(), is_iter, ba_iter);
	}


	void RemoveByIndex(BigInt blob_index)
	{
		auto is_iter 	= set_.GetByIndex(blob_index);

		BigInt pos 		= is_iter.prefix(1);
		BigInt size 	= is_iter.GetRawKey(1);

		auto ba_iter 	= array_.Seek(pos);

		ba_iter.Remove(size);
		set_.RemoveEntry(is_iter);
	}

	static Int Init()
	{
		Int salt = 123456;
		Int hash = IdxSet::Init(salt) + ByteArray::Init(salt);

		if (reflection_ == NULL)
		{
			MetadataList list;

			IdxSet::reflection()->PutAll(list);
			ByteArray::reflection()->PutAll(list);

			reflection_ = new ContainerMetadataImpl("memoria::VectorMap", list, VectorMap::Code + salt, &CreateContainer);
		}

		return hash;
	}

	static ContainerMetadata* reflection() {
		return reflection_;
	}

	static Int hash()
	{
		return IdxSet::hash() + ByteArray::hash();
	}


	bool Check(void* ptr)
	{
		bool r0 = array_.Check(ptr);
		bool r1 = set_.Check(ptr);
		return r1 && r0;
	}



	bool& debug() {
		return debug_;
	}

	const bool& debug() const {
		return debug_;
	}

	Allocator& allocator() {
		return allocator_;
	}

	Allocator& allocator() const {
		return allocator_;
	}

	const char* type_name() const {
		return model_type_name_;
	}

	bool is_log(Int level)
	{
		return logger_.IsLogEnabled(level);
	}

	memoria::vapi::Logger& logger() {
		return logger_;
	}

	static memoria::vapi::Logger& class_logger() {
		return class_logger_;
	}

	BigInt name() const {
		return array_->name();
	}

	BigInt& name() {
		return array_->name();
	}

	MyType* me()
	{
		return this;
	}

	const MyType* me() const
	{
		return this;
	}

	const CtrShared* shared() const {
		return array_.shared();
	}

	CtrShared* shared() {
		return array_.shared();
	}

private:
	static ID get_ctr_root(Allocator& allocator, const ID& root_id, BigInt name)
	{
		typedef typename ByteArray::NodeBaseG 	NodeBaseG;
		typedef typename ByteArray::Metadata 	Metadata;

		NodeBaseG root 	= allocator.GetPage(root_id, Allocator::READ);
		Metadata  meta 	= ByteArray::GetRootMetadata(root);

		return meta.roots(name);
	}

	// To be removed
	static Container* CreateContainer(const IDValue& rootID, memoria::vapi::ContainerCollection* container, BigInt name) {
		return NULL;
	}
};


template<
        typename Types
>
Logger Ctr<VectorMapCtrTypes<Types> >::class_logger_("memoria::BlobMap", Logger::DERIVED, &memoria::vapi::logger);

template<
        typename Types
>
ContainerMetadata* Ctr<VectorMapCtrTypes<Types> >::reflection_ = NULL;

}

#endif
