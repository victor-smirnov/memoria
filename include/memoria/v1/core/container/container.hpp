
// Copyright 2011 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/tools/reflection.hpp>
#include <memoria/v1/core/tools/assert.hpp>
#include <memoria/v1/core/tools/uuid.hpp>

#include <memoria/v1/metadata/container.hpp>

#include <memoria/v1/core/container/logs.hpp>
#include <memoria/v1/core/container/names.hpp>
#include <memoria/v1/core/container/builder.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/dispatcher.hpp>
#include <memoria/v1/core/container/defaults.hpp>
#include <memoria/v1/core/container/macros.hpp>
#include <memoria/v1/core/container/init.hpp>

#include <string>
#include <memory>
#include "../tools/pair.hpp"



#define MEMORIA_MODEL_METHOD_IS_NOT_IMPLEMENTED() \
        throw Exception(MEMORIA_SOURCE, SBuf()<<"Method is not implemented for "<<me()->typeName())

namespace memoria {
namespace v1 {

template <typename Profile, typename SelectorType, typename ContainerTypeName> class CtrTF;

template <typename Name, typename Base, typename Types> class CtrPart;

template <typename Types> class Ctr;
template <typename Types> class Iter;

template <typename Profile> class MetadataRepository;

constexpr UUID CTR_DEFAULT_NAME = UUID(-1ull, -1ull);

class CtrInitData {
    UUID master_name_;
    Int master_ctr_type_hash_;
    Int owner_ctr_type_hash_;

public:
    CtrInitData(const UUID& master_name, Int master_hash, Int owner_hash):
        master_name_(master_name),
        master_ctr_type_hash_(master_hash),
        owner_ctr_type_hash_(owner_hash)
    {}

    CtrInitData(Int master_hash):
        master_name_(),
        master_ctr_type_hash_(master_hash),
        owner_ctr_type_hash_()
    {}

    const auto& master_name() const {
        return master_name_;
    }

    void set_master_name(UUID name){
        master_name_ = name;
    }

    Int owner_ctr_type_hash() const {
        return owner_ctr_type_hash_;
    }

    Int master_ctr_type_hash() const {
        return master_ctr_type_hash_;
    }

    CtrInitData owner(int owner_hash) const
    {
        return CtrInitData(master_name_, master_ctr_type_hash_, owner_hash);
    }
};


template <typename TypesType>
class CtrBase: public TypesType::Allocator, public std::enable_shared_from_this<Ctr<TypesType>> {
public:

    using ThisType  = CtrBase<TypesType>;
    using MyType    = Ctr<TypesType>;

    using ContainerTypeName = typename TypesType::ContainerTypeName;
    using Name              = ContainerTypeName;
    using Types             = TypesType;

    using Allocator = typename Types::Allocator;
    using ID        = typename Allocator::ID;
    using Page      = typename Allocator::Page;
    using PageG     = typename Allocator::PageG;

    using Iterator      = Iter<typename Types::IterTypes>;
    using IteratorPtr   = std::shared_ptr<Iterator>;
    
    static constexpr Int CONTAINER_HASH = TypeHash<Name>::Value;

    template <typename> friend class BTIteratorBase;

    template <typename> friend class Iter;
    template <typename, typename, typename> friend class IterPart;
    template <typename> friend class IterStart;

    template <typename> friend class CtrStart;
    template <typename, typename, typename> friend class CtrPart;
    template <typename> friend class Ctr;


protected:
    static ContainerMetadataPtr reflection_;

    ID root_;

    CtrInitData init_data_;

    PairPtr pair_;

public:
    CtrBase(const CtrInitData& data): init_data_(data)
    {}

    virtual ~CtrBase() throw () {}

    PairPtr& pair() {return pair_;}
    const PairPtr& pair() const {return pair_;}

    void set_root(const ID &root)
    {
        root_ = root;

        self().allocator().setRoot(self().master_name(), root);
    }

    void set_root_id(const ID &root)
    {
        root_ = root;
    }

    const ID &root() const
    {
        return root_;
    }

    void operator=(ThisType&& other)
    {
        init_data_  = other.init_data_;

        other.shared_ = NULL;
    }

    void operator=(const ThisType& other)
    {
        init_data_  = other.init_data_;
    }

    static Int hash() {
        return CONTAINER_HASH;
    }

    static const ContainerMetadataPtr& getMetadata()
    {
        return reflection_;
    }
    
    static void destroyMetadata()
    {
        if (reflection_)
        {
            MetadataRepository<typename Types::Profile>::unregisterMetadata(reflection_);
            reflection_.reset();
        }
    }


    struct CtrInterfaceImpl: public ContainerInterface {

    	virtual String ctr_name()
    	{
    		return TypeNameFactory<Name>::name();
    	}

        void with_ctr(const UUID& root_id, const UUID& name, void* allocator, std::function<void(MyType&)> fn) const
        {
            Allocator* alloc = T2T<Allocator*>(allocator);

            PageG page = alloc->getPage(root_id, name);

            auto ctr_name = MyType::getModelNameS(page);

            auto ctr_ptr = std::make_shared<MyType>(alloc, root_id, CtrInitData(ctr_name, page->master_ctr_type_hash(), page->owner_ctr_type_hash()));

            fn(*ctr_ptr.get());
        }

        virtual bool check(const UUID& root_id, const UUID& name, void* allocator) const
        {
            bool result = false;

            with_ctr(root_id, name, allocator, [&](MyType& ctr){
                result = ctr.check(nullptr);
            });

            return result;
        }

        virtual void walk(
                const UUID& root_id,
                const UUID& name,
                void* allocator,
                ContainerWalker* walker
        ) const
        {
            with_ctr(root_id, name, allocator, [&](MyType& ctr){
                ctr.walkTree(walker);
            });
        }

        virtual void drop(const UUID& root_id, const UUID& name, void* allocator)
        {
            with_ctr(root_id, name, allocator, [&](MyType& ctr){
                ctr.drop();
            });
        }

        virtual String ctr_type_name() const
        {
            return TypeNameFactory<ContainerTypeName>::name();
        }

        virtual ~CtrInterfaceImpl() {}
    };


    static ContainerInterfacePtr getContainerInterface()
    {
        return std::make_shared<CtrInterfaceImpl>();
    }

    static Int initMetadata(Int salt = 0)
    {
    	if (!reflection_)
        {
            MetadataList list;

            Types::Pages::NodeDispatcher::buildMetadataList(list);

            reflection_ = std::make_shared<ContainerMetadata>(TypeNameFactory<Name>::name(),
                                                list,
												static_cast<int>(CONTAINER_HASH),
                                                MyType::getContainerInterface());

            MetadataRepository<typename Types::Profile>::registerMetadata(reflection_);
        }

        return reflection_->ctr_hash();
    }

    const CtrInitData& init_data() const {
        return init_data_;
    }

    CtrInitData& init_data() {
        return init_data_;
    }


    UUID getModelName(ID root_id)
    {
        return UUID();
    }


    void initCtr(Int command) {}
    void initCtr(const ID& root_id) {}

protected:

    template <typename... Args>
    IteratorPtr make_iterator(Args&&... args) const {
        return make_shared<Iterator>(this->shared_from_this(), std::forward<Args>(args)...);
    }

    template <typename... Args>
    IteratorPtr make_iterator(Args&&... args) {
        return make_shared<Iterator>(this->shared_from_this(), std::forward<Args>(args)...);
    }

    template <typename... Args>
    IteratorPtr clone_iterator(Args&&... args) const {
        return make_shared<Iterator>(std::forward<Args>(args)...);
    }

    template <typename... Args>
    IteratorPtr clone_iterator(Args&&... args) {
        return make_shared<Iterator>(std::forward<Args>(args)...);
    }


    /**
     * \brief Set container reflection metadata.
     */

    static void setMetadata(ContainerMetadataPtr metadata)
    {
        reflection_ = metadata;
    }


private:


    MyType& self()
    {
        return *static_cast<MyType*>(this);
    }

    const MyType& self() const
    {
        return *static_cast<const MyType*>(this);
    }
};

template <typename TypesType>
ContainerMetadataPtr CtrBase<TypesType>::reflection_;



template <int Idx, typename Types>
class CtrHelper: public CtrPart<
                            SelectByIndex<Idx, typename Types::List>,
                            CtrHelper<Idx - 1, Types>,
                            Types>
{
    typedef CtrHelper<Idx, Types>                               ThisType;
    typedef Ctr<Types>                                          MyType;
    typedef CtrPart<SelectByIndex<Idx, typename Types::List>, CtrHelper<Idx - 1, Types>, Types> Base;

    typedef typename Types::Allocator Allocator0;

public:
    CtrHelper(const CtrInitData& data): Base(data) {}

    virtual ~CtrHelper() throw () {}
};

template <typename Types>
class CtrHelper<-1, Types>: public Types::template BaseFactory<Types>::Type {
    typedef CtrHelper<-1, Types>                                ThisType;
    typedef Ctr<Types>                                          MyType;
    typedef typename Types::template BaseFactory<Types>::Type   Base;

public:

    typedef typename Types::Allocator                           Allocator0;

    CtrHelper(const CtrInitData& data): Base(data) {}

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
};


extern Int CtrRefCounters;
extern Int CtrUnrefCounters;


template <typename Types>
class Ctr: public CtrStart<Types> {
    typedef CtrStart<Types>                                                     Base;
public:
    typedef Ctr<Types>                                                          MyType;

    typedef typename Types::Allocator                                           Allocator;
    typedef typename Types::Allocator::PageG                                    PageG;
    typedef typename PageG::Page::ID                                            ID;

public:

    typedef typename Types::ContainerTypeName                                   ContainerTypeName;
    typedef ContainerTypeName                                                   Name;

private:

    Allocator*  allocator_;
    UUID        name_;
    const char* model_type_name_;

    Logger          logger_;
    static Logger   class_logger_;

    bool        debug_;

    Int         owner_ctr_type_hash_ = 0;
    Int         master_ctr_type_hash_ = 0;

public:

    Ctr(
            Allocator* allocator,
            Int command = CTR_CREATE,
            const UUID& name = CTR_DEFAULT_NAME,
            const char* mname = NULL
    ):
        Base(CtrInitData(Base::CONTAINER_HASH)),
        allocator_(allocator),
        model_type_name_(mname != NULL ? mname : TypeNameFactory<ContainerTypeName>::cname()),
        debug_(false)
    {
        MEMORIA_V1_ASSERT_NOT_NULL(allocator);

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

    void checkCommandArguments(Int command, const UUID& name)
    {
        if ((command & CTR_CREATE) == 0 && (command & CTR_FIND) == 0)
        {
            throw v1::Exception(MEMORIA_SOURCE, "Either CTR_CREATE, CTR_FIND or both must be specified");
        }

        if ((command & CTR_FIND) && name == CTR_DEFAULT_NAME)
        {
            throw v1::Exception(MEMORIA_SOURCE, "Container name must be specified for the CTR_FIND operation");
        }
    }


    Ctr(Allocator* allocator, const ID& root_id, const CtrInitData& ctr_init_data, const char* mname = NULL):
        Base(ctr_init_data),
        allocator_(allocator),
        name_(),
        model_type_name_(mname != NULL ? mname : TypeNameFactory<ContainerTypeName>::cname()),
        debug_(false)
    {
        MEMORIA_V1_ASSERT_NOT_NULL(allocator);

        initLogger();

        initCtr(allocator, root_id, mname);
    }

    Ctr(const CtrInitData& data):
        Base(data),
        allocator_(),
        model_type_name_(TypeNameFactory<ContainerTypeName>::cname()),
        logger_(model_type_name_, Logger::DERIVED, NULL),
        debug_(false)
    {
    }

    Ctr(const MyType& other) = delete;
    Ctr(MyType&& other) = delete;

    virtual ~Ctr() noexcept
    {}

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

    void initCtr(Allocator* allocator, const UUID& name, Int command, const char* mname = NULL)
    {
        allocator_          = allocator;
        name_               = name;
        model_type_name_    = mname != NULL ? mname : TypeNameFactory<ContainerTypeName>::cname();

        this->init_data().set_master_name(name);

        //FIXME: init logger correctly

        Base::initCtr(command);
    }

    void initCtr(Allocator* allocator, const ID& root_id, const char* mname = NULL)
    {
        MEMORIA_V1_ASSERT_EXPR(!root_id.is_null(), "Container root ID must not be empty");

        allocator_          = allocator;
        model_type_name_    = mname != NULL ? mname : TypeNameFactory<ContainerTypeName>::cname();
        name_               = this->getModelName(root_id);

        //FIXME: init logger correctly

        Base::initCtr(root_id, name_);
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

    Allocator& allocator() {
        return *allocator_;
    }

    Allocator& allocator() const {
        return *allocator_;
    }

    const char* typeName() const
    {
        return model_type_name_;
    }

    static String type_name_str()
    {
        return TypeNameFactory<ContainerTypeName>::name();
    }

    static const char* type_name_cstr()
    {
        return TypeNameFactory<ContainerTypeName>::cname();
    }


    bool is_log(Int level) const
    {
        return logger_.isLogEnabled(level);
    }

    const v1::Logger& logger() const {
        return logger_;
    }

    v1::Logger& logger() {
            return logger_;
        }

    static v1::Logger& class_logger() {
        return class_logger_;
    }

    const auto& name() const {
        return name_;
    }

    const auto& master_name() const
    {
        return Base::init_data().master_name();
    }

    MyType& operator=(const MyType& other)
    {
        if (this != &other)
        {
            name_               = other.name_;
            model_type_name_    = other.model_type_name_;
            logger_             = other.logger_;
            debug_              = other.debug_;

            Base::operator=(other);
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

            Base::operator=(std::move(other));
        }

        return *this;
    }

};

template<
        typename Types
>
Logger Ctr<Types>::class_logger_(typeid(typename Types::ContainerTypeName).name(), Logger::DERIVED, &v1::logger);


}}
