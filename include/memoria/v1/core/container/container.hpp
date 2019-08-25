
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

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/tools/reflection.hpp>
#include <memoria/v1/core/tools/assert.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/memory.hpp>

#include <memoria/v1/profiles/common/container_operations.hpp>

#include <memoria/v1/core/container/logs.hpp>
#include <memoria/v1/core/container/names.hpp>
#include <memoria/v1/core/container/builder.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/dispatcher.hpp>
#include <memoria/v1/core/container/macros.hpp>
#include <memoria/v1/core/container/ctr_referenceable.hpp>
#include <memoria/v1/core/container/block_vertex.hpp>

#include <memoria/v1/profiles/common/common.hpp>

#include <memoria/v1/core/strings/string.hpp>

#include <memoria/v1/core/tools/pair.hpp>
#include <memoria/v1/core/tools/memory.hpp>

#include <memoria/v1/core/graph/graph.hpp>
#include <memoria/v1/core/graph/graph_default_edge.hpp>
#include <memoria/v1/core/graph/graph_default_vertex_property.hpp>

#include <memoria/v1/profiles/common/metadata.hpp>

#include <memoria/v1/api/datatypes/traits.hpp>
#include <memoria/v1/api/datatypes/type_registry.hpp>

#include <memoria/v1/api/common/ctr_api.hpp>

#include <string>
#include <memory>

#define MEMORIA_MODEL_METHOD_IS_NOT_IMPLEMENTED() \
        throw Exception(MMA1_SOURCE, SBuf() << "Method is not implemented for " << me()->typeName())

namespace memoria {
namespace v1 {

template <typename Profile, typename SelectorType, typename ContainerTypeName = SelectorType> class CtrTF;

template <typename Name, typename Base, typename Types> class CtrPart;

template <typename Types> class Ctr;
template <typename Types> class Iter;
template <typename CtrName, typename Profile> class SharedIter;
template <typename CtrName, typename Allocator, typename Profile> class SharedCtr;


constexpr UUID CTR_DEFAULT_NAME = UUID(-1ull, -1ull);

class CtrInitData {
    UUID master_name_;
    uint64_t ctr_type_hash_;

public:
    CtrInitData(const UUID& master_name, uint64_t ctr_hash):
        master_name_(master_name),
        ctr_type_hash_(ctr_hash)
    {}

    CtrInitData(uint64_t ctr_hash):
        master_name_{},
        ctr_type_hash_(ctr_hash)
    {}

    const auto& master_name() const {
        return master_name_;
    }

    void set_master_name(UUID name){
        master_name_ = name;
    }

    uint64_t ctr_type_hash() const {
        return ctr_type_hash_;
    }

    CtrInitData owner(uint64_t ctr_hash) const
    {
        return CtrInitData(master_name_, ctr_hash);
    }
};





template <typename TypesType>
class CtrBase:
        public IVertex,
        public ::memoria::v1::ICtrApi<typename TypesType::ContainerTypeName, typename TypesType::Profile>,
        public CtrSharedFromThis<Ctr<TypesType>>
{
public:

    using ThisType  = CtrBase<TypesType>;
    using MyType    = Ctr<TypesType>;

    using ContainerTypeName = typename TypesType::ContainerTypeName;
    using Name              = ContainerTypeName;
    using Types             = TypesType;
    using ProfileT          = typename Types::Profile;
    
    using Allocator = typename Types::Allocator;
    using BlockID   = typename Allocator::BlockID;
    using BlockType = typename Allocator::BlockType;
    using CtrID     = typename Allocator::CtrID;
    using BlockG    = typename Allocator::BlockG;

    using Iterator          = Iter<typename Types::IterTypes>;
    using SharedIterator    = SharedIter<ContainerTypeName, typename TypesType::Profile>;
    using IteratorPtr       = CtrSharedPtr<SharedIterator>;
    using AllocatorPtr      = CtrSharedPtr<Allocator>;

    using NodeDispatcher = typename Types::Blocks::template NodeDispatcher<MyType>;

    static constexpr uint64_t CONTAINER_HASH = TypeHashV<Name>;

    template <typename> friend class BTIteratorBase;

    template <typename> friend class Iter;
    template <typename, typename, typename> friend class IterPart;
    template <typename> friend class IterStart;

    template <typename> friend class CtrStart;
    //template <typename, typename, typename> friend class CtrPart;
    template <typename> friend class Ctr;

protected:
    BlockID root_{};

    CtrInitData init_data_;

    PairPtr pair_;
    
    AllocatorPtr allocator_holder_;

public:
    CtrBase(const CtrInitData& data, const AllocatorPtr& allocator):
        init_data_(data),
        allocator_holder_(allocator)
    {}

    virtual ~CtrBase() throw () {}
    
    bool isNew() const {
        return root_.is_null();
    }
    
    void reset_allocator_holder() {
        allocator_holder_.reset();
    }
    
    virtual bool is_castable_to(uint64_t type_code) const {
        return CONTAINER_HASH == type_code;
    }
    
    virtual uint64_t type_hash() {
        return CONTAINER_HASH;
    }
    
    virtual U16String describe_type() const {
        return TypeNameFactory<ContainerTypeName>::name();
    }

    PairPtr& pair() {return pair_;}
    const PairPtr& pair() const {return pair_;}

    void set_root(const BlockID &root)
    {
        root_ = root;
        self().store().setRoot(self().master_name(), root);
    }

    void set_root_id(const BlockID &root)
    {
        root_ = root;
    }

    const BlockID &root() const
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

    static uint64_t hash() {
        return CONTAINER_HASH;
    }

    struct NodesInit {
        NodesInit(ContainerOperationsPtr<ProfileT> ctr_ops, ContainerInstanceFactoryPtr<ProfileT> ctr_factory)
        {
            ProfileMetadataStore<ProfileT>::global().add_container_factories(
                make_datatype_signature<ContainerTypeName>().name(),
                std::move(ctr_factory)
            );

            ProfileMetadataStore<ProfileT>::global().add_container_operations(
                static_cast<uint64_t>(CONTAINER_HASH),
                std::move(ctr_ops)
            );

            std::vector<BlockOperationsPtr<ProfileT>> list;
            NodeDispatcher::build_metadata_list(list);

            for (auto& ptr: list)
            {
                ProfileMetadataStore<ProfileT>::global().add_block_operations(
                        static_cast<uint64_t>(CONTAINER_HASH),
                        std::move(ptr)
                );
            }
        }
    };

    static void init_profile_metadata()
    {
        static NodesInit nodes_init(getContainerOperations(), getContainerFactories());
    }

    struct CtrInstanceFactoryImpl: public CtrInstanceFactory<ProfileT> {

        template <typename CtrName>
        using CtrT = SharedCtr<CtrName, ProfileAllocatorType<ProfileT>, ProfileT>;

        template <typename CtrName>
        using CtrPtr = CtrSharedPtr<CtrT<CtrName>>;

        virtual SnpSharedPtr<CtrReferenceable<ProfileT>> create_instance(
                const AllocatorPtr& allocator,
                int32_t command,
                const CtrID& ctr_id
        ) const
        {
            return ctr_make_shared<CtrT<ContainerTypeName>>(allocator, command, ctr_id);
        }
    };

    struct CtrInterfaceImpl: public ContainerOperations<ProfileT> {

        using CIBase = ContainerOperations<ProfileT>;

        using typename CIBase::BlockCallbackFn;

        virtual U8String data_type_decl_signature() const {
            return make_datatype_signature<ContainerTypeName>().name();
        }

        virtual Vertex describe_block(const BlockID& block_id, const CtrID& ctr_id, AllocatorPtr allocator) const
        {
            auto ctr_ptr = memoria_static_pointer_cast<MyType>(allocator->get(ctr_id));
            return ctr_ptr->block_as_vertex(block_id);
        }

        virtual Collection<Edge> describe_block_links(const BlockID& block_id, const CtrID& ctr_id, AllocatorPtr allocator, Direction direction) const
        {
            auto ctr_ptr = memoria_static_pointer_cast<MyType>(allocator->get(ctr_id));
            return ctr_ptr->describe_block_links(block_id, direction);
        }

        virtual Collection<VertexProperty> block_properties(const Vertex& vx, const BlockID& block_id, const CtrID& ctr_id, AllocatorPtr allocator) const
        {
            auto ctr_ptr = memoria_static_pointer_cast<MyType>(allocator->get(ctr_id));
            return ctr_ptr->block_properties(vx, block_id);
        }

        virtual U16String ctr_name() const
        {
            return TypeNameFactory<Name>::name();
        }

        void with_ctr(const CtrID& ctr_id, const AllocatorPtr& allocator, std::function<void(MyType&)> fn) const
        {
            auto ctr_ref = allocator->get(ctr_id);
            if (ctr_ref)
            {
                auto ctr_ptr = memoria_static_pointer_cast<MyType>(ctr_ref);
                fn(*ctr_ptr.get());
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"No container is found for id {}", ctr_id));
            }
        }

        virtual bool check(const CtrID& ctr_id, AllocatorPtr allocator) const
        {
            bool result = false;

            with_ctr(ctr_id, allocator, [&](MyType& ctr){
                result = ctr.check(nullptr);
            });

            return result;
        }

        virtual void walk(
                const UUID& ctr_id,
                AllocatorPtr allocator,
                ContainerWalker<ProfileT>* walker
        ) const
        {
            with_ctr(ctr_id, allocator, [&](MyType& ctr){
                ctr.walkTree(walker);
            });
        }

        virtual void drop(const CtrID& ctr_id, AllocatorPtr allocator) const
        {
            with_ctr(ctr_id, allocator, [&](MyType& ctr){
                ctr.drop();
            });
        }

        virtual U16String ctr_type_name() const
        {
            return TypeNameFactory<ContainerTypeName>::name();
        }


        class CtrNodesWalkerAdapter: public ContainerWalkerBase<ProfileT> {

        	BlockCallbackFn consumer_;
        public:
        	CtrNodesWalkerAdapter(BlockCallbackFn consumer): consumer_(consumer)
        	{}

            virtual void beginRoot(int32_t idx, const BlockType* block) {
                beginNode(idx, block);
            }


            virtual void beginNode(int32_t idx, const BlockType* block) {
                consumer_(block->uuid(), block->id(), block);
            }

            virtual void rootLeaf(int32_t idx, const BlockType* block) {
                beginNode(idx, block);
            }

            virtual void leaf(int32_t idx, const BlockType* block) {
                beginNode(idx, block);
            }
        };



        virtual void for_each_ctr_node(
            const CtrID& ctr_id,
            AllocatorPtr allocator,
            BlockCallbackFn consumer
        ) const
        {
            CtrNodesWalkerAdapter walker(consumer);

            with_ctr(ctr_id, allocator, [&](MyType& ctr){
        		ctr.walkTree(&walker);
        	});
        }
        
        virtual CtrSharedPtr<CtrReferenceable<ProfileT>> new_ctr_instance(const BlockID& root_id, const CtrID& ctr_id, AllocatorPtr allocator) const
        {    
            BlockG block = allocator->getBlock(root_id);

            if (block)
            {
            	return ctr_make_shared<SharedCtr<ContainerTypeName, Allocator, typename Types::Profile>> (
                    allocator,
                    root_id, 
                    CtrInitData(ctr_id, block->ctr_type_hash())
                );
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"No container root block is found for id {} and name {}", root_id, ctr_id));
            }
        }

        virtual CtrID clone_ctr(const BlockID& ctr_id, const CtrID& new_ctr_id, AllocatorPtr allocator) const
        {
            CtrID new_name_rtn{};

            with_ctr(ctr_id, allocator, [&](MyType& ctr){
                new_name_rtn = ctr.clone(new_ctr_id);
            });

            return new_name_rtn;
        }

        virtual CtrBlockDescription<ProfileT> describe_block1(const BlockID& block_id, AllocatorPtr allocator) const
        {
            return MyType::describe_block(block_id, allocator.get());
        }
    };


    static ContainerOperationsPtr<ProfileT> getContainerOperations()
    {
        static auto container_operations_ptr = metadata_make_shared<CtrInterfaceImpl>();
        return container_operations_ptr;
    }

    static ContainerInstanceFactoryPtr<ProfileT> getContainerFactories()
    {
        static auto container_factories_ptr = metadata_make_shared<CtrInstanceFactoryImpl>();
        return container_factories_ptr;
    }


    const CtrInitData& init_data() const {
        return init_data_;
    }

    CtrInitData& init_data() {
        return init_data_;
    }


    CtrID getModelName(const BlockID root_id)
    {
        return CtrID{};
    }


    void initCtr(int32_t command) {}
    void initCtr(const BlockID& root_id) {}

    Vertex as_vertex() const {
        return Vertex(StaticPointerCast<IVertex>(ConstPointerCast<MyType>(this->shared_from_this())));
    }

    virtual Graph graph()
    {
        return allocator_holder_->allocator_vertex().graph();
    }

    virtual Any id() const
    {
        return Any(this->name());
    }

    virtual U16String label() const
    {
        return U16String(u"container");
    }

    virtual void remove()
    {
        MMA1_THROW(GraphException()) << WhatCInfo("Can't remove container with Vertex::remove()");
    }

    virtual bool is_removed() const
    {
        return false;
    }

    virtual Collection<VertexProperty> properties()
    {
        return make_fn_vertex_properties(
            as_vertex(),
            u"type", [&]{return U16String(TypeNameFactory<Name>::name());}
        );
    }

    Vertex block_as_vertex(const BlockID& block_id)
    {
        Graph my_graph = this->graph();
        Vertex block_vx = BlockVertex<AllocatorPtr, ContainerOperationsPtr<ProfileT>>::make(
                    my_graph,
                    allocator_holder_,
                    getContainerOperations(),
                    block_id,
                    this->name()
        );

        return block_vx;
    }

    Collection<Edge> describe_block_links(const BlockID& block_id, const CtrID& name, Direction direction) const
    {
        return EmptyCollection<Edge>::make();
    }

    virtual Collection<Edge> edges(Direction direction)
    {
        std::vector<Edge> edges;

        Graph my_graph  = this->graph();
        Vertex alloc_vx = allocator_holder_->allocator_vertex();

        Vertex my_vx = as_vertex();

        if (is_out(direction))
        {
            Vertex root_vx = block_as_vertex(root());
            edges.emplace_back(DefaultEdge::make(my_graph, u"root", my_vx, root_vx));
        }

        if (is_in(direction))
        {
            edges.emplace_back(DefaultEdge::make(my_graph, u"container", alloc_vx, my_vx));
        }

        return STLCollection<Edge>::make(std::move(edges));
    }

    CtrID clone(const CtrID& new_name)
    {
        MMA1_THROW(Exception()) << WhatCInfo("Clone operation is not supported for this container");
    }

    bool is_updatable() const {
        return self().store().isActive();
    }

protected:

    template <typename... Args>
    IteratorPtr make_iterator(Args&&... args) const {
        return ctr_make_shared<SharedIterator>(this->shared_from_this(), std::forward<Args>(args)...);
    }

    template <typename... Args>
    IteratorPtr make_iterator(Args&&... args) {
        return ctr_make_shared<SharedIterator>(this->shared_from_this(), std::forward<Args>(args)...);
    }

    template <typename... Args>
    IteratorPtr clone_iterator(Args&&... args) const {
        return ctr_make_shared<SharedIterator>(std::forward<Args>(args)...);
    }

    template <typename... Args>
    IteratorPtr clone_iterator(Args&&... args) {
        return ctr_make_shared<SharedIterator>(std::forward<Args>(args)...);
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




template <int Idx, typename Types>
class CtrHelper: public CtrPart<
                            Select<Idx, typename Types::List>,
                            CtrHelper<Idx - 1, Types>,
                            Types>
{
    using ThisType = CtrHelper<Idx, Types>;
    using MyType   = Ctr<Types>;
    using Base     = CtrPart<Select<Idx, typename Types::List>, CtrHelper<Idx - 1, Types>, Types>;

    using Allocator0 = typename Types::Allocator;

public:
    CtrHelper(const CtrInitData& data, const CtrSharedPtr<Allocator0>& allocator): Base(data, allocator) {}

    virtual ~CtrHelper() throw () {}
};

template <typename Types>
class CtrHelper<-1, Types>: public Types::template BaseFactory<Types> {
    using ThisType = CtrHelper<-1, Types>;
    using MyType   = Ctr<Types>;
    using Base     = typename Types::template BaseFactory<Types>;

public:

    using Allocator0 = typename Types::Allocator;

    CtrHelper(const CtrInitData& data, const CtrSharedPtr<Allocator0>& allocator): Base(data, allocator) {}

    virtual ~CtrHelper() throw () {}

    void operator=(ThisType&& other) {
        Base::operator=(std::move(other));
    }

    void operator=(const ThisType& other) {
        Base::operator=(other);
    }
};


template <typename Types>
class CtrStart: public CtrHelper<ListSize<typename Types::List> - 1, Types> {

    using ThisType  = CtrStart<Types>;
    using MyType    = Ctr<Types>;
    using Base      = CtrHelper<ListSize<typename Types::List> - 1, Types>;

    using Allocator0 = typename Types::Allocator ;

public:
    CtrStart(const CtrInitData& data, const CtrSharedPtr<Allocator0>& allocator): Base(data, allocator) {}
};


extern int32_t CtrRefCounters;
extern int32_t CtrUnrefCounters;


template <typename Types>
class Ctr: public CtrStart<Types> {
    using Base = CtrStart<Types>;

public:
    using MyType = Ctr<Types>;

    using Allocator = typename Types::Allocator;
    using BlockG     = typename Types::Allocator::BlockG;
    using BlockID   = typename Allocator::BlockID;
    using CtrID     = typename Allocator::CtrID;

public:

    using ContainerTypeName = typename Types::ContainerTypeName;
    using Name = ContainerTypeName;

private:

    Allocator*  allocator_;
    CtrID       name_;
    const char* model_type_name_;

    Logger          logger_;
    static Logger   class_logger_;

    bool debug_;

protected:
    CtrSharedPtr<Allocator> alloc_holder_;

public:

    Ctr(
        const CtrSharedPtr<Allocator>& allocator,
        int32_t command = CTR_CREATE,
        const CtrID& name = CTR_DEFAULT_NAME,
        const char* mname = NULL
    ):
        Base(CtrInitData(Base::CONTAINER_HASH), allocator),
        allocator_(allocator.get()),
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

    void checkCommandArguments(int32_t command, const CtrID& name)
    {
        if ((command & CTR_CREATE) == 0 && (command & CTR_FIND) == 0)
        {
            MMA1_THROW(Exception()) << WhatCInfo("Either CTR_CREATE, CTR_FIND or both must be specified");
        }

        if ((command & CTR_FIND) && name == CTR_DEFAULT_NAME)
        {
            MMA1_THROW(Exception()) << WhatCInfo("Container name must be specified for the CTR_FIND operation");
        }
    }


    Ctr(const CtrSharedPtr<Allocator>& allocator, const BlockID& root_id, const CtrInitData& ctr_init_data, const char* mname = NULL):
        Base(ctr_init_data, allocator),
        allocator_(allocator.get()),
        name_(),
        model_type_name_(mname != NULL ? mname : TypeNameFactory<ContainerTypeName>::cname()),
        debug_(false)
    {
        MEMORIA_V1_ASSERT_NOT_NULL(allocator);

        initLogger();

        initCtr(allocator, root_id, mname);
    }

    Ctr(const CtrInitData& data):
        Base(data, CtrSharedPtr<Allocator>{}),
        allocator_{},
        model_type_name_(TypeNameFactory<ContainerTypeName>::cname()),
        logger_(model_type_name_, Logger::DERIVED, NULL),
        debug_(false)
    {
    }

    Ctr(const MyType& other) = delete;
    Ctr(MyType&& other) = delete;

    virtual ~Ctr() noexcept
    {
        try {
    		allocator_->unregisterCtr(typeid(*this));
    	}
    	catch (Exception& ex) {
            ex.dump(std::cout);
    		std::abort();
    	}
    	catch(...) {
            std::cout << "Unknown exception in ~Ctr(): " << typeid(*this).name() << std::endl;
    		std::abort();
    	}
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

    void initCtr(const CtrSharedPtr<Allocator>& allocator, const CtrID& name, int32_t command, const char* mname = NULL)
    {
        Base::allocator_holder_ = allocator;
        
        allocator_          = allocator.get();
        
        name_               = name;
        model_type_name_    = mname != NULL ? mname : TypeNameFactory<ContainerTypeName>::cname();

        allocator_->registerCtr(typeid(*this));

        this->init_data().set_master_name(name);

        //FIXME: init logger correctly

        Base::initCtr(command);
    }

    void initCtr(const CtrSharedPtr<Allocator>& allocator, const BlockID& root_id, const char* mname = NULL)
    {
        MEMORIA_V1_ASSERT_EXPR(!root_id.is_null(), "Container root ID must not be empty");

        Base::allocator_holder_ = allocator;
        
        allocator_          = allocator.get();
        model_type_name_    = mname != NULL ? mname : TypeNameFactory<ContainerTypeName>::cname();
        name_               = this->getModelName(root_id);

        allocator_->registerCtr(typeid(*this));

        //FIXME: init logger correctly

        Base::initCtr(root_id, name_);
    }


    bool& debug() {
        return debug_;
    }

    const bool& debug() const {
        return debug_;
    }

    Allocator& store() {
        return *allocator_;
    }

    Allocator& store() const {
        return *allocator_;
    }

    const char* typeName() const
    {
        return model_type_name_;
    }

    static U16String type_name_str()
    {
        return TypeNameFactory<ContainerTypeName>::name();
    }

    static const char* type_name_cstr()
    {
        return TypeNameFactory<ContainerTypeName>::cname();
    }


    bool is_log(int32_t level) const
    {
        return logger_.isLogEnabled(level);
    }

    const Logger& logger() const {
        return logger_;
    }

    Logger& logger() {
        return logger_;
    }

    static Logger& class_logger() {
        return class_logger_;
    }

    virtual const CtrID& name() const {
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
Logger Ctr<Types>::class_logger_(typeid(typename Types::ContainerTypeName).name(), Logger::DERIVED, &logger);


}}
