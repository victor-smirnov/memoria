
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_ALLOCATORS_PERSISTENT_INMEM_PERSISTENT_TREE_TXN_HPP
#define _MEMORIA_ALLOCATORS_PERSISTENT_INMEM_PERSISTENT_TREE_TXN_HPP

#include <memoria/allocators/persistent-inmem/persistent_tree_node.hpp>

#include <memoria/core/container/allocator.hpp>
#include <memoria/core/container/metadata_repository.hpp>

#include <memoria/core/exceptions/memoria.hpp>

#include <memoria/core/tools/pool.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <memoria/containers/map/map_factory.hpp>

#include <vector>
#include <memory>

namespace memoria {

template <typename Profile, typename PageType>
class PersistentInMemAllocatorT;

namespace persistent_inmem {

namespace {

template <typename CtrT, typename Allocator>
class SharedCtr: public CtrT {

	std::shared_ptr<Allocator> allocator_;

public:
	SharedCtr(const std::shared_ptr<Allocator>& allocator, Int command, const UUID& name):
		CtrT(allocator.get(), command, name),
		allocator_(allocator)
	{}

	auto snapshot() const {
		return allocator_;
	}
};


}



template <typename Profile, typename PageType, typename HistoryNode, typename PersitentTree, typename HistoryTree>
class Snapshot:
		public IWalkableAllocator<PageType>,
		public std::enable_shared_from_this<
			Snapshot<Profile, PageType, HistoryNode, PersitentTree, HistoryTree>
		>
{
	using Base 				= IAllocator<PageType>;
	using MyType 			= Snapshot<Profile, PageType, HistoryNode, PersitentTree, HistoryTree>;

	using HistoryTreePtr 	= std::shared_ptr<HistoryTree>;
	using SnapshotPtr 		= std::shared_ptr<MyType>;

	using NodeBaseT			= typename PersitentTree::NodeBaseT;
	using LeafNodeT			= typename PersitentTree::LeafNodeT;
	using PTreeValue		= typename PersitentTree::LeafNodeT::Value;

	using Status 			= typename HistoryNode::Status;
public:

	template <typename CtrName>
	using CtrT = SharedCtr<typename CtrTF<Profile, CtrName>::Type, MyType>;

	template <typename CtrName>
	using CtrPtr = std::shared_ptr<CtrT<CtrName>>;

	using typename Base::Page;
	using typename Base::ID;
	using typename Base::PageG;
	using typename Base::Shared;


private:
	using RootMapType = Ctr<typename CtrTF<Profile, Map<UUID, ID>>::CtrTypes>;


	class Properties: public IAllocatorProperties {
	public:
		virtual Int defaultPageSize() const
		{
			return 4096;
		}

		virtual BigInt lastCommitId() const {
			return 0;
		}

		virtual void setLastCommitId(BigInt txn_id) {}

		virtual BigInt newTxnId() {return 0;}
	};

	HistoryNode* 	history_node_;
	HistoryTreePtr  history_tree_;
	HistoryTree*  	history_tree_raw_ = nullptr;

	PersitentTree persistent_tree_;

	StaticPool<ID, Shared, 256>  pool_;

	Logger logger_;

	Properties properties_;

	std::shared_ptr<RootMapType> root_map_;


	ContainerMetadataRepository*  metadata_;

	template <typename, typename>
	friend class ::memoria::PersistentInMemAllocatorT;


public:

	Snapshot(HistoryNode* history_node, const HistoryTreePtr& history_tree):
		history_node_(history_node),
		history_tree_(history_tree),
		history_tree_raw_(history_tree.get()),
		persistent_tree_(history_node_),
		logger_("PersistentInMemAllocatorSnp", Logger::DERIVED, &history_tree->logger_),
		root_map_(nullptr),
		metadata_(MetadataRepository<Profile>::getMetadata())
	{
		history_node_->ref();

		Int ctr_op;

		if (history_node->is_active())
		{
			history_tree_raw_->ref_active();
			ctr_op = CTR_CREATE | CTR_FIND;
		}
		else {
			ctr_op = CTR_FIND;
		}

		root_map_ = std::make_shared<RootMapType>(this, ctr_op, UUID());
	}

	Snapshot(HistoryNode* history_node, HistoryTree* history_tree):
		history_node_(history_node),
		history_tree_raw_(history_tree),
		persistent_tree_(history_node_),
		logger_("PersistentInMemAllocatorTxn"),
		root_map_(nullptr),
		metadata_(MetadataRepository<Profile>::getMetadata())
	{
		history_node_->ref();

		Int ctr_op;

		if (history_node->is_active())
		{
			history_tree_raw_->ref_active();
			ctr_op = CTR_CREATE | CTR_FIND;
		}
		else {
			ctr_op = CTR_FIND;
		}

		root_map_ = std::make_shared<RootMapType>(this, ctr_op, UUID());
	}

	virtual ~Snapshot()
	{
		if (history_node_->unref() == 0)
		{
			if (history_node_->is_active())
			{
				do_drop();

				history_tree_raw_->forget_snapshot(history_node_);
			}
			else if(history_node_->is_dropped())
			{
				check_tree_structure(history_node_->root());

				do_drop();
			}
		}
	}

	static void initMetadata() {
		RootMapType::initMetadata();
	}

	ContainerMetadataRepository* getMetadata() const {
		return metadata_;
	}

	const auto& uuid() const {
		return history_node_->txn_id();
	}

	bool is_active() const {
		return history_node_->is_active();
	}

	virtual bool isActive() {
		return is_active();
	}

	bool is_marked_to_clear() const {
		return history_node_->mark_to_clear();
	}

	bool is_committed() const {
		return history_node_->is_committed();
	}

	void commit()
	{
		if (history_node_->is_active())
		{
			history_node_->commit();
			history_tree_raw_->unref_active();
		}
		else {
			throw Exception(MA_SRC, SBuf()<<"Invalid Snapshot state: "<<(Int)history_node_->status());
		}
	}

	void drop()
	{
		if (history_node_->parent() != nullptr)
		{
			history_node_->mark_to_clear();
		}
		else {
			throw Exception(MA_SRC, "Can't drop root snapshot");
		}
	}

	bool drop_ctr(const UUID& name)
	{
		UUID root_id = getRootID(name);

		if (root_id.is_set())
		{
			PageG page = this->getPage(root_id, name);

			ContainerMetadata* ctr_meta = metadata_->getContainerMetadata(page->ctr_type_hash());

			ctr_meta->getCtrInterface()->drop(root_id, name, this);

			return true;
		}
		else {
			return false;
		}
	}

	void set_as_master()
	{
		history_tree_raw_->set_master(history_node_->txn_id());
	}

	void set_as_branch(StringRef name)
	{
		history_tree_raw_->set_branch(name, history_node_->txn_id());
	}

	StringRef metadata() const {
		return history_node_->metadata();
	}

	void set_metadata(StringRef metadata)
	{
		if (history_node_->is_active())
		{
			history_node_->metadata() = metadata;
		}
		else
		{
			throw Exception(MA_SRC, "Snapshot is already committed.");
		}
	}

	SnapshotPtr branch()
	{
		if (history_node_->is_committed())
		{
			HistoryNode* history_node = new HistoryNode(history_node_);
			history_tree_raw_->snapshot_map_[history_node->txn_id()] = history_node;

			return std::make_shared<Snapshot>(history_node, history_tree_->shared_from_this());
		}
		else
		{
			throw Exception(MA_SRC, "Snapshot is still being active. Commit it first.");
		}
	}

	bool has_parent() const {
		return history_node_->parent() != nullptr;
	}

	SnapshotPtr parent()
	{
		if (history_node_->parent())
		{
			HistoryNode* history_node = history_node_->parent();
			return std::make_shared<Snapshot>(history_node, history_tree_->shared_from_this());
		}
		else
		{
			throw Exception(MA_SRC, "Snapshot has no parent.");
		}
	}

	void join(SnapshotPtr txn) {
		//TODO
	}

	virtual PageG getPage(const ID& id, const UUID& name)
	{
		if (id.isSet())
		{
			Shared* shared = get_shared(id, Shared::READ);

			if (!shared->get())
			{
				checkReadAllowed();

				auto page_opt = persistent_tree_.find(id);

				if (page_opt)
				{
					const auto& txn_id = history_node_->txn_id();

					if (page_opt.value().txn_id() != txn_id)
					{
						shared->state() = Shared::READ;
					}
					else {
						shared->state() = Shared::UPDATE;
					}

					shared->set_page(page_opt.value().page());
				}
				else {
					persistent_tree_.dump();

					persistent_tree_.find(id);

					throw Exception(MA_SRC, SBuf()<<"Page is not found for the specified id: "<<id);
				}
			}

			return PageG(shared);
		}
		else {
			return PageG();
		}
	}

	void dumpAccess(const char* msg, ID id, const Shared* shared)
	{
		cout<<msg<<": "<<id<<" "<<shared->get()<<" "<<shared->get()->uuid()<<" "<<shared->state()<<endl;
	}

	virtual PageG getPageForUpdate(const ID& id, const UUID& name)
	{
		// FIXME: Though this check prohibits new page acquiring for update,
		// already acquired updatable pages can be updated further.
		// To guarantee non-updatability, MMU-protection should be used
		checkUpdateAllowed();

		if (id.isSet())
		{
			Shared* shared = get_shared(id, Shared::UPDATE);

			if (!shared->get())
			{
				auto page_opt = persistent_tree_.find(id);

				if (page_opt)
				{
					const auto& txn_id = history_node_->txn_id();

					if (page_opt.value().txn_id() != txn_id)
					{
						Page* new_page = clone_page(page_opt.value().page());

						set_page(new_page);

						shared->set_page(new_page);

						shared->refresh();
					}
					else {
						MEMORIA_ASSERT(shared->state(), ==, Shared::UPDATE);

						shared->set_page(page_opt.value().page());

						shared->refresh();
					}
				}
				else {
					persistent_tree_.dump();

					throw Exception(MA_SRC, SBuf()<<"Page is not found for the specified id: "<<id);
				}
			}
			else if (shared->state() == Shared::READ)
			{
				auto page_opt = persistent_tree_.find(id);

				if (page_opt)
				{
					Page* new_page = clone_page(page_opt.value().page());

					set_page(new_page);
					shared->set_page(new_page);

					shared->refresh();
				}
				else {
					throw Exception(MA_SRC, SBuf()<<"Page is not found for the specified id: "<<id);
				}
			}
			else if (shared->state() == Shared::UPDATE)
			{
				//MEMORIA_ASEERT();
			}
			else {
				throw Exception(MA_SRC, SBuf()<<"Invalid PageShared state: "<<shared->state());
			}

			shared->state() = Shared::UPDATE;

			return PageG(shared);
		}
		else {
			return PageG();
		}
	}



	virtual PageG updatePage(Shared* shared, const UUID& name)
	{
		// FIXME: Though this check prohibits new page acquiring for update,
		// already acquired updatable pages can be updated further.
		// To guarantee non-updatability, MMU-protection should be used
		checkUpdateAllowed();

        if (shared->state() == Shared::READ)
        {
        	Page* new_page = clone_page(shared->get());

        	set_page(new_page);

        	shared->set_page(new_page);

            shared->state() = Shared::UPDATE;

            shared->refresh();
        }

        return PageG(shared);
	}

	virtual void removePage(const ID& id, const UUID& name)
	{
		checkUpdateAllowed();

		auto iter = persistent_tree_.locate(id);

		if (!iter.is_end())
		{
			auto shared = pool_.get(id);

			if (!shared)
			{
				persistent_tree_.remove(iter);
			}
			else {
				shared->state() = Shared::DELETE;
			}
		}
	}






	virtual PageG createPage(Int initial_size, const UUID& name)
	{
		checkUpdateAllowed();

		if (initial_size == -1)
		{
			initial_size = properties_.defaultPageSize();
		}

		void* buf = this->malloc(initial_size);

		memset(buf, 0, initial_size);

		ID id = newId();

		Page* p = new (buf) Page(id);

		p->page_size() = initial_size;

		Shared* shared  = pool_.allocate(id);

		shared->id()    = id;
		shared->state()	= Shared::UPDATE;

		shared->set_page(p);
		shared->set_allocator(this);

		set_page(p);



		return PageG(shared);
	}


	virtual void resizePage(Shared* shared, Int new_size)
	{
		checkUpdateAllowed();

		if (shared->state() == Shared::READ)
		{
			Page* page = shared->get();
			PageMetadata* pageMetadata = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

			Page* new_page = T2T<Page*>(this->malloc(new_size));

			pageMetadata->getPageOperations()->resize(page, new_page, new_size);

			shared->set_page(new_page);

			set_page(new_page);
		}
		else if (shared->state() == Shared::UPDATE)
		{
			Page* page = shared->get();
			PageMetadata* pageMetadata = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

			Page* new_page  = T2T<Page*>(realloc(page, new_size));

			pageMetadata->getPageOperations()->resize(page, new_page, new_size);

			shared->set_page(new_page);

			set_page(new_page);
		}
	}

	virtual void releasePage(Shared* shared) noexcept
	{
		if (shared->state() == Shared::DELETE)
		{
			persistent_tree_.remove(shared->get()->id());
		}

		pool_.release(shared->id());
	}

	virtual PageG getPageG(Page* page)
	{
		throw Exception(MA_SRC, "Method getPageG is not implemented for this allocator");
	}


	virtual ID newId() {
		return history_tree_raw_->newId();
	}

	virtual UUID currentTxnId() const {
		return history_node_->txn_id();
	}

	// memory pool allocator
	virtual void* allocateMemory(size_t size) {
		return ::malloc(size);
	}
	virtual void  freeMemory(void* ptr) {
		::free(ptr);
	}

	virtual Logger& logger() {return logger_;}
	virtual IAllocatorProperties& properties() {
		return properties_;
	}

	virtual ID getRootID(const UUID& name)
	{
		if (!name.is_null())
		{
			auto iter = root_map_->find(name);

			if (iter->is_found(name))
			{
				return iter->value();
			}
			else {
				return ID();
			}
		}
		else {
			return history_node_->root_id();
		}
	}

	virtual void setRoot(const UUID& name, const ID& root)
	{
		if (root.is_null())
		{
			if (!name.is_null())
			{
				root_map_->remove(name);
			}
			else {
				throw Exception(MA_SRC, SBuf()<<"Allocator directory removal attempted");
			}
		}
		else {
			if (!name.is_null())
			{
				root_map_->assign(name, root);
			}
			else {
				history_node_->root_id() = root;
			}
		}
	}

	virtual void markUpdated(const UUID& name) {}

	virtual bool hasRoot(const UUID& name)
	{
		if (!name.is_null())
		{
			auto iter = root_map_->find(name);
			return iter->is_found(name);
		}
		else {
			return !history_node_->root_id().is_null();
		}
	}

	virtual UUID createCtrName()
	{
        return UUID::make_random();
	}


	virtual bool check()
	{
		bool result = false;

		for (auto iter = root_map_->begin(); !iter->is_end(); )
		{
			auto ctr_name = iter->key();

			PageG page = this->getPage(iter->value(), ctr_name);

			ContainerMetadata* ctr_meta = metadata_->getContainerMetadata(page->ctr_type_hash());

			result = ctr_meta->getCtrInterface()->check(&page->id(), ctr_name, this) || result;

			iter->next();
		}

		return result;
	}

	virtual void walkContainers(ContainerWalker* walker, const char* allocator_descr = nullptr)
	{
        walker->beginSnapshot((SBuf()<<"Snapshot-"<<history_node_->txn_id()).str().c_str());

        auto iter = root_map_->Begin();

        while (!iter->isEnd())
        {
            auto ctr_name   = iter->key();
            auto root_id    = iter->value();

            auto page       = this->getPage(root_id, ctr_name);

            Int master_hash = page->master_ctr_type_hash();
            Int ctr_hash    = page->ctr_type_hash();

            ContainerMetadata* ctr_meta = metadata_->getContainerMetadata(master_hash != 0 ? master_hash : ctr_hash);

            ctr_meta->getCtrInterface()->walk(&page->id(), ctr_name, this, walker);

            iter->next();
        }

        walker->endSnapshot();
	}

	void dump_persistent_tree() {
		persistent_tree_.dump();
	}



	template <typename CtrName>
	auto find_or_create(const UUID& name)
	{
		return std::make_shared<CtrT<CtrName>>(this->shared_from_this(), CTR_FIND | CTR_CREATE, name);
	}

	template <typename CtrName>
	auto create(const UUID& name)
	{
		return std::make_shared<CtrT<CtrName>>(this->shared_from_this(), CTR_CREATE, name);
	}

	template <typename CtrName>
	auto create()
	{
		return std::make_shared<CtrT<CtrName>>(this->shared_from_this(), CTR_CREATE, CTR_DEFAULT_NAME);
	}

	template <typename CtrName>
	auto find(const UUID& name)
	{
		return std::make_shared<CtrT<CtrName>>(this->shared_from_this(), CTR_FIND, name);
	}


protected:

	Page* clone_page(const Page* page)
	{
		char* buffer = (char*) this->malloc(page->page_size());

		CopyByteBuffer(page, buffer, page->page_size());
		Page* new_page = T2T<Page*>(buffer);

		new_page->uuid() 		= newId();
		new_page->references() 	= 0;

		return new_page;
	}

	Shared* get_shared(Page* page)
	{
		MEMORIA_ASSERT_TRUE(page != nullptr);

		Shared* shared = pool_.get(page->id());

		if (shared == NULL)
		{
			shared = pool_.allocate(page->id());

			shared->id()        = page->id();
			shared->state()     = Shared::UNDEFINED;
			shared->set_page(page);
			shared->set_allocator(this);
		}

		return shared;
	}

	Shared* get_shared(const ID& id, Int state)
	{
		Shared* shared = pool_.get(id);

		if (shared == NULL)
		{
			shared = pool_.allocate(id);

			shared->id()        = id;
			shared->state()     = state;
			shared->set_page((Page*)nullptr);
			shared->set_allocator(this);
		}

		return shared;
	}

    void* malloc(size_t size)
    {
    	return ::malloc(size);
    }

    void set_page(Page* page)
    {
    	page->ref();

    	const auto& txn_id = history_node_->txn_id();
    	using Value = typename PersitentTree::Value;
    	auto old_value = persistent_tree_.assign(page->id(), Value(page, txn_id));

    	if (old_value.page()) {
    		old_value.page()->unref();
    	}
    }


    void checkReadAllowed()
    {
    	if (!history_node_->root())
    	{
    		throw Exception(MA_SRC, "Snapshot has been already cleared");
    	}
    }

    void checkUpdateAllowed()
    {
    	checkReadAllowed();

    	if (!history_node_->is_active())
    	{
    		throw Exception(MA_SRC, "Snapshot has been already committed");
    	}
    }


	void do_drop() throw ()
	{
		persistent_tree_.delete_tree([&](LeafNodeT* leaf){
			for (Int c = 0; c < leaf->size(); c++)
			{
				auto& page_descr = leaf->data(c);
				if (page_descr.page()->unref() == 0)
				{
					auto shared = pool_.get(page_descr.page()->id());

					if (!shared)
					{
						::free(page_descr.page());
					}
					else {
						shared->state() = Shared::DELETE;
					}
				}
			}
		});
	}

	static void delete_snapshot(HistoryNode* node) throw ()
	{
		PersitentTree persistent_tree(node);

		persistent_tree.delete_tree([&](LeafNodeT* leaf){
			for (Int c = 0; c < leaf->size(); c++)
			{
				auto& page_descr = leaf->data(c);
				if (page_descr.page()->unref() == 0)
				{
					::free(page_descr.page());
				}
			}
		});

		node->assign_root_no_ref(nullptr);
		node->root_id() = ID();
	}


	void check_tree_structure(const NodeBaseT* node)
	{
//		if (node->txn_id() == history_node_->txn_id())
//		{
//			if (node->refs() != 1)
//			{
//				cerr << "NodeRefProblem1 for: " << endl;
//				node->dump(cerr);
//			}
//		}
//		else {
//			if (node->refs() < 1)
//			{
//				cerr << "NodeRefProblem2 for: " << endl;
//				node->dump(cerr);
//			}
//		}
//
//		if (node->is_leaf())
//		{
//			//auto leaf_node = PersitentTree::to_leaf_node(node);
//		}
//		else {
//			auto branch_node = PersitentTree::to_branch_node(node);
//
//			for (Int c = 0; c < branch_node->size(); c++)
//			{
//				check_tree_structure(branch_node->data(c));
//			}
//		}
	}
};

}

template <typename CtrName, typename Profile, typename PageType, typename HistoryNode, typename PersitentTree, typename HistoryTree>
auto create(const std::shared_ptr<persistent_inmem::Snapshot<Profile, PageType, HistoryNode, PersitentTree, HistoryTree>>& alloc, const UUID& name)
{
	return alloc->template create<CtrName>(name);
}

template <typename CtrName, typename Profile, typename PageType, typename HistoryNode, typename PersitentTree, typename HistoryTree>
auto create(const std::shared_ptr<persistent_inmem::Snapshot<Profile, PageType, HistoryNode, PersitentTree, HistoryTree>>& alloc)
{
	return alloc->template create<CtrName>();
}

template <typename CtrName, typename Profile, typename PageType, typename HistoryNode, typename PersitentTree, typename HistoryTree>
auto find_or_create(const std::shared_ptr<persistent_inmem::Snapshot<Profile, PageType, HistoryNode, PersitentTree, HistoryTree>>& alloc, const UUID& name)
{
	return alloc->template find_or_create<CtrName>(name);
}

template <typename CtrName, typename Profile, typename PageType, typename HistoryNode, typename PersitentTree, typename HistoryTree>
auto find(const std::shared_ptr<persistent_inmem::Snapshot<Profile, PageType, HistoryNode, PersitentTree, HistoryTree>>& alloc, const UUID& name)
{
	return alloc->template find<CtrName>(name);
}


template <typename Allocator>
void check_snapshot(const std::shared_ptr<Allocator>& allocator, const char* message,  const char* source)
{
    Int level = allocator->logger().level();

    allocator->logger().level() = Logger::ERROR;

    if (allocator->check())
    {
        allocator->logger().level() = level;

        throw Exception(source, message);
    }

    allocator->logger().level() = level;
}

template <typename Allocator>
void check_snapshot(const std::shared_ptr<Allocator>& allocator)
{
	check_snapshot(allocator, "Snapshot check failed", MA_RAW_SRC);
}



}



#endif
