
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_MODEL_HPP
#define _MEMORIA_CORE_CONTAINER_MODEL_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/metadata/container.hpp>

#include <memoria/core/container/logs.hpp>
#include <memoria/core/container/names.hpp>
#include <memoria/core/container/builder.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/dispatcher.hpp>
#include <memoria/core/container/defaults.hpp>
#include <memoria/core/container/macros.hpp>
#include <memoria/core/container/init.hpp>

#include <string>



#define MEMORIA_MODEL_METHOD_IS_NOT_IMPLEMENTED() throw Exception(MEMORIA_SOURCE, SBuf()<<"Method is not implemented for "\
        <<me()->typeName())

namespace memoria    {

template <typename Profile, typename SelectorType, typename ContainerTypeName> class CtrTF;

template <typename Name, typename Base, typename Types> class CtrPart;

template <typename Types> class Ctr;
template <typename Types> class Iter;

template <typename Profile> class MetadataRepository;

template <typename Allocator>
struct IParentCtrInterface
{
    typedef typename Allocator::CtrShared                                       CtrShared;
    typedef typename Allocator::Page::ID                                        ID;

    virtual ID    getRootID(void* caller, BigInt name)                  = 0;
    virtual void  setRootID(void* caller, BigInt name, const ID& root)  = 0;

    virtual Allocator& getAllocator()                                   = 0;
    virtual CtrShared* getShared()                                      = 0;
};


template <typename TypesType>
class ContainerBase: public TypesType::Allocator {
public:

    typedef ContainerBase<TypesType>                                            ThisType;
    typedef Ctr<TypesType>                                                      MyType;

    typedef typename TypesType::ContainerTypeName                               ContainerTypeName;
    typedef ContainerTypeName                                                   Name;
    typedef TypesType                                                           Types;

    typedef typename Types::Allocator                                           Allocator;
    typedef typename Allocator::ID                                              ID;
    typedef typename Allocator::Page                                            Page;
    typedef typename Allocator::PageG                                           PageG;
    typedef typename Allocator::Page::ID                                        PageId;
    typedef typename Allocator::CtrShared                                       CtrShared;


    typedef Iter<typename Types::IterTypes>                                     Iterator;
    
    static const Int CONTAINER_HASH                                             = TypeHash<Name>::Value;

protected:
    static ContainerMetadata*   reflection_;

    CtrShared* shared_;

public:
    ContainerBase(): shared_(nullptr)
    {}

    ContainerBase(const ThisType& other):
        shared_(other.shared_)
    {}

    ContainerBase(const ThisType& other, Allocator* allocator):
        shared_(other.shared_)
    {}

    //shared_ is configured in move constructors of subclasses.
    ContainerBase(ThisType&& other):
        shared_(other.shared_)
    {
        other.shared_ = NULL;
    }

    ContainerBase(ThisType&& other, Allocator* allocator):
        shared_(other.shared_)
    {
        other.shared_ = NULL;
    }

    virtual ~ContainerBase() throw () {}

    void operator=(ThisType&& other)
    {
        shared_ = other.shared_;
        other.shared_ = NULL;
    }

    void operator=(const ThisType& other)
    {
        shared_ = other.shared_;
    }

    MEMORIA_PUBLIC static Int hash() {
        return CONTAINER_HASH;
    }


    MEMORIA_PUBLIC static ContainerMetadata* getMetadata()
    {
        return reflection_;
    }
    
    static void destroyMetadata()
    {
        if (reflection_ != NULL)
        {
            delete reflection_->getCtrInterface();

            MetadataRepository<typename Types::Profile>::unregisterMetadata(reflection_);

            delete reflection_;
            reflection_ = NULL;
        }
    }


    struct CtrInterfaceImpl: public ContainerInterface {

        virtual bool check(const void* id, void* allocator) const
        {
            Allocator* alloc = T2T<Allocator*>(allocator);
            ID* root_id = T2T<ID*>(id);

            MyType ctr(alloc, *root_id);
            return ctr.check(NULL);
        }
    };


    static ContainerInterface* getContainerInterface()
    {
        return new CtrInterfaceImpl();
    }

    static Int initMetadata(Int salt = 0)
    {
        if (reflection_ == NULL)
        {
            MetadataList list;

            Types::Pages::NodeDispatcher::buildMetadataList(list);

            PageInitDispatcher<typename Types::DataPagesList>::buildMetadataList(list);

            reflection_ = new ContainerMetadata(TypeNameFactory<Name>::name(),
                                                list,
                                                CONTAINER_HASH,
                                                MyType::getContainerInterface());

            MetadataRepository<typename Types::Profile>::registerMetadata(reflection_);
        }

        return reflection_->hash();
    }

    PageG createRoot() {
        return PageG();
    }

    BigInt getModelName(ID root_id)
    {
        return -1;
    }

    CtrShared* createCtrShared(BigInt name)
    {
        return new (&me()->allocator()) CtrShared(name);
    }

    CtrShared* getOrCreateCtrShared(BigInt name)
    {
        if (me()->allocator().isCtrSharedRegistered(name))
        {
            return me()->allocator().getCtrShared(name);
        }
        else {
            CtrShared* shared = me()->createCtrShared(name);
            me()->allocator().registerCtrShared(shared);

            PageG node = me()->allocator().getRoot(name, Allocator::READ);

            if (node.is_updated())
            {
                shared->root_log() = node->id();
                shared->updated() = true;
            }
            else {
                shared->root() = node->id();
                shared->updated() = false;
            }

            me()->configureNewCtrShared(shared, node);

            return shared;
        }
    }

    void configureNewCtrShared(CtrShared* shared, PageG root) const {}

    void removeCtrShared(CtrShared* shared)
    {
        shared->~CtrShared();
        me()->allocator().freeMemory(shared);
    }

    const CtrShared* shared() const {
        return shared_;
    }

    CtrShared* shared() {
        return shared_;
    }

    void initCtr(bool create) {}
    void initCtr(const ID& root_id) {}

protected:

    /**
     * \brief Set container reflection metadata.
     */

    static void setMetadata(ContainerMetadata* metadata)
    {
        reflection_ = metadata;
    }

    void setCtrShared(CtrShared* shared)
    {
        this->shared_ = shared;
    }

private:

    MyType* me() {
        return static_cast<MyType*>(this);
    }

    const MyType* me() const {
        return static_cast<const MyType*>(this);
    }
};

template <typename TypesType>
ContainerMetadata* ContainerBase<TypesType>::reflection_ = NULL;



template <int Idx, typename Types>
class CtrHelper: public CtrPart<
                            typename SelectByIndexTool<Idx, typename Types::List>::Result,
                            CtrHelper<Idx - 1, Types>,
                            Types>
{
    typedef CtrHelper<Idx, Types>                               ThisType;
    typedef Ctr<Types>                                          MyType;
    typedef CtrPart<typename SelectByIndexTool<Idx, typename Types::List>::Result, CtrHelper<Idx - 1, Types>, Types> Base;

    typedef typename Types::Allocator Allocator0;

public:
    CtrHelper(): Base() {}
    CtrHelper(const ThisType& other): Base(other) {}
    CtrHelper(ThisType&& other): Base(std::move(other)) {}
    CtrHelper(ThisType&& other, Allocator0* allocator): Base(std::move(other), allocator)   {}
    CtrHelper(const ThisType& other, Allocator0* allocator): Base(other, allocator)         {}

    virtual ~CtrHelper() throw () {}

    void operator=(ThisType&& other) {
        Base::operator=(std::move(other));
    }

    void operator=(const ThisType& other) {
        Base::operator=(other);
    }
};

template <typename Types>
class CtrHelper<-1, Types>: public Types::template BaseFactory<Types>::Type {
    typedef CtrHelper<-1, Types>                                ThisType;
    typedef Ctr<Types>                                          MyType;
    typedef typename Types::template BaseFactory<Types>::Type   Base;

public:

    typedef typename Types::Allocator                           Allocator0;

    CtrHelper(): Base() {}
    CtrHelper(const ThisType& other): Base(other) {}
    CtrHelper(ThisType&& other): Base(std::move(other)) {}
    CtrHelper(ThisType&& other, Allocator0* allocator): Base(std::move(other), allocator)    {}
    CtrHelper(const ThisType& other, Allocator0* allocator): Base(other, allocator)          {}

    virtual ~CtrHelper() throw () {}

    void operator=(ThisType&& other) {
        Base::operator=(std::move(other));
    }

    void operator=(const ThisType& other) {
        Base::operator=(other);
    }
};


template <typename Types>
class CtrStart: public CtrHelper<ListSize<typename Types::List>::Value - 1, Types> {

    typedef CtrStart<Types>         ThisType;
    typedef Ctr<Types>              MyType;

    typedef CtrHelper<ListSize<typename Types::List>::Value - 1, Types> Base;

    typedef typename Types::Allocator                                   Allocator0;

public:
    CtrStart(): Base() {}
    CtrStart(const ThisType& other): Base(other) {}
    CtrStart(ThisType&& other): Base(std::move(other)) {}
    CtrStart(ThisType&& other, Allocator0* allocator): Base(std::move(other), allocator) {}
    CtrStart(const ThisType& other, Allocator0* allocator): Base(other, allocator)       {}

    void operator=(ThisType&& other) {
        Base::operator=(std::move(other));
    }

    void operator=(const ThisType& other) {
        Base::operator=(other);
    }
};


extern Int CtrRefCounters;
extern Int CtrUnrefCounters;


template <typename Types>
class Ctr: public CtrStart<Types> {
    typedef CtrStart<Types>                                                     Base;
public:
    typedef Ctr<Types>                                                          MyType;

    typedef typename Types::Allocator                                           Allocator;
    typedef typename Types::Allocator::CtrShared                                CtrShared;
    typedef typename Types::Allocator::PageG                                    PageG;
    typedef typename PageG::Page::ID                                            ID;

public:

    typedef typename Types::ContainerTypeName                                   ContainerTypeName;
    typedef ContainerTypeName                                                   Name;

private:

    Allocator*  allocator_;
    BigInt      name_;
    const char* model_type_name_;

    Logger          logger_;
    static Logger   class_logger_;

    bool        debug_;

public:

    MEMORIA_PUBLIC Ctr(Allocator* allocator):
        Base(),
        allocator_(allocator),
        model_type_name_(TypeNameFactory<ContainerTypeName>::cname()),
        debug_(false)
    {
        MEMORIA_ASSERT_NOT_NULL(allocator);

        initLogger();

        initCtr(allocator, allocator->createCtrName(), true, model_type_name_);
    }


    MEMORIA_PUBLIC Ctr(Allocator* allocator, BigInt name, bool create = false, const char* mname = NULL):
        Base(),
        allocator_(allocator),
        model_type_name_(mname != NULL ? mname : TypeNameFactory<ContainerTypeName>::cname()),
        debug_(false)
    {
        MEMORIA_ASSERT_NOT_NULL(allocator);

        initLogger();

        initCtr(allocator, name, create, mname);
    }

    MEMORIA_PUBLIC Ctr(Allocator* allocator, const ID& root_id, const char* mname = NULL):
        Base(),
        allocator_(allocator),
        name_(-1),
        model_type_name_(mname != NULL ? mname : TypeNameFactory<ContainerTypeName>::cname()),
        debug_(false)
    {
        MEMORIA_ASSERT_NOT_NULL(allocator);

        initLogger();

        initCtr(allocator, root_id, mname);
    }

    MEMORIA_PUBLIC Ctr(const MyType& other):
        Base(other, other.allocator_),
        allocator_(other.allocator_),
        model_type_name_(other.model_type_name_),
        logger_(other.logger_),
        debug_(other.debug_)
    {
        Base::setCtrShared(other.shared_);
        ref();
    }

    MEMORIA_PUBLIC Ctr(const MyType& other, Allocator* allocator):
        Base(other, allocator),
        allocator_(allocator),
        model_type_name_(other.model_type_name_),
        logger_(other.logger_),
        debug_(other.debug_)
    {
        MEMORIA_ASSERT_NOT_NULL(allocator);

        Base::setCtrShared(other.shared_);
        ref();
    }


    MEMORIA_PUBLIC Ctr(MyType&& other):
        Base(std::move(other), other.allocator_),
        allocator_(other.allocator_),
        model_type_name_(other.model_type_name_),
        logger_(other.logger_),
        debug_(other.debug_)
    {}

    Ctr(MyType&& other, Allocator* allocator):
        Base(std::move(other), allocator),
        allocator_(allocator),
        model_type_name_(other.model_type_name_),
        logger_(other.logger_),
        debug_(other.debug_)
    {
        MEMORIA_ASSERT_NOT_NULL(allocator);
    }

    MEMORIA_PUBLIC Ctr(const NoParamCtr&):
        Base(),
        allocator_(NULL),
        model_type_name_(TypeNameFactory<ContainerTypeName>::cname()),
        logger_(model_type_name_, Logger::DERIVED, NULL),
        debug_(false)
    {
        Base::setCtrShared(NULL);
    }

    MEMORIA_PUBLIC virtual ~Ctr() throw()
    {
        unref();
    }

    void initLogger(const Logger* other)
    {
        logger_.setCategory(other->category());
        logger_.setHandler(other->getHandler());
        logger_.setParent(other->getParent());

        logger_.level() = other->level();
    }

    void initLogger()
    {
        logger_.configure(model_type_name_, Logger::DERIVED, &allocator_->logger());
    }

    void initCtr(Allocator* allocator, BigInt name, bool create = false, const char* mname = NULL)
    {
        MEMORIA_ASSERT(name, >=, 0);

        allocator_          = allocator;
        name_               = name;
        model_type_name_    = mname != NULL ? mname : TypeNameFactory<ContainerTypeName>::cname();
        //FIXME: init logger correctly

        Base::initCtr(create);

        ref();
    }

    void initCtr(Allocator* allocator, const ID& root_id, const char* mname = NULL)
    {
        MEMORIA_ASSERT_EXPR(root_id.isNotEmpty(), "Container root ID must not be empty");

        allocator_          = allocator;
        model_type_name_    = mname != NULL ? mname : TypeNameFactory<ContainerTypeName>::cname();
        name_               = me()->getModelName(root_id);

        //FIXME: init logger correctly

        Base::initCtr(root_id);

        ref();
    }


    bool& debug() {
        return debug_;
    }

    const bool& debug() const {
        return debug_;
    }

    MEMORIA_PUBLIC Allocator& allocator() {
        return *allocator_;
    }

    MEMORIA_PUBLIC Allocator& allocator() const {
        return *allocator_;
    }

    MEMORIA_PUBLIC const char* typeName() const {
        return model_type_name_;
    }

    MEMORIA_PUBLIC bool is_log(Int level)
    {
        return logger_.isLogEnabled(level);
    }

    MEMORIA_PUBLIC memoria::vapi::Logger& logger() {
        return logger_;
    }

    static memoria::vapi::Logger& class_logger() {
        return class_logger_;
    }

    MEMORIA_PUBLIC BigInt name() const {
        return name_;
    }

    MEMORIA_PUBLIC BigInt& name() {
        return name_;
    }

    MyType& operator=(const MyType& other)
    {
        if (this != &other)
        {
            name_               = other.name_;
            model_type_name_    = other.model_type_name_;
            logger_             = other.logger_;
            debug_              = other.debug_;

            unref();

            Base::operator=(other);

            ref();
        }

        return *this;
    }

    MyType& operator=(MyType&& other)
    {
        if (this != &other)
        {
            name_               = other.name_;
            model_type_name_    = other.model_type_name_;
            logger_             = other.logger_;
            debug_              = other.debug_;

            unref();

            Base::operator=(std::move(other));
        }

        return *this;
    }

    void inc () {
        CtrRefCounters++;
    }

    void dec() {
        CtrUnrefCounters--;
    }


private:
    MyType* me()
    {
        return this;
    }

    const MyType* me() const
    {
        return this;
    }

    void ref()
    {
        if (me()->shared() != NULL)
        {
            inc();
            me()->shared()->ref();
        }
    }

    void unref()
    {
        CtrShared* shared = me()->shared();
        if (shared != NULL)
        {
            dec();
            if (shared->unref() == 0)
            {
                allocator_->unregisterCtrShared(shared);
                me()->removeCtrShared(shared);

                Base::setCtrShared(NULL);
            }
        }
    }
};

template<
        typename Types
>
Logger Ctr<Types>::class_logger_(typeid(typename Types::ContainerTypeName).name(), Logger::DERIVED, &memoria::vapi::logger);


}




#endif
