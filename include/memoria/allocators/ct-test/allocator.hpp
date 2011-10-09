
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef		_MEMORIA_MODULES_CONTAINERS_STREAM_POSIX_MANAGER_HPP
#define		_MEMORIA_MODULES_CONTAINERS_STREAM_POSIX_MANAGER_HPP

#include <map>
#include <string>

#include <memoria/vapi.hpp>
#include <memoria/vapi/models/logs.hpp>

#include <memoria/core/container/collection.hpp>

#include <malloc.h>



namespace memoria {

using namespace memoria::vapi;

template <typename Profile, typename PageType, typename TxnType>
class CtTestAllocator: public AbstractAllocatorFactory<Profile, AbstractAllocatorName<PageType, 4096> >::Type /*, public AllocatorImpl*/ {

	typedef IAbstractAllocator<PageType>										Base;
	typedef CtTestAllocator<Profile, PageType, TxnType>							Me;

public:
	typedef typename Base::Page 												Page;
	typedef typename Page::ID 													ID;
	static const Int PAGE_SIZE 													= 4096;

	typedef Base 																AbstractAllocator;

	ID root_;

private:
	typedef Me 																	MyType;


	Logger logger_;

public:
	CtTestAllocator() :
		logger_("memoria::CtTestAllocator", Logger::DERIVED, &memoria::vapi::logger),
		root_(0)
	{

	}


	ContainerCollectionMetadata* GetMetadata() const {
		return NULL;
	}

	virtual Logger& logger() {
		return logger_;
	}

	virtual Logger* GetLogger() {
		return &logger_;
	}



	const ID &root() const {
		return root_;
	}

	virtual Page* for_update(Page *page) {
		return NULL;
	}

	/**
	 * If a tree page is created using new (allocator) PageType call
	 * than Page() constructor is invoked twice with undefined results
	 */
	virtual Page* create_new1()
	{
		return NULL;
	}



	virtual void free1(const ID &id)
	{

	}

	virtual void free1(Page *page) {

	}

	virtual Page *get1(const ID &page_id)
	{
		return NULL;
	}

	// Begin RootMapInterface

	void set_root(const ID& id) {

	}

	void remove_by_key(BigInt name) {

	}

	void set_value_for_key(BigInt name, const ID& page_id) {

	}

	ID get_value_for_key(BigInt name)
	{

	}

	// End RootMapInterface





	virtual void sync(const ID &page_id) {

	}

	Int get_page_size()
	{
		return PAGE_SIZE;
	}

	virtual Page* GetPage(const ID& id)	{
		return get1(id);
	}

	virtual void  RemovePage(const ID& id) {
		free1(id);
	}

	virtual Page* CreatePage(Int initial_size = PAGE_SIZE) {
		return this->create_new1();
	}

	virtual Page* ReallocPage(Page* page, Int new_size) {
		return page;
	}

	virtual Page* GetRoot(BigInt name) {
		return NULL;
	}

	virtual ID GetRootID(BigInt name) {
		return ID(name);
	}

	virtual void SetRoot(BigInt name, const ID& root) {
		new_root(name, root);
	}


	virtual void load(InputStreamHandler *input)
	{

	}

	virtual void store(OutputStreamHandler *output)
	{

	}

	void dump_page(OutputStreamHandler *output, char* buf, Page *page) {

	}

	void set_root(BigInt name, const ID &page_id) {

	}

	void remove_root(BigInt name) {

	}

	virtual void new_root(BigInt name, const ID &page_id) {

	}

	bool is_log(Int level) {


		return logger_.IsLogEnabled(level);
	}

	StringRef type_name() {
		return "";
	}





	// Allocator implementaion

	virtual ContainerCollection* GetContainer() {
		return NULL;//container_;
	}

	virtual void SetContainer(ContainerCollection* container) {
		throw MemoriaException(MEMORIA_SOURCE, "This Stream container doesn't allows manual page manager setup");
	}

	virtual memoria::vapi::Page* CreatePageWrapper()
	{
		return NULL;
	}

	virtual void GetRootPageId(IDValue& id)
	{

	}

	virtual void SetRootPageId(const IDValue& id)
	{

	}

	virtual BigInt GetPageCount() {
		return 0;
	}

	virtual void GetPage(memoria::vapi::Page* page, const IDValue& idValue)
	{

	}


	virtual void RemovePage(const IDValue& idValue)
	{

	}

	virtual void CreateNewPage(int flags, memoria::vapi::Page* page)
	{
	}

};

}

#endif
