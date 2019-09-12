
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

#include <memoria/v1/profiles/common/block_operations.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/tools/assert.hpp>
#include <memoria/v1/core/tools/platform.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/memory.hpp>
#include <memoria/v1/core/container/ctr_referenceable.hpp>

#include <memoria/v1/profiles/common/common.hpp>

#include <memoria/v1/filesystem/operations.hpp>
#include <memoria/v1/filesystem/path.hpp>
#include <memoria/v1/reactor/file_streams.hpp>
#include <memoria/v1/api/datatypes/type_signature.hpp>

#include <memoria/v1/core/graph/graph.hpp>

#include <boost/filesystem.hpp>

#include <stack>
#include <sstream>
#include <fstream>
#include <ostream>
#include <mutex>

namespace memoria {
namespace v1 {
    
template <typename Profile> class MetadataRepository;

template <typename Profile>
struct ContainerWalker {

    using BlockType = ProfileBlockType<Profile>;
    using BlockID   = ProfileBlockID<Profile>;
    using CtrID     = ProfileCtrID<Profile>;

    virtual void beginAllocator(const char16_t* type, const char16_t* desc)         = 0;
    virtual void endAllocator()                                                     = 0;

    virtual void beginSnapshot(const char16_t* descr)                               = 0;
    virtual void endSnapshot()                                                      = 0;

    virtual void beginSnapshotSet(const char16_t* descr, size_t number)             = 0;
    virtual void endSnapshotSet()                                                   = 0;

    virtual void beginCompositeCtr(const char16_t* descr, const CtrID& name)        = 0;
    virtual void endCompositeCtr()                                                  = 0;

    virtual void beginCtr(const char16_t* descr, const CtrID& name, const CtrID& root) = 0;
    virtual void endCtr()                                                        = 0;

    virtual void beginRoot(int32_t idx, const BlockType* block)                  = 0;
    virtual void endRoot()                                                       = 0;

    virtual void beginNode(int32_t idx, const BlockType* block)                  = 0;
    virtual void endNode()                                                       = 0;

    virtual void rootLeaf(int32_t idx, const BlockType* block)                   = 0;
    virtual void leaf(int32_t idx, const BlockType* block)                       = 0;

    virtual void singleNode(const char16_t* descr, const BlockType* block)       = 0;

    virtual void beginSection(const char16_t* name)                              = 0;
    virtual void endSection()                                                    = 0;

    virtual void content(const char16_t* name, const char16_t* content)          = 0;

    virtual ~ContainerWalker() noexcept {}
};


template <typename Profile>
struct ContainerWalkerBase: ContainerWalker<Profile> {

    using Base = ContainerWalker<Profile>;

    using typename Base::BlockID;
    using typename Base::BlockType;
    using typename Base::CtrID;

    virtual void beginAllocator(const char16_t* type, const char16_t* desc) {}
    virtual void endAllocator() {}

    virtual void beginSnapshot(const char16_t* descr) {}
    virtual void endSnapshot() {}

    virtual void beginSnapshotSet(const char16_t* descr, size_t number) {}
    virtual void endSnapshotSet() {}

    virtual void beginCompositeCtr(const char16_t* descr, const CtrID& name) {}
    virtual void endCompositeCtr() {}

    virtual void beginCtr(const char16_t* descr, const CtrID& name, const BlockID& root) {}
    virtual void endCtr() {}

    virtual void beginRoot(int32_t idx, const BlockType* block) {}
    virtual void endRoot() {}

    virtual void beginNode(int32_t idx, const BlockType* block) {}
    virtual void endNode() {}

    virtual void rootLeaf(int32_t idx, const BlockType* block) {}
    virtual void leaf(int32_t idx, const BlockType* block) {}

    virtual void singleNode(const char16_t* descr, const BlockType* block) {}

    virtual void beginSection(const char16_t* name) {}
    virtual void endSection() {}

    virtual void content(const char16_t* name, const char16_t* content) {}

    virtual ~ContainerWalkerBase() noexcept {}
};



template <typename Profile>
class CtrBlockDescription {

    using CtrID = ProfileCtrID<Profile>;

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
    const CtrID& ctr_name() const {return ctr_name_;}
    bool is_root() const {return root_;}
    bool is_leaf() const {return leaf_;}
    bool is_branch() const {return !is_leaf();}
    uint64_t offset() const {return offset_;}
};

template <typename Profile> struct IAllocator;

template <typename Profile>
struct ContainerOperations {

    using BlockType = ProfileBlockType<Profile>;
    using BlockID   = ProfileBlockID<Profile>;
    using CtrID     = ProfileCtrID<Profile>;

    virtual ~ContainerOperations() noexcept {}

    // uuid, id, block data
    using BlockCallbackFn = std::function<void(const BlockID&, const BlockID&, const BlockType*)>;
    using AllocatorBasePtr = SnpSharedPtr<IAllocator<Profile>>;


    virtual U8String data_type_decl_signature() const = 0;

    virtual Vertex describe_block(const BlockID& block_id, const CtrID& ctr_id, AllocatorBasePtr allocator) const = 0;
    virtual Collection<Edge> describe_block_links(const BlockID& block_id, const CtrID& ctr_id, AllocatorBasePtr allocator, Direction direction) const = 0;
    virtual Collection<VertexProperty> block_properties(const Vertex& vx, const BlockID& block_id, const CtrID& ctr_id, AllocatorBasePtr allocator) const = 0;


    // FIXME: remove name from parameters, it's already in Ctr's block root metadata
    virtual U16String ctr_name() const = 0;

    virtual bool check(
        const CtrID& name,
        AllocatorBasePtr allocator
    ) const = 0;

    virtual void walk(
            const CtrID& name,
            AllocatorBasePtr allocator,
            ContainerWalker<Profile>* walker
    ) const = 0;


    virtual U16String ctr_type_name() const = 0;

    virtual void drop(
            const CtrID& name,
            AllocatorBasePtr allocator
    ) const = 0;

    virtual void for_each_ctr_node(
        const CtrID& name,
        AllocatorBasePtr allocator,
        BlockCallbackFn consumer
    ) const = 0;
    
    virtual CtrSharedPtr<CtrReferenceable<Profile>> new_ctr_instance(
        const ProfileBlockG<Profile>& root_block,
        AllocatorBasePtr allocator
    ) const = 0;


    virtual CtrID clone_ctr(
        const CtrID& name,
        const CtrID& new_name,
        AllocatorBasePtr allocator
    ) const = 0;

    virtual CtrBlockDescription<Profile> describe_block1(
        const BlockID& block_id,
        AllocatorBasePtr allocator
    ) const = 0;
};

template <typename Profile>
using ContainerOperationsPtr = std::shared_ptr<ContainerOperations<Profile>>;


template<typename Profile>
struct CtrInstanceFactory {

    virtual ~CtrInstanceFactory() noexcept {}

    using CtrID         = ProfileCtrID<Profile>;
    using Allocator     = ProfileAllocatorType<Profile>;
    using AllocatorPtr  = SnpSharedPtr<Allocator>;

    virtual SnpSharedPtr<CtrReferenceable<Profile>> create_instance(
            const AllocatorPtr& allocator,
            const CtrID& ctr_id,
            const DataTypeDeclaration& type_decl
    ) const = 0;
};

template <typename Profile>
using ContainerInstanceFactoryPtr = std::shared_ptr<CtrInstanceFactory<Profile>>;


}}
