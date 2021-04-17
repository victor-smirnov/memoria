
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

#include <memoria/profiles/common/block_operations.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/assert.hpp>
#include <memoria/core/tools/platform.hpp>
#include <memoria/core/tools/uuid.hpp>
#include <memoria/core/memory/memory.hpp>
#include <memoria/core/container/ctr_referenceable.hpp>

#include <memoria/profiles/common/common.hpp>

#include <memoria/filesystem/operations.hpp>
#include <memoria/filesystem/path.hpp>

#include <memoria/core/datatypes/type_signature.hpp>

#include <memoria/core/linked/document/linked_document.hpp>

#include <stack>
#include <sstream>
#include <fstream>
#include <ostream>
#include <mutex>

namespace memoria {
    
template <typename Profile> class MetadataRepository;

template <typename Profile>
struct ContainerWalker {

    using BlockType = ProfileBlockType<Profile>;
    using BlockID   = ProfileBlockID<Profile>;
    using CtrID     = ProfileCtrID<Profile>;

    virtual void beginAllocator(const char* type, const char* desc)             = 0;
    virtual void endAllocator()                                                 = 0;

    virtual void beginSnapshot(const char* descr)                               = 0;
    virtual void endSnapshot()                                                  = 0;

    virtual void beginSnapshotSet(const char* descr, size_t number)             = 0;
    virtual void endSnapshotSet()                                               = 0;

    virtual void beginCompositeCtr(const char* descr, const CtrID& name)        = 0;
    virtual void endCompositeCtr()                                              = 0;

    virtual void beginCtr(const char* descr, const CtrID& name, const BlockID& root) = 0;
    virtual void endCtr()                                                       = 0;

    virtual void beginRoot(const BlockType* block)                              = 0;
    virtual void endRoot()                                                      = 0;

    virtual void ctr_begin_node(const BlockType* block)                         = 0;
    virtual void ctr_end_node()                                                 = 0;

    virtual void rootLeaf(const BlockType* block)                               = 0;
    virtual void leaf(const BlockType* block)                                   = 0;

    virtual void singleNode(const char* descr, const BlockType* block)          = 0;

    virtual void beginSection(const char* name)                                 = 0;
    virtual void endSection()                                                   = 0;

    virtual void content(const char* name, const char* content)                 = 0;

    virtual ~ContainerWalker() noexcept {}
};


template <typename Profile>
struct ContainerWalkerBase: ContainerWalker<Profile> {

    using Base = ContainerWalker<Profile>;

    using typename Base::BlockID;
    using typename Base::BlockType;
    using typename Base::CtrID;

    virtual void beginAllocator(const char* type, const char* desc) {}
    virtual void endAllocator() {}

    virtual void beginSnapshot(const char* descr) {}
    virtual void endSnapshot() {}

    virtual void beginSnapshotSet(const char* descr, size_t number) {}
    virtual void endSnapshotSet() {}

    virtual void beginCompositeCtr(const char* descr, const CtrID& name) {}
    virtual void endCompositeCtr() {}

    virtual void beginCtr(const char* descr, const CtrID& name, const BlockID& root) {}
    virtual void endCtr() {}


    virtual void beginRoot(const BlockType* block) {}
    virtual void endRoot() {}

    virtual void ctr_begin_node(const BlockType* block) {}
    virtual void ctr_end_node() {}

    virtual void rootLeaf(const BlockType* block) {}
    virtual void leaf(const BlockType* block) {}


    virtual void singleNode(const char* descr, const BlockType* block) {}

    virtual void beginSection(const char* name) {}
    virtual void endSection() {}

    virtual void content(const char* name, const char* content) {}

    virtual ~ContainerWalkerBase() noexcept {}
};



template <typename Profile>
class CtrBlockDescription {

    using CtrID = ApiProfileCtrID<Profile>;

    int32_t size_;
    UUID ctr_name_;
    bool root_;
    bool leaf_;
    uint64_t offset_;
public:
    CtrBlockDescription(int32_t size, const CtrID& ctr_name, bool root, bool leaf, uint64_t offset):
        size_(size), ctr_name_(ctr_name), root_(root), leaf_(leaf), offset_(offset)
    {}

    int32_t size() const {return size_;}
    const CtrID& ctr_name1() const {return ctr_name_;}
    bool is_root() const {return root_;}
    bool is_leaf() const {return leaf_;}
    bool is_branch() const {return !is_leaf();}
    uint64_t offset() const {return offset_;}
};

template <typename Profile> struct IAllocator;

template <typename Profile>
struct ContainerOperationsBase {

    using ApiProfileT = ApiProfile<Profile>;

    using BlockType = ProfileBlockType<Profile>;
    using BlockID   = ProfileBlockID<Profile>;
    using BlockGUID = ProfileBlockGUID<Profile>;
    using CtrID     = ProfileCtrID<Profile>;

    virtual ~ContainerOperationsBase() noexcept {}

    // uuid, id, block data
    using BlockCallbackFn = std::function<VoidResult (const BlockGUID&, const BlockID&, const BlockType*)>;
    using Allocator = ProfileAllocatorType<Profile>;
    using AllocatorBasePtr = SnpSharedPtr<ProfileAllocatorType<Profile>>;

    virtual U8String data_type_decl_signature() const noexcept = 0;

    // FIXME: remove name from parameters, it's already in Ctr's block root metadata
    virtual U8String ctr_name() const noexcept = 0;

    virtual BoolResult check(
        const CtrID& name,
        AllocatorBasePtr allocator
    ) const noexcept = 0;

    virtual VoidResult walk(
            const CtrID& name,
            AllocatorBasePtr allocator,
            ContainerWalker<Profile>* walker
    ) const noexcept = 0;


    virtual U8String ctr_type_name() const noexcept = 0;

    virtual VoidResult drop(
            const CtrID& name,
            AllocatorBasePtr allocator
    ) const noexcept = 0;


    virtual VoidResult for_each_ctr_node(
        const CtrID& name,
        AllocatorBasePtr allocator,
        BlockCallbackFn consumer
    ) const noexcept = 0;
    
    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>> new_ctr_instance(
        const ProfileSharedBlockConstPtr<Profile>& root_block,
        AllocatorBasePtr allocator
    ) const noexcept = 0;

    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>> new_ctr_instance(
        const ProfileSharedBlockConstPtr<Profile>& root_block,
        Allocator* allocator
    ) const noexcept = 0;


    virtual Result<CtrID> clone_ctr(
        const CtrID& name,
        const CtrID& new_name,
        AllocatorBasePtr allocator
    ) const noexcept = 0;

    virtual Result<CtrBlockDescription<ApiProfileT>> describe_block1(
        const BlockID& block_id,
        AllocatorBasePtr allocator
    ) const noexcept = 0;
};


template <typename Profile>
struct ContainerOperations: ContainerOperationsBase<Profile> {

};


template <typename Profile>
using ContainerOperationsPtr = std::shared_ptr<ContainerOperations<Profile>>;

template <typename Profile>
struct BTreeTraverseNodeHandler {
    using BlockType = ProfileBlockType<Profile>;
    using BlockID   = ProfileBlockID<Profile>;


    virtual VoidResult process_branch_node(const BlockType* block) noexcept = 0;
    virtual VoidResult process_leaf_node(const BlockType* block) noexcept = 0;
    virtual VoidResult process_directory_leaf_node(const BlockType* block) noexcept = 0;

    virtual BoolResult proceed_with(const BlockID& block_id) const noexcept = 0;
};


template<typename Profile>
struct CtrInstanceFactory {

    virtual ~CtrInstanceFactory() noexcept {}

    using CtrID         = ProfileCtrID<Profile>;
    using Allocator     = ProfileAllocatorType<Profile>;
    using AllocatorPtr  = SnpSharedPtr<Allocator>;

    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfile<Profile>>>> create_instance(
            const AllocatorPtr& allocator,
            const CtrID& ctr_id,
            const LDTypeDeclarationView& type_decl
    ) const = 0;

    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfile<Profile>>>> create_instance(
            Allocator* allocator,
            const CtrID& ctr_id,
            const LDTypeDeclarationView& type_decl
    ) const = 0;


};

template <typename Profile>
using ContainerInstanceFactoryPtr = std::shared_ptr<CtrInstanceFactory<Profile>>;


}
