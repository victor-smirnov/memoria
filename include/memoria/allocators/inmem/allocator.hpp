
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef     _MEMORIA_MODULES_CONTAINERS_STREAM_POSIX_MANAGER_HPP
#define     _MEMORIA_MODULES_CONTAINERS_STREAM_POSIX_MANAGER_HPP

//#include <map>
#include <unordered_map>
#include <string>

#include <memoria/core/tools/stream.hpp>

#include <memoria/core/container/metadata_repository.hpp>

#include <memoria/core/tools/pool.hpp>

#include <malloc.h>



namespace memoria {


typedef struct
{
    template <typename T, size_t S>
    long operator() (const AbstractPageID<T,S> &k) const { return k.value(); }
} IDKeyHash;

typedef struct
{
    template <typename T, size_t S>
    bool operator() (const AbstractPageID<T,S> &x, const AbstractPageID<T,S> &y) const { return x == y; }
} IDKeyEq;

using namespace memoria::vapi;

template <typename Profile, typename PageType, typename TxnType>
class InMemAllocator: public AbstractAllocatorFactory<Profile, AbstractAllocatorName<PageType, MAX_BLOCK_SIZE> >::Type {

    typedef IAllocator<PageType>                                        Base;
    typedef InMemAllocator<Profile, PageType, TxnType>                          Me;

public:
    typedef typename Base::Page                                                 Page;
    typedef typename Base::PageG                                                PageG;
    typedef typename Base::Shared                                               Shared;
    typedef typename Base::CtrShared                                            CtrShared;
    typedef typename Page::ID                                                   ID;
    static const Int PAGE_SIZE                                                  = MAX_BLOCK_SIZE;

    typedef Base                                                                AbstractAllocator;

    typedef Ctr<typename CtrTF<Profile, RootCtr>::CtrTypes>                     RootMapType;
    typedef typename RootMapType::Metadata                                      RootMetatata;
    typedef typename RootMapType::BTreeCtrShared                                RootCtrShared;

private:
    typedef InMemAllocator<Profile, PageType, TxnType>                          MyType;

    struct PageOp
    {
        enum {UPDATE = Shared::UPDATE, DELETE = Shared::DELETE, NONE};
        ID id_;
        Page* page_;
        Int op_;

        PageOp(Page* page): id_(page->id()), page_(page), op_(UPDATE)   {}
        PageOp(const ID& id): id_(id), page_(NULL), op_(DELETE)         {}
        PageOp(): id_(), page_(NULL), op_(NONE)                         {}
    };

    typedef std::unordered_map<ID, Page*, IDKeyHash, IDKeyEq>                   IDPageMap;
    typedef std::unordered_map<ID, PageOp, IDKeyHash, IDKeyEq>                  IDPageOpMap;
    typedef std::unordered_map<BigInt, CtrShared*>                              CtrSharedMap;


    IDPageMap           pages_;
    IDPageOpMap         pages_log_;
    CtrSharedMap        ctr_shared_;

    Logger              logger_;
    Int                 counter_;
    ContainerMetadataRepository*    metadata_;

    MyType&             me_;
    const char*         type_name_;
    BigInt              allocs1_;
    BigInt              allocs2_;

    Me*                 roots_;

    RootMapType*        root_map_;

    StaticPool<ID, Shared>  pool_;

public:
    InMemAllocator() :
        logger_("memoria::StreamAllocator", Logger::DERIVED, &memoria::vapi::logger),
        counter_(100), metadata_(MetadataRepository<Profile>::getMetadata()), me_(*this),
        type_name_("StreamAllocator"), allocs1_(0), allocs2_(0), roots_(this)
    {
        root_map_ = new RootMapType(this, 0, true);
    }

    InMemAllocator(const InMemAllocator& other):
            logger_(other.logger_),
            counter_(other.counter_), metadata_(other.metadata_), me_(*this),
            type_name_("StreamAllocator"), allocs1_(other.allocs1_), allocs2_(other.allocs2_), roots_(this),
            pool_(other.pool_)
    {
        for (auto i = other.pages_.begin(); i != other.pages_.end(); i++)
        {
            char* buffer = (char*) malloc(PAGE_SIZE);
            CopyBuffer(i->second, buffer, PAGE_SIZE);
            pages_[i->first] = T2T<Page*>(buffer);
        }

        root_map_ = new RootMapType(this, 0);
    }

    virtual ~InMemAllocator() throw ()
    {
        try {
            delete root_map_;

            Int npages = 0;
            for (auto i = pages_.begin(); i != pages_.end(); i++)
            {
                MEMORIA_TRACE(me(), "Free Page", i->second);
                ::free(i->second);
                npages++;
            }

            //FIXME: clear pages_log_
            //FIXME: clear ctr_shared_

            if (allocs1_ - npages > 0)
            {
                MEMORIA_ERROR(me(), "Page leak detected:", npages, allocs1_, (allocs1_ - npages));
            }
        }
        catch (...) {
        }
    }

    ContainerMetadataRepository* getMetadata() const {
        return metadata_;
    }

    virtual Logger& logger() {
        return logger_;
    }

    virtual Logger* getLogger() {
        return &logger_;
    }

    RootMapType* roots() {
        return root_map_;
    }

    const ID &root() const
    {
        return root_map_->root();
    }


    virtual void releasePage(Shared* shared)
    {
        pool_.release(shared->id());
    }




    virtual PageG getPage(const ID& id, Int flags)
    {
        if (id.isNull())
        {
            return PageG();
        }


        PageOp op = get_in_log(id);

        if (flags == Base::READ)
        {
            if (op.op_ != PageOp::NONE)
            {
                Shared* shared = get_shared(op.page_, op.op_);
                return PageG(shared);
            }
            else
            {
                Page*   page = get0(id);
                Shared* shared = get_shared(page, Shared::READ);
                return PageG(shared);
            }
        }
        else {
            if (op.op_ == PageOp::NONE)
            {
                Page* page = get0(id);

                char* buffer = (char*) malloc(PAGE_SIZE);
                allocs1_++;
                CopyBuffer(page, buffer, PAGE_SIZE);
                Page* page2 = T2T<Page*>(buffer);

                pages_log_[id] = page2;

                Shared* shared = pool_.get(id);

                if (shared == NULL)
                {
                    shared = pool_.allocate(id);

                    shared->set_allocator(this);
                    shared->id() = id;
                }

                shared->set_page(page2);
                shared->state() = Shared::UPDATE;

                return PageG(shared);
            }
            else
            {
                return PageG(get_shared(op.page_, op.op_));
            }
        }
    }

    virtual PageG getPageG(Page* page)
    {
        return getPage(page->id(), Base::READ);
    }

    virtual void updatePage(Shared* shared)
    {
        if (shared->state() == Shared::READ)
        {
            char* buffer = (char*) malloc(PAGE_SIZE);
            allocs1_++;

            CopyBuffer(shared->get(), buffer, PAGE_SIZE);
            Page* page0 = T2T<Page*>(buffer);

            pages_log_[page0->id()] = page0;

            shared->set_page(page0);
            shared->state() = Shared::UPDATE;
        }
    }

    virtual void  removePage(const ID& id)
    {
        Shared* shared = pool_.get(id);
        if (shared != NULL)
        {
            // FIXME it doesn't really necessary to inform PageGuards that the page is deleted
            shared->state() = Shared::DELETE;
        }

        auto i = pages_log_.find(id);
        if (i != pages_log_.end())
        {
            (i->second).op_ = PageOp::DELETE;
        }
        else {
            pages_log_[id] = PageOp(id);
        }
    }


    /**
     * If a tree page is created using new (allocator) PageType call
     * than Page() constructor is invoked twice with undefined results
     */
    virtual PageG createPage(Int initial_size = PAGE_SIZE)
    {
        allocs1_++;
        char* buf = (char*) malloc(PAGE_SIZE);
//      Clean(buf, PAGE_SIZE);

        for (int c = 0; c < PAGE_SIZE; c++)
        {
            buf[c] = 0;
        }

        ID id = counter_++;

        Page* p = new (buf) Page(id);

        pages_log_[id] = p;

        Shared* shared  = pool_.allocate(id);

        shared->id()        = id;
        shared->state()     = Shared::UPDATE;

        shared->set_page(p);
        shared->set_allocator(this);

        return PageG(shared);
    }

    void stat() {
        cout<<allocs1_<<" "<<allocs2_<<" "<<pages_.size()<<endl;
    }

//// FIXME: allocator clearing doesn't work
//  void clear()
//  {
//      pages_.clear();
//      pages_log_.clear();
//      ctr_shared_.clear();
//
//      //pool_.clear();
//
//      root_map_shared_.clear();
//
//      counter_    = 100;
//  }

    void commit()
    {
        for (auto i = pages_log_.begin(); i != pages_log_.end(); i++)
        {
            PageOp op = i->second;

            if (op.op_ == PageOp::UPDATE)
            {
                op.page_->set_updated(false);

                auto j = pages_.find(op.id_);
                if (j != pages_.end())
                {
                    ::free(j->second);
                    allocs2_--;
                }

                pages_[op.id_] = op.page_;

                Shared* page = pool_.get(op.id_);
                if (page != NULL)
                {
                    page->set_page(op.page_);
                }
            }
            else {
                auto j = pages_.find(op.id_);
                if (j != pages_.end())
                {
                    Shared* shared = pool_.get(op.id_);
                    if (shared != NULL)
                    {
                        ::free(shared->get());
                        shared->set_page((Page*)NULL);
                    }
                    else {
                        ::free(j->second);
                    }

                    pages_.erase(op.id_);
                    allocs2_--;
                }
            }
        }

        for (auto i = ctr_shared_.begin(); i != ctr_shared_.end(); i++)
        {
            i->second->commit();
        }

        pages_log_.clear();
    }

    void rollback()
    {
        for (auto i = pages_log_.begin(); i != pages_log_.end(); i++)
        {
            PageOp op = i->second;
            ::free(op.page_);
        }

        for (auto i = ctr_shared_.begin(); i != ctr_shared_.end(); i++)
        {
            i->second->rollback();
        }

        pages_log_.clear();
    }


    virtual void resizePage(Shared* page, Int new_size)
    {
    }

    virtual PageG getRoot(BigInt name, Int flags)
    {
        if (name == 0)
        {
            return getPage(root(), flags);
        }
        else {
            return getPage(roots_->get_value_for_key(name), flags);
        }
    }

    virtual ID getRootID(BigInt name)
    {
        if (name == 0)
        {
            return root();
        }
        else {
            return roots_->get_value_for_key(name);
        }
    }

    virtual void setRoot(BigInt name, const ID& root)
    {
        new_root(name, root);
    }

    virtual CtrShared* getCtrShared(BigInt name)
    {
        auto i = ctr_shared_.find(name);

        if (i != ctr_shared_.end())
        {
            return i->second;
        }
        else
        {
            throw Exception(MEMORIA_SOURCE, SBuf()<<"Unknown CtrShared requested for name "<<name);
        }
    }

    virtual void registerCtrShared(CtrShared* shared)
    {
        BigInt name = shared->name();

        auto i = ctr_shared_.find(name);

        if (i == ctr_shared_.end())
        {
            ctr_shared_[name] = shared;
        }
        else if (i->second == NULL)
        {
            i->second = shared;
        }
        else
        {
            throw Exception(MEMORIA_SOURCE, SBuf()<<"CtrShared for name "<<name<<" is already registered");
        }
    }

    virtual void unregisterCtrShared(CtrShared* shared)
    {
        ctr_shared_.erase(shared->name());
    }

    virtual bool isCtrSharedRegistered(BigInt name)
    {
        return ctr_shared_.find(name) != ctr_shared_.end();
    }

    virtual void load(InputStreamHandler *input)
    {
        //FIXME: clear allocator
        commit();
        //clear();

        char signature[12];

        MEMORIA_TRACE(me(),"read header from:", input->pos());
        input->read(signature, sizeof(signature));
        MEMORIA_TRACE(me(),"Current:", input->pos());

        if (!(
                signature[0] == 'M' &&
                signature[1] == 'E' &&
                signature[2] == 'M' &&
                signature[3] == 'O' &&
                signature[4] == 'R' &&
                signature[5] == 'I' &&
                signature[6] == 'A'))
        {
            //FIXME: Is signature a 0-terminated string?
            throw Exception(MEMORIA_SOURCE, SBuf()<<"The stream does not start from MEMORIA signature: "<<signature);
        }

        if (!(signature[7] == 0 || signature[7] == 1))
        {
            throw BoundsException(MEMORIA_SOURCE, SBuf()<<"Endiannes filed value is out of bounds "<<signature[7]);
        }

        if (signature[8] != 0)
        {
            throw Exception(MEMORIA_SOURCE, "This is not a stream container");
        }

        ID root(0);

        bool first = true;
        Short size;
        while (input->read(size))
        {
            char buf[PAGE_SIZE];
            for (Int c = 0; c < PAGE_SIZE; c++)
                buf[c] = 0;

            Int page_hash;

            MEMORIA_TRACE(me(),"File pos before reading page hash:", input->pos());
            input->read(page_hash);

            MEMORIA_TRACE(me(),"Page size", size, "from", input->pos(), "page_hash=", page_hash);
            input->read(buf, 0, size);

            Page* page = T2T<Page*>(buf);

            PageMetadata* pageMetadata = metadata_->getPageMetadata(page_hash);

            char* mem = new char[PAGE_SIZE];
            for (Int c = 0; c < PAGE_SIZE; c++)
            {
                mem[c] = 0;
            }

            pageMetadata->getPageOperations()->deserialize(page, size, mem);

            page = T2T<Page*>(mem);

            pages_[page->id()] = page;

            MEMORIA_TRACE(me(), "Register page", page, page->id());

            if (first)
            {
                root = page->id();
                first = false;
            }
        }

        Int maxId = -1;
        for (auto i = pages_.begin(); i != pages_.end(); i++)
        {
            Int idValue = i->second->id().value();
            if (idValue > maxId)
            {
                maxId = idValue;
            }
        }

        counter_ = maxId + 1;

        //FIXME: Is it safe?
        root_map_->initCtr(this, root);
    }

    virtual void store(OutputStreamHandler *output)
    {
        char signature[12] = "MEMORIA";
        for (UInt c = 7; c < sizeof(signature); c++) signature[c] = 0;

        output->write(&signature, 0, sizeof(signature));

        char buf[PAGE_SIZE];

        ID root = root_map_->shared()->root();

        if (!root.isNull())
        {
            dump_page(output, buf, pages_[root]);

            for (typename IDPageMap::iterator i = pages_.begin(); i!= pages_.end(); i++)
            {
                Page *page = i->second;
                if (page != NULL && !(page->id() == root))
                {
                    dump_page(output, buf, page);
                }
            }
        }

        output->close();
    }




public:
    bool is_log(Int level) {
        return logger_.isLogEnabled(level);
    }

    const char* typeName() {
        return type_name_;
    }

    // Allocator implementaion


    virtual memoria::vapi::Page* createPageWrapper()
    {
        return new PageWrapper<Page, PAGE_SIZE>();
    }

    virtual void getRootPageId(IDValue& id)
    {
        id = IDValue(&root_map_->root());
    }

    BigInt getPageCount() {
        return pages_.size();
    }

    void getPage(memoria::vapi::Page* page, const IDValue& idValue)
    {
        if (page == NULL)
        {
            throw NullPointerException(MEMORIA_SOURCE, "page must not be null");
        }

        Page* page0 =  this->get1(idValue);
        page->setPtr(page0);
    }



    MyType* me() {
        return static_cast<MyType*>(this);
    }

    const MyType* me() const {
        return static_cast<const MyType*>(this);
    }


    void dumpPages(ostream& out = cout)
    {
        for (auto i = pages_.begin(); i != pages_.end(); i++)
        {
            Page* page = i->second;
            PageMetadata* pageMetadata = metadata_->getPageMetadata(page->page_type_hash());

            PageWrapper<Page, PAGE_SIZE> pw(page);
            memoria::vapi::dumpPage(pageMetadata, &pw, out);
            out<<endl;
            out<<endl;
        }
    }

    bool check()
    {
        bool result = false;

        for (auto iter = this->roots()->Begin(); !iter.isEnd(); )
        {
            PageG page = this->getPage(iter.getValue(), Base::READ);

            ContainerMetadata* ctr_meta = metadata_->getContainerMetadata(page->model_hash());

            result = ctr_meta->getCtrInterface()->check(&page->id(), this) || result;

            iter.next();
        }

        return result;
    }


    virtual void* allocateMemory(size_t size)
    {
        return malloc(size);
    }

    virtual void freeMemory(void* ptr)
    {
        free(ptr);
    }

    virtual BigInt createCtrName()
    {
        RootMetatata meta = root_map_->getRootMetadata();

        BigInt new_name = ++meta.model_name_counter();

        root_map_->setRootMetadata(meta);

        return new_name;
    }

private:

    void dump_page(OutputStreamHandler *output, char* buf, Page *page)
    {
        if (page->page_type_hash() != 0)
        {
            if (page->references() > 0) {cout<<"dump "<<page->id()<<" "<<page->references()<<endl;}

            MEMORIA_TRACE(
                    me(),
                    "dump page with hashes",
                    page->page_type_hash(),
                    page->model_hash(),
                    "with id",
                    page->id(),
                    page,
                    &page->id());

            PageMetadata* pageMetadata = metadata_->getPageMetadata(page->page_type_hash());

            for (Int c = 0; c < PAGE_SIZE; c++)
            {
                buf[c] = 0;
            }

            const IPageOperations* operations = pageMetadata->getPageOperations();

            operations->serialize(page, buf);

            Int ptr = operations->getPageSize(page);

            Short size = ptr;

            output->write(size);
            output->write(page->page_type_hash());

            MEMORIA_TRACE(me(), "Page size", size, "at", output->pos(), page->page_type_hash());
            output->write(buf, 0, size);
        }
        else {
            MEMORIA_TRACE(me(), "hash for page", page->id(), "is not specified");
        }
    }

    void set_root(BigInt name, const ID &page_id)
    {
        if (name != 0)
        {
            roots_->set_value_for_key(name, page_id);
        }
    }

    void remove_root(BigInt name)
    {
        if (name != 0)
        {
            roots_->remove_by_key(name);
        }
    }

    virtual void new_root(BigInt name, const ID &page_id)
    {
        MEMORIA_TRACE(me(), "Register new root", page_id, "for", name);

        if (page_id.isNull())
        {
            remove_root(name);
        }
        else {
            set_root(name, page_id);
        }
    }

    PageOp get_in_log(const ID &page_id)
    {
        auto i = pages_log_.find(page_id);
        if (i != pages_log_.end())
        {
            return i->second;
        }
        else {
            return PageOp();
        }
    }

    Page *get0(const ID &page_id)
    {
        auto i = pages_.find(page_id);
        if (i != pages_.end())
        {
            if (i->second == NULL)
            {
                throw NullPointerException(MEMORIA_SOURCE, "Null page for the specified page_id");
            }
            return i->second;
        }
        else {
            throw NullPointerException(MEMORIA_SOURCE, SBuf()<<"Can't find page for the specified page_id: "<<page_id);
        }
    }

    Page *get1(const ID &page_id)
    {
        if (page_id.isNull())
        {
            return NULL;
        }
        else {
            return get0(page_id);
        }
    }

    // Begin RootMapInterface

    void set_root(const ID& id) {
        root_map_->set_root(id);
    }

    void remove_by_key(BigInt name)
    {
        root_map_->remove(name);
    }

    void set_value_for_key(BigInt name, const ID& page_id)
    {
        root_map_->operator[](name).setData(page_id);
    }

    ID get_value_for_key(BigInt name)
    {
        auto iter = root_map_->find(name);

        if (!iter.isEnd())
        {
            return iter.getValue();
        }
        else {
            return ID(0);
            //throw new Exception(MEMORIA_SOURCE, "Can't find Root ID for model " + toString(name));
        }
    }


    Shared* get_shared(Page* page, Int op)
    {
        Shared* shared = pool_.get(page->id());

        if (shared == NULL)
        {
            shared = pool_.allocate(page->id());

            shared->id()        = page->id();
            shared->state()     = op;
            shared->set_page(page);
            shared->set_allocator(this);
        }

        return shared;
    }
};

}



#endif
