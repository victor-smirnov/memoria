
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_MODEL_HPP
#define _MEMORIA_CORE_CONTAINER_MODEL_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/types/selector.hpp>
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


class CtrInitData {
	Int master_ctr_type_hash_;
	Int owner_ctr_type_hash_;

public:
	CtrInitData(Int master_hash, Int owner_hash):
		master_ctr_type_hash_(master_hash),
		owner_ctr_type_hash_(owner_hash)
	{}

	CtrInitData(Int master_hash):
		master_ctr_type_hash_(master_hash),
		owner_ctr_type_hash_(0)
	{}

	Int owner_ctr_type_hash() const {
		return owner_ctr_type_hash_;
	}

	Int master_ctr_type_hash() const {
		return master_ctr_type_hash_;
	}

	CtrInitData owner(int owner_hash) const
	{
		return CtrInitData(master_ctr_type_hash_, owner_hash);
	}
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

    CtrInitData init_data_;

public:
    ContainerBase(const CtrInitData& data): shared_(nullptr), init_data_(data)
    {}

    ContainerBase(const ThisType& other):
        shared_(other.shared_),
    	init_data_(other.init_data_)
    {}

    ContainerBase(const ThisType& other, Allocator* allocator):
        shared_(other.shared_),
    	init_data_(other.init_data_)
    {}

    //shared_ is configured in move constructors of subclasses.
    ContainerBase(ThisType&& other):
        shared_(other.shared_),
    	init_data_(other.init_data_)
    {
        other.shared_ = NULL;
    }

    ContainerBase(ThisType&& other, Allocator* allocator):
        shared_(other.shared_),
    	init_data_(other.init_data_)
    {
        other.shared_ = NULL;
    }

    virtual ~ContainerBase() throw () {}

    void operator=(ThisType&& other)
    {
        shared_ 	= other.shared_;
        init_data_	= other.init_data_;

        other.shared_ = NULL;
    }

    void operator=(const ThisType& other)
    {
        shared_ 	= other.shared_;
        init_data_	= other.init_data_;
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

            PageG page = alloc->getPage(*root_id, Allocator::READ);

            MyType ctr(alloc, *root_id, CtrInitData(page->master_ctr_type_hash(), page->owner_ctr_type_hash()));
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

            reflection_ = new ContainerMetadata(TypeNameFactory<Name>::name(),
                                                list,
                                                CONTAINER_HASH,
                                                MyType::getContainerInterface());

            MetadataRepository<typename Types::Profile>::registerMetadata(reflection_);
        }

        return reflection_->ctr_hash();
    }

    const CtrInitData& init_data() const {
    	return init_data_;
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
            PageG node = me()->allocator().getRoot(name, Allocator::READ);

            if (node.isSet())
            {
            	CtrShared* shared = me()->createCtrShared(name);
            	me()->allocator().registerCtrShared(shared);

            	if (node->ctr_type_hash() == CONTAINER_HASH)
            	{
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
            	else {
            		throw CtrTypeException(MEMORIA_SOURCE, SBuf()<<"Invalid container type: "<<node->ctr_type_hash());
            	}
            }
            else {
            	throw NoCtrException(MEMORIA_SOURCE, SBuf()<<"Container with name "<<name<<" does not exists");
            }
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

    void initCtr(Int command) {}
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
    CtrHelper(const CtrInitData& data): Base(data) {}
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

    CtrHelper(const CtrInitData& data): Base(data) {}
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
    CtrStart(const CtrInitData& data): Base(data) {}
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

    Int 		owner_ctr_type_hash_ = 0;
    Int 		master_ctr_type_hash_ = 0;

public:

    MEMORIA_PUBLIC Ctr(
    		Allocator* allocator,
    		Int command = CTR_CREATE,
    		BigInt name = CTR_DEFAULT_NAME,
    		const char* mname = NULL
    ):
        Base(CtrInitData(Base::CONTAINER_HASH)),
        allocator_(allocator),
        model_type_name_(mname != NULL ? mname : TypeNameFactory<ContainerTypeName>::cname()),
        debug_(false)
    {
        MEMORIA_ASSERT_NOT_NULL(allocator);

        checkCommandArguments(command, name);

        initLogger();

        if (name == CTR_DEFAULT_NAME)
        {
        	initCtr(allocator, allocator->createCtrName(), command, model_type_name_);
        }
        else {
        	initCtr(allocator, name, command, model_type_name_);
        }
    }

    void checkCommandArguments(Int command, BigInt name)
    {
    	if ((command & CTR_CREATE) == 0 && (command & CTR_FIND) == 0)
    	{
    		throw memoria::vapi::Exception(MEMORIA_SOURCE, "Either CTR_CREATE, CTR_FIND or both must be specified");
    	}

    	if ((command & CTR_FIND) && name == CTR_DEFAULT_NAME)
    	{
    		throw memoria::vapi::Exception(MEMORIA_SOURCE, "Container name must be specified for the CTR_FIND operation");
    	}
    }


    MEMORIA_PUBLIC Ctr(Allocator* allocator, const ID& root_id, const CtrInitData& ctr_init_data, const char* mname = NULL):
        Base(ctr_init_data),
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
    {
    	//FIXME: ref() ???
    }

    Ctr(MyType&& other, Allocator* allocator):
        Base(std::move(other), allocator),
        allocator_(allocator),
        model_type_name_(other.model_type_name_),
        logger_(other.logger_),
        debug_(other.debug_)
    {
        MEMORIA_ASSERT_NOT_NULL(allocator);
        //FIXME: ref() ???
    }

//    MEMORIA_PUBLIC Ctr(const NoParamCtr&):
//        Base(CtrInitData()),
//        allocator_(NULL),
//        model_type_name_(TypeNameFactory<ContainerTypeName>::cname()),
//        logger_(model_type_name_, Logger::DERIVED, NULL),
//        debug_(false)
//    {
//        Base::setCtrShared(NULL);
//        //FIXME: ref() ???
//    }

    MEMORIA_PUBLIC Ctr(const CtrInitData& data):
        Base(data),
        allocator_(NULL),
        model_type_name_(TypeNameFactory<ContainerTypeName>::cname()),
        logger_(model_type_name_, Logger::DERIVED, NULL),
        debug_(false)
    {
    	Base::setCtrShared(NULL);
    	//FIXME: ref() ???
    }

    MEMORIA_PUBLIC virtual ~Ctr() throw()
    {
        unref();
    }

    void initLogger(Logger* other)
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

    void initCtr(Allocator* allocator, BigInt name, Int command, const char* mname = NULL)
    {
        MEMORIA_ASSERT(name, >=, 0);

        allocator_          = allocator;
        name_               = name;
        model_type_name_    = mname != NULL ? mname : TypeNameFactory<ContainerTypeName>::cname();
        //FIXME: init logger correctly

        Base::initCtr(command);

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

    Int owner_ctr_type_hash () const {
    	return owner_ctr_type_hash_;
    }

    Int master_ctr_type_hash() const {
    	return master_ctr_type_hash_;
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

    MEMORIA_PUBLIC const char* typeName() const
    {
        return model_type_name_;
    }

    MEMORIA_PUBLIC bool is_log(Int level) const
    {
        return logger_.isLogEnabled(level);
    }

    MEMORIA_PUBLIC const memoria::vapi::Logger& logger() const {
        return logger_;
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
