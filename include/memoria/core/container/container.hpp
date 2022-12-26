
// Copyright 2011-2022 Victor Smirnov
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

#include <memoria/core/memory/object_pool.hpp>

#include <string>
#include <memory>
#include <unordered_set>

namespace memoria {

template <typename Profile, typename SelectorType, typename ContainerTypeName = SelectorType> class CtrTF;

template <typename Name, typename Base, typename Types> class CtrPart;

template <typename Types> class Ctr;
template <typename Types> class CtrStart;
template <typename Types> class Iter;
template <typename CtrName, typename Profile> class SharedIter;
template <typename CtrName, typename Allocator, typename Profile> class SharedCtr;

template <typename TypesType>
class ROCtrBase:
        public ::memoria::ICtrApi<typename TypesType::ContainerTypeName, ApiProfile<typename TypesType::Profile>>,
        public CtrSharedFromThis<CtrStart<TypesType>>
{
public:

    using ThisType  = ROCtrBase<TypesType>;
    using MyType    = CtrStart<TypesType>;

    using ContainerTypeName = typename TypesType::ContainerTypeName;
    using Name              = ContainerTypeName;
    using Types             = TypesType;

    using ProfileT          = typename Types::Profile;
    using ApiProfileT       = ApiProfile<ProfileT>;

    using TreeNodePtr = typename Types::TreeNodePtr;
    
    using ROAllocator = ProfileStoreType<ProfileT>;


    using BlockID   = typename ROAllocator::BlockID;
    using BlockType = typename ROAllocator::BlockType;
    using CtrID     = typename ROAllocator::CtrID;
    using SharedBlockPtr        = typename ROAllocator::SharedBlockPtr;
    using SharedBlockConstPtr   = typename ROAllocator::SharedBlockConstPtr;

    using BlockIteratorState    = Iter<typename Types::BlockIterStateTypes>;
    using BlockIteratorStatePtr = IterSharedPtr<BlockIteratorState>;

    using ROAllocatorPtr    = SnpSharedPtr<ROAllocator>;

    using NodeDispatcher = typename Types::Blocks::template NodeDispatcher<MyType>;

    static constexpr uint64_t CONTAINER_HASH = TypeHashV<Name>;

    template <typename> friend class BTIteratorBase;

    template <typename> friend class Iter;
    template <typename, typename, typename> friend class IterPart;
    template <typename> friend class IterStart;

    template <typename> friend class CtrStart;
    template <typename, typename, typename> friend class CtrPart;
    template <typename> friend class Ctr;

protected:
    BlockID root_{};

    PairPtr pair_;
    
    ROAllocatorPtr store_holder_;
    ROAllocator*   allocator_{};
    CtrID name_;

public:
    ROCtrBase() noexcept {}

    virtual ~ROCtrBase() noexcept {}

    ROAllocator& store() noexcept {
        return *allocator_;
    }

    ROAllocator& store() const noexcept {
        return *allocator_;
    }

    const CtrID& name() const noexcept {
        return name_;
    }

    const CtrID& master_name() const noexcept {
        return name_;
    }

    const std::type_info& api_type_info() const noexcept {
        return typeid(::memoria::ICtrApi<typename TypesType::ContainerTypeName, ApiProfile<typename TypesType::Profile>>);
    }

    auto make_shared_ptr() const noexcept {
        return this->shared_from_this();
    }

    auto make_shared_ptr() noexcept {
        return this->shared_from_this();
    }

    // TODO: error handling
    virtual CtrSharedPtr<CtrReferenceable<ApiProfile<ProfileT>>> shared_self() noexcept {
        return this->shared_from_this();
    }

    virtual CtrSharedPtr<const CtrReferenceable<ApiProfile<ProfileT>>> shared_self() const noexcept {
        return this->shared_from_this();
    }
    
    bool isNew() const noexcept {
        return root_.is_null();
    }
    
    ROAllocatorPtr allocator_holder() noexcept {
        return store_holder_;
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

    void set_root(const BlockID &root)
    {
        root_ = root;
        return self().store().setRoot(self().master_name(), root);
    }

    void set_root_id(const BlockID &root) noexcept {
        root_ = root;
    }

    const BlockID &root() const noexcept {
        return root_;
    }

    static uint64_t hash() noexcept {
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
        using CtrT = SharedCtr<
            CtrName,
            ROAllocator,
            ProfileT
        >;

        template <typename CtrName>
        using RWCtrT = RWSharedCtr<
            CtrName,
            ROAllocator,
            ProfileT
        >;

        using CtrReferenceablePtrT = CtrSharedPtr<CtrReferenceable<ApiProfile<ProfileT>>>;
        using CtrReferenceableUPtrT = std::unique_ptr<CtrReferenceable<ApiProfile<ProfileT>>>;

        virtual CtrReferenceableUPtrT create_ctr_instance(
                const ROAllocatorPtr& allocator,
                const CtrID& ctr_id,
                const hermes::Datatype& type_decl,
                bool writable
        ) const {
            boost::any obj = get_cxx_instance(type_decl);

            if (writable) {
                return std::make_unique<RWCtrT<ContainerTypeName>>(
                    allocator, ctr_id, *boost::any_cast<ContainerTypeName>(&obj)
                );
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Can't create containers in read-only snapshots").do_throw();
            }
        }
    };

    struct CtrInterfaceImpl: public ContainerOperations<ProfileT> {

        using CIBase = ContainerOperations<ProfileT>;    
        using typename CIBase::BlockCallbackFn;

        using CtrT   = SharedCtr<ContainerTypeName, ROAllocator, ProfileT>;
        using RWCtrT = RWSharedCtr<ContainerTypeName, ROAllocator, ProfileT>;

        using CtrReferenceablePtrT  = CtrSharedPtr<CtrReferenceable<ApiProfile<ProfileT>>>;
        using CtrReferenceableUPtrT = std::unique_ptr<CtrReferenceable<ApiProfile<ProfileT>>>;

        virtual U8String data_type_decl_signature() const {
            return make_datatype_signature<ContainerTypeName>().name();
        }

        virtual U8String ctr_name() const
        {
            return TypeNameFactory<Name>::name();
        }

        void with_ctr(const CtrID& ctr_id, const ROAllocatorPtr& allocator, std::function<void (MyType&)> fn) const
        {
            auto ctr_ref = allocator->find(ctr_id);
            if (ctr_ref)
            {
                auto ctr_ptr = memoria_static_pointer_cast<MyType>(ctr_ref);
                return fn(*ctr_ptr.get());
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("No container is found for id {}", ctr_id).do_throw();
            }
        }

        virtual void check(const CtrID& ctr_id, ROAllocatorPtr allocator, const CheckResultConsumerFn& consumer) const
        {
            with_ctr(ctr_id, allocator, [&](MyType& ctr) {
                ctr.check(consumer);
            });
        }

        virtual void walk(
                const CtrID& ctr_id,
                ROAllocatorPtr allocator,
                ContainerWalker<ProfileT>* walker
        ) const
        {
            return with_ctr(ctr_id, allocator, [&](MyType& ctr) {
                return ctr.ctr_walk_tree(walker);
            });
        }

        virtual U8String ctr_type_name() const
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
                consumer_(block->uid(), block->id(), block);
            }

            virtual void rootLeaf(int32_t idx, const BlockType* block) {
                ctr_begin_node(idx, block);
            }

            virtual void leaf(int32_t idx, const BlockType* block) {
                ctr_begin_node(idx, block);
            }
        };



        virtual void for_each_ctr_node(
            const CtrID& ctr_id,
            ROAllocatorPtr allocator,
            BlockCallbackFn consumer
        ) const
        {
            CtrNodesWalkerAdapter walker(consumer);

            return with_ctr(ctr_id, allocator, [&](MyType& ctr) {
                return ctr.ctr_walk_tree(&walker);
            });
        }

        virtual CtrID get_ctr_id(
            const SharedBlockConstPtr& root_block
        ) const {
            return MyType::ctr_get_model_name(root_block);
        }
        
        virtual CtrReferenceablePtrT create_ctr_instance(
            const SharedBlockConstPtr& root_block,
            ROAllocator* allocator,
            bool writable
        ) const
        {
            if (writable) {
                return allocate_shared<RWCtrT> (
                            allocator->object_pools(),
                            allocator,
                            root_block
                );
            }
            else {
                return allocate_shared<CtrT> (
                            allocator->object_pools(),
                            allocator,
                            root_block
                );
            }
        }

        virtual CtrReferenceableUPtrT create_ctr_instance(
                const ROAllocatorPtr& allocator,
                SharedBlockConstPtr root,
                bool writable
        ) const {

            if (writable) {
                return std::make_unique<RWCtrT>(allocator, root);
            }
            else {
                return std::make_unique<CtrT>(allocator, root);
            }
        }

        virtual CtrID clone_ctr(
                const CtrID& ctr_id,
                const CtrID& new_ctr_id,
                ROAllocatorPtr allocator
        ) const {
            CtrID new_name_rtn{};

            with_ctr(ctr_id, allocator, [&](MyType& ctr) {
                auto clone_res = ctr.clone(new_ctr_id);
                new_name_rtn = clone_res;
            });

            return new_name_rtn;
        }

        virtual CtrBlockDescription<ApiProfile<ProfileT>> describe_block1(
                const BlockID& block_id,
                ROAllocatorPtr allocator
        ) const {
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

    CtrID getModelName(const BlockID root_id) {
        MMA_THROW(UnsupportedOperationException()) << WhatCInfo("getModelName");
    }

    CtrID clone(const CtrID& new_name) {
        MEMORIA_MAKE_GENERIC_ERROR("Clone operation is not supported for this container").do_throw();
    }

    bool is_updatable() const {
        return self().store().isActive();
    }

    virtual void flush() {}

protected:



public:

    template <typename IteratorStateT>
    IterSharedPtr<IteratorStateT> make_block_iterator_state(TypeTag<IteratorStateT> = TypeTag<BlockIteratorState>{}) const {
        static_assert(
            std::is_base_of_v<BlockIteratorState, IteratorStateT>,
            "State iterators must derive from container's BlockIteratorState"
        );

        auto state = get_reusable_shared_instance<IteratorStateT>(
            self().store().object_pools()
        );

        state->iter_initialize(this->shared_from_this());

        return state;
    }


    virtual void internal_detouch_from_store() noexcept {
        store_holder_.reset();
    }

    virtual void internal_attach_to_store(SnpSharedPtr<IStoreApiBase<ApiProfileT>> store) noexcept {
        store_holder_ = memoria_static_pointer_cast<ROAllocator>(store);
        allocator_ = store_holder_.get();
    }

    virtual void internal_configure_shared_from_this(
            SharedPtrHolder* holder) noexcept
    {
        pool::detail::SharedFromThisHelper<MyType>::initialize(&self(), holder);
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


template <typename TypesType>
class RWCtrBase: public CtrStart<typename TypesType::ROTypes> {
    using Base = CtrStart<typename TypesType::ROTypes>;
public:
    using MyType  = CtrStart<TypesType>;
    using MyROType  = Base;
    using ROTypes = typename TypesType::ROTypes;
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
    CtrHelper():
        Base()
    {}

    virtual ~CtrHelper() noexcept {}
};

template <typename Types>
class CtrHelper<-1, Types>: public Types::template BaseFactory<Types> {
    using ThisType = CtrHelper<-1, Types>;
    using MyType   = Ctr<Types>;
    using Base     = typename Types::template BaseFactory<Types>;

public:
    CtrHelper(): Base() {}

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
    CtrStart():
        Base()
    {}
};


template <typename> class BTreeCtrBase;

template <typename Types>
class Ctr: public CtrStart<Types> {
    using Base = CtrStart<Types>;
public:
    using CommonCtr = Base;

    using MyType = Ctr<Types>;
    using ProfileT = typename Types::Profile;

    using ROAllocator = ProfileStoreType<ProfileT>;

    using SharedBlockPtr = ProfileSharedBlockPtr<ProfileT>;
    using SharedBlockConstPtr = ProfileSharedBlockConstPtr<ProfileT>;

    using BlockID   = ProfileBlockID<ProfileT>;
    using CtrID     = ProfileCtrID<ProfileT>;

public:

    using ContainerTypeName = typename Types::ContainerTypeName;
    using Name = ContainerTypeName;

public:

    // create new ctr
    Ctr(
        const SnpSharedPtr<ROAllocator>& allocator,
        const CtrID& ctr_id,
        const ContainerTypeName& ctr_type_name
    ):
        Base()
    {
        Base::store_holder_ = allocator;
        Base::allocator_    = allocator.get();

        Base::name_ = ctr_id;

        this->do_create_ctr(ctr_id, ctr_type_name);        
    }



    // find existing ctr
    Ctr(
        const SnpSharedPtr<ROAllocator>& allocator,
        const SharedBlockConstPtr& root_block
    ):
        Base()
    {
        Base::store_holder_ = allocator;
        Base::allocator_ = allocator.get();

        auto name = this->do_init_ctr(root_block);
        Base::name_ = name;
    }


    // find existing ctr (for store-internal use only!)
    Ctr(
        ROAllocator* allocator,
        const SharedBlockConstPtr& root_block
    ):
        Base()
    {
        Base::allocator_ = allocator;

        auto name = this->do_init_ctr(root_block);
        Base::name_ = name;
    }



    Ctr(const MyType& other) = delete;
    Ctr(MyType&& other) = delete;
};


}
