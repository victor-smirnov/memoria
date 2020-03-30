
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

#include <memoria/core/types.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/tools/assert.hpp>
#include <memoria/core/tools/uuid.hpp>
#include <memoria/core/memory/memory.hpp>

#include <memoria/profiles/common/container_operations.hpp>

#include <memoria/core/container/logs.hpp>
#include <memoria/core/container/names.hpp>
#include <memoria/core/container/builder.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/dispatcher.hpp>
#include <memoria/core/container/macros.hpp>
#include <memoria/core/container/ctr_referenceable.hpp>

#include <memoria/profiles/common/common.hpp>

#include <memoria/core/strings/string.hpp>

#include <memoria/core/tools/pair.hpp>
#include <memoria/core/memory/memory.hpp>

#include <memoria/profiles/common/metadata.hpp>

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/datatypes/type_registry.hpp>

#include <memoria/api/common/ctr_api.hpp>
#include <memoria/core/tools/result.hpp>

#include <string>
#include <memory>
#include <unordered_set>

namespace memoria {

template <typename Profile, typename SelectorType, typename ContainerTypeName = SelectorType> class CtrTF;

template <typename Name, typename Base, typename Types> class CtrPart;

template <typename Types> class Ctr;
template <typename Types> class Iter;
template <typename CtrName, typename Profile> class SharedIter;
template <typename CtrName, typename Allocator, typename Profile> class SharedCtr;


constexpr UUID CTR_DEFAULT_NAME = UUID(-1ull, -1ull);



template <typename TypesType>
class CtrBase:
        public ::memoria::ICtrApi<typename TypesType::ContainerTypeName, typename TypesType::Profile>,
        public CtrSharedFromThis<Ctr<TypesType>>
{
public:

    using ThisType  = CtrBase<TypesType>;
    using MyType    = Ctr<TypesType>;

    using ContainerTypeName = typename TypesType::ContainerTypeName;
    using Name              = ContainerTypeName;
    using Types             = TypesType;
    using ProfileT          = typename Types::Profile;

    using NodeBaseG = typename Types::NodeBaseG;
    
    using Allocator = typename Types::Allocator;
    using BlockID   = typename Allocator::BlockID;
    using BlockType = typename Allocator::BlockType;
    using CtrID     = typename Allocator::CtrID;
    using BlockG    = typename Allocator::BlockG;

    using Iterator          = Iter<typename Types::IterTypes>;
    using SharedIterator    = SharedIter<ContainerTypeName, typename TypesType::Profile>;
    using IteratorPtr       = CtrSharedPtr<SharedIterator>;
    using AllocatorPtr      = SnpSharedPtr<Allocator>;

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

    PairPtr pair_;
    
    AllocatorPtr allocator_holder_;

    bool do_unregister_on_dtr_{true};
    bool do_unregister_{true};

public:
    CtrBase(MaybeError&) noexcept {}

    virtual ~CtrBase() noexcept {}

    auto make_shared_ptr() const noexcept {
        return this->shared_from_this();
    }

    auto make_shared_ptr() noexcept {
        return this->shared_from_this();
    }

    // TODO: error handling
    virtual CtrSharedPtr<CtrReferenceable<ProfileT>> shared_self() noexcept {
        return this->shared_from_this();
    }

    virtual CtrSharedPtr<const CtrReferenceable<ProfileT>> shared_self() const noexcept {
        return this->shared_from_this();
    }
    
    bool isNew() const noexcept {
        return root_.is_null();
    }
    
    void reset_allocator_holder() noexcept {
        allocator_holder_.reset();
    }

    AllocatorPtr allocator_holder() noexcept {
        return allocator_holder_;
    }
    
    virtual bool is_castable_to(uint64_t type_code) const noexcept {
        return CONTAINER_HASH == type_code;
    }
    
    virtual uint64_t type_hash() noexcept {
        return CONTAINER_HASH;
    }
    
    virtual U8String describe_type() const noexcept {
        return TypeNameFactory<ContainerTypeName>::name().to_u8();
    }

    virtual U8String describe_datatype() const noexcept {
        return make_datatype_signature<ContainerTypeName>().name();
    }

    PairPtr& pair() noexcept {return pair_;}
    const PairPtr& pair() const noexcept {return pair_;}

    VoidResult set_root(const BlockID &root) noexcept
    {
        root_ = root;
        return self().store().setRoot(self().master_name(), root);
    }

    void set_root_id(const BlockID &root) noexcept
    {
        root_ = root;
    }

    const BlockID &root() const noexcept
    {
        return root_;
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

        using CtrReferenceableResult = Result<CtrSharedPtr<CtrReferenceable<ProfileT>>>;



        virtual CtrReferenceableResult create_instance(
                const AllocatorPtr& allocator,
                const CtrID& ctr_id,
                const LDTypeDeclarationView& type_decl
        ) const
        {
            using ResultT = CtrReferenceableResult;
            boost::any obj = DataTypeRegistry::local().create_object(type_decl);

            MaybeError maybe_error;

            auto instance = ctr_make_shared<CtrT<ContainerTypeName>>(
                maybe_error, allocator, ctr_id, *boost::any_cast<ContainerTypeName>(&obj)
            );

            if (!maybe_error) {
                return ResultT::of(std::move(instance));
            }
            else {
                return std::move(maybe_error.get());
            }
        }


        virtual CtrReferenceableResult create_instance(
                Allocator* allocator,
                const CtrID& ctr_id,
                const LDTypeDeclarationView& type_decl
        ) const
        {
            using ResultT = Result<CtrSharedPtr<CtrReferenceable<ProfileT>>>;
            boost::any obj = DataTypeRegistry::local().create_object(type_decl);

            MaybeError maybe_error;

            auto instance = ctr_make_shared<CtrT<ContainerTypeName>>(
                maybe_error, allocator, ctr_id, *boost::any_cast<ContainerTypeName>(&obj)
            );

            if (!maybe_error) {
                return ResultT::of(std::move(instance));
            }
            else {
                return std::move(maybe_error.get());
            }
        }

//        virtual CtrReferenceableResult create_instance(
//                Allocator* allocator,
//                const BlockG& root_block
//        ) const
//        {
//            using ResultT = Result<CtrSharedPtr<CtrReferenceable<ProfileT>>>;

//            MaybeError maybe_error;

//            auto instance = ctr_make_shared<CtrT<ContainerTypeName>>(
//                maybe_error, allocator, root_block
//            );

//            if (!maybe_error) {
//                return ResultT::of(std::move(instance));
//            }
//            else {
//                return std::move(maybe_error.get());
//            }
//        }

    };

    struct CtrInterfaceImpl: public ContainerOperations<ProfileT> {

        using CIBase = ContainerOperations<ProfileT>;    
        using typename CIBase::BlockCallbackFn;

        virtual U8String data_type_decl_signature() const noexcept {
            return make_datatype_signature<ContainerTypeName>().name();
        }

        virtual U8String ctr_name() const noexcept
        {
            return TypeNameFactory<Name>::name();
        }

        VoidResult with_ctr(const CtrID& ctr_id, const AllocatorPtr& allocator, std::function<VoidResult (MyType&)> fn) const noexcept
        {
            MEMORIA_TRY(ctr_ref, allocator->find(ctr_id));
            if (ctr_ref)
            {
                auto ctr_ptr = memoria_static_pointer_cast<MyType>(ctr_ref);
                return fn(*ctr_ptr.get());
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("No container is found for id {}", ctr_id);
            }
        }

        virtual BoolResult check(const CtrID& ctr_id, AllocatorPtr allocator) const noexcept
        {
            bool result = false;

            auto res = with_ctr(ctr_id, allocator, [&](MyType& ctr) noexcept -> VoidResult {
                MEMORIA_TRY(res, ctr.check(nullptr));
                result = res;
                return VoidResult::of();
            });
            MEMORIA_RETURN_IF_ERROR(res);

            return BoolResult::of(result);
        }

        virtual VoidResult walk(
                const UUID& ctr_id,
                AllocatorPtr allocator,
                ContainerWalker<ProfileT>* walker
        ) const noexcept
        {
            return with_ctr(ctr_id, allocator, [&](MyType& ctr) noexcept{
                return ctr.ctr_walk_tree(walker);
            });
        }

        virtual VoidResult drop(const CtrID& ctr_id, AllocatorPtr allocator) const noexcept
        {
            return with_ctr(ctr_id, allocator, [&](MyType& ctr){
                return ctr.drop();
            });
        }

        virtual U8String ctr_type_name() const noexcept
        {
            return TypeNameFactory<ContainerTypeName>::name();
        }


        class CtrNodesWalkerAdapter: public ContainerWalkerBase<ProfileT> {

        	BlockCallbackFn consumer_;
        public:
        	CtrNodesWalkerAdapter(BlockCallbackFn consumer): consumer_(consumer)
        	{}

            virtual void beginRoot(int32_t idx, const BlockType* block) {
                ctr_begin_node(idx, block);
            }


            virtual void ctr_begin_node(int32_t idx, const BlockType* block) {
                consumer_(block->uuid(), block->id(), block).get_or_throw();
            }

            virtual void rootLeaf(int32_t idx, const BlockType* block) {
                ctr_begin_node(idx, block);
            }

            virtual void leaf(int32_t idx, const BlockType* block) {
                ctr_begin_node(idx, block);
            }
        };



        virtual VoidResult for_each_ctr_node(
            const CtrID& ctr_id,
            AllocatorPtr allocator,
            BlockCallbackFn consumer
        ) const noexcept
        {
            CtrNodesWalkerAdapter walker(consumer);

            return with_ctr(ctr_id, allocator, [&](MyType& ctr) noexcept {
                return ctr.ctr_walk_tree(&walker);
            });
        }
        
        virtual Result<CtrSharedPtr<CtrReferenceable<typename Types::Profile>>> new_ctr_instance(
            const ProfileBlockG<typename Types::Profile>& root_block,
            AllocatorPtr allocator
        ) const noexcept
        {
            using ResultT = Result<CtrSharedPtr<CtrReferenceable<typename Types::Profile>>>;

            MaybeError maybe_error;
            auto instance = ctr_make_shared<SharedCtr<ContainerTypeName, Allocator, typename Types::Profile>> (
                    maybe_error,
                    allocator,
                    root_block
            );

            if (!maybe_error) {
                return ResultT::of(std::move(instance));
            }
            else {
                return std::move(maybe_error.get());
            }
        }

        virtual Result<CtrSharedPtr<CtrReferenceable<typename Types::Profile>>> new_ctr_instance(
            const ProfileBlockG<typename Types::Profile>& root_block,
            ProfileAllocatorType<ProfileT>* allocator
        ) const noexcept
        {
            using ResultT = Result<CtrSharedPtr<CtrReferenceable<typename Types::Profile>>>;

            MaybeError maybe_error;
            auto instance = ctr_make_shared<SharedCtr<ContainerTypeName, Allocator, typename Types::Profile>> (
                    maybe_error,
                    allocator,
                    root_block
            );

            if (!maybe_error) {
                return ResultT::of(std::move(instance));
            }
            else {
                return std::move(maybe_error.get());
            }
        }

        virtual Result<CtrID> clone_ctr(const CtrID& ctr_id, const CtrID& new_ctr_id, AllocatorPtr allocator) const noexcept
        {
            using ResultT = Result<CtrID>;

            CtrID new_name_rtn{};

            auto res = with_ctr(ctr_id, allocator, [&](MyType& ctr) noexcept -> VoidResult {
                MEMORIA_TRY(clone_res, ctr.clone(new_ctr_id));
                new_name_rtn = clone_res;
                return VoidResult::of();
            });
            MEMORIA_RETURN_IF_ERROR(res);

            return ResultT::of(new_name_rtn);
        }

        virtual Result<CtrBlockDescription<ProfileT>> describe_block1(const BlockID& block_id, AllocatorPtr allocator) const noexcept
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

    CtrID getModelName(const BlockID root_id)
    {
        MMA_THROW(UnsupportedOperationException()) << WhatCInfo("getModelName");
    }

    Result<CtrID> clone(const CtrID& new_name) noexcept
    {
        return MEMORIA_MAKE_GENERIC_ERROR("Clone operation is not supported for this container");
    }

    bool is_updatable() const noexcept {
        return self().store().isActive();
    }

    virtual VoidResult flush() noexcept {
        return VoidResult::of();
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
    MyType& self() noexcept
    {
        return *static_cast<MyType*>(this);
    }

    const MyType& self() const noexcept
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
public:
    CtrHelper(MaybeError& maybe_error) noexcept:
        Base(maybe_error)
    {}

    virtual ~CtrHelper() noexcept {}
};

template <typename Types>
class CtrHelper<-1, Types>: public Types::template BaseFactory<Types> {
    using ThisType = CtrHelper<-1, Types>;
    using MyType   = Ctr<Types>;
    using Base     = typename Types::template BaseFactory<Types>;

public:
    CtrHelper(MaybeError& maybe_error) noexcept: Base(maybe_error) {}

    virtual ~CtrHelper() noexcept {}

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

public:
    CtrStart(MaybeError& maybe_error) noexcept:
        Base(maybe_error)
    {}
};




template <typename Types>
class Ctr: public CtrStart<Types> {
    using Base = CtrStart<Types>;

public:
    using MyType = Ctr<Types>;

    using Allocator = typename Types::Allocator;
    using BlockG    = typename Types::Allocator::BlockG;
    using BlockID   = typename Allocator::BlockID;
    using CtrID     = typename Allocator::CtrID;

public:

    using ContainerTypeName = typename Types::ContainerTypeName;
    using Name = ContainerTypeName;

private:

    Allocator*  allocator_;
    CtrID       name_;
    Logger      logger_;
    static Logger class_logger_;

protected:
    CtrSharedPtr<Allocator> alloc_holder_;

public:

    // create new ctr
    Ctr(
        MaybeError& maybe_error,
        const CtrSharedPtr<Allocator>& allocator,
        const CtrID& ctr_id,
        const ContainerTypeName& ctr_type_name
    ) noexcept:
        Base(maybe_error)
    {
        wrap_construction(maybe_error, [&]() -> VoidResult {
            Base::allocator_holder_ = allocator;

            allocator_ = allocator.get();
            name_ = ctr_id;

            MEMORIA_TRY_VOID(this->do_create_ctr(ctr_id, ctr_type_name));
            MEMORIA_TRY_VOID(allocator_->registerCtr(ctr_id, this));
            return VoidResult::of();
        });
    }

    // create new ctr
    Ctr(
        MaybeError& maybe_error,
        Allocator* allocator,
        const CtrID& ctr_id,
        const ContainerTypeName& ctr_type_name
    ) noexcept:
        Base(maybe_error)
    {
        wrap_construction(maybe_error, [&]() -> VoidResult {
            allocator_ = allocator;
            name_ = ctr_id;

            MEMORIA_TRY_VOID(this->do_create_ctr(ctr_id, ctr_type_name));

            this->do_unregister_ = false;
            return VoidResult::of();
        });
    }

    // find existing ctr
    Ctr(
        MaybeError& maybe_error,
        const CtrSharedPtr<Allocator>& allocator,
        const typename Allocator::BlockG& root_block
    ) noexcept :
        Base(maybe_error)
    {
        wrap_construction(maybe_error, [&]() -> VoidResult {
            Base::allocator_holder_ = allocator;

            allocator_ = allocator.get();

            initLogger();

            MEMORIA_TRY(name, this->do_init_ctr(root_block));

            name_ = name;

            MEMORIA_TRY_VOID(allocator_->registerCtr(name_, this));

            return VoidResult::of();
        });
    }


    // find existing ctr
    Ctr(
        MaybeError& maybe_error,
        Allocator* allocator,
        const typename Allocator::BlockG& root_block
    ) noexcept :
        Base(maybe_error)
    {
        wrap_construction(maybe_error, [&]() -> VoidResult {
            allocator_ = allocator;

            initLogger();

            MEMORIA_TRY(name, this->do_init_ctr(root_block));

            name_ = name;

            this->do_unregister_ = false;

            return VoidResult::of();
        });
    }


    Ctr(const MyType& other) = delete;
    Ctr(MyType&& other) = delete;

    virtual ~Ctr() noexcept
    {
        if (this->do_unregister_on_dtr_ && this->do_unregister_)
        {
            try {
                allocator_->unregisterCtr(name_, this).get_or_throw();
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
    }

    void initLogger() noexcept
    {
        logger_.configure(TypeNameFactory<ContainerTypeName>::cname(), Logger::DERIVED, &allocator_->logger());
    }



    Allocator& store() noexcept {
        return *allocator_;
    }

    Allocator& store() const noexcept {
        return *allocator_;
    }

    static U8String type_name_str() noexcept
    {
        return TypeNameFactory<ContainerTypeName>::name();
    }

    static const char* type_name_cstr() noexcept
    {
        return TypeNameFactory<ContainerTypeName>::cname();
    }


    bool is_log(int32_t level) const noexcept
    {
        return logger_.isLogEnabled(level);
    }

    const Logger& logger() const noexcept {
        return logger_;
    }

    Logger& logger() noexcept {
        return logger_;
    }

    static Logger& class_logger() noexcept {
        return class_logger_;
    }

    virtual const CtrID& name() const noexcept {
        return name_;
    }

    const auto& master_name() const noexcept
    {
        return name_;
    }
};

template<
        typename Types
>
Logger Ctr<Types>::class_logger_(typeid(typename Types::ContainerTypeName).name(), Logger::DERIVED, &logger);


}
