
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

#include <memoria/v1/metadata/group.hpp>
#include <memoria/v1/metadata/block.hpp>
#include <memoria/v1/metadata/tools.hpp>
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

namespace bf = boost::filesystem;    

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

struct AllocatorBase;

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


template <typename Profile>
struct ContainerInterface {

    using BlockType = ProfileBlockType<Profile>;
    using BlockID   = ProfileBlockID<Profile>;
    using CtrID     = ProfileCtrID<Profile>;

    virtual ~ContainerInterface() noexcept {}

    // uuid, id, block data
    using BlockCallbackFn = std::function<void(const BlockID&, const BlockID&, const BlockType*)>;
    using AllocatorBasePtr = SnpSharedPtr<AllocatorBase>;


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
    
    virtual CtrSharedPtr<CtrReferenceable> new_ctr_instance(
        const BlockID& root_id,
        const CtrID& name,
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
using ContainerInterfacePtr = std::shared_ptr<ContainerInterface<Profile>>;

template <typename Profile> class MetadataRepository;

template <typename Profile>
struct ContainerMetadata: public MetadataGroup {
public:

    template <typename Types>
    ContainerMetadata(U16StringRef name, Types* nothing, uint64_t ctr_hash, ContainerInterfacePtr<Profile> container_interface):
        MetadataGroup(name, buildPageMetadata<Types>()),
        container_interface_(container_interface),
        ctr_hash_(ctr_hash)
        
    {
    	MetadataGroup::set_type() = MetadataGroup::CONTAINER;
        for (uint32_t c = 0; c < content_.size(); c++)
        {
            if (content_[c]->getTypeCode() == Metadata::PAGE)
            {
                BlockMetadataPtr<Profile> block = static_pointer_cast<BlockMetadata<Profile>> (content_[c]);
                block_map_[block->hash() ^ ctr_hash] = block;
            }
            else if (content_[c]->getTypeCode() == Metadata::CONTAINER) {
                // nothing to do
            }
            else {
                //exception;
            }
        }
    }
    

    virtual ~ContainerMetadata() throw ()
    {}

    virtual uint64_t ctr_hash() const {
        return ctr_hash_;
    }

    virtual const BlockMetadataPtr<Profile>& getBlockMetadata(uint64_t model_hash, uint64_t block_hash) const
    {
        auto i = block_map_.find(model_hash ^ block_hash);
        if (i != block_map_.end())
        {
            return i->second;
        }
        else {
            MMA1_THROW(Exception()) << WhatCInfo("Unknown block type hash code");
        }
    }

    virtual const ContainerInterfacePtr<Profile>& getCtrInterface() const
    {
        return container_interface_;
    }
    

    struct RegistrationHelper {
        RegistrationHelper(const ContainerMetadataPtr<Profile>& ptr) {
            MetadataRepository<Profile>::registerMetadata(ptr);
        }
    };

private:
    
    template <typename Types>
    static auto buildPageMetadata() 
    {
        MetadataList list;
        Types::Blocks::NodeDispatcher::buildMetadataList(list);
        return list;
    }

    BlockMetadataMap<Profile>       block_map_;
    ContainerInterfacePtr<Profile>  container_interface_;

    uint64_t                        ctr_hash_;
};





template <typename Profile>
struct ContainerMetadataRepository: public MetadataGroup {

public:
    ContainerMetadataRepository(U16StringRef name, const MetadataList &content):
            MetadataGroup(name, content), hash_(0)
    {
        MetadataGroup::set_type() = Metadata::CONTAINER;

        for (size_t c = 0; c < content_.size(); c++)
        {
            if (content[c]->getTypeCode() == Metadata::CONTAINER)
            {
                ContainerMetadataPtr<Profile> model = static_pointer_cast<ContainerMetadata<Profile>> (content_[c]);
                process_model(model);
            }
            else {
                //exception;
            }
        }
    }

    virtual ~ContainerMetadataRepository() noexcept
    {
    }

    virtual uint64_t hash() const {
        return hash_;
    }



    const BlockMetadataPtr<Profile>& getBlockMetadata(uint64_t ctr_hash, uint64_t block_hash) const
    {
        auto i = block_map_.find(ctr_hash ^ block_hash);
        if (i != block_map_.end())
        {
            return i->second;
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Unknown block type hash codes {} {}", ctr_hash, block_hash));
        }
    }


    const ContainerMetadataPtr<Profile>& getContainerMetadata(uint64_t hashCode) const
    {
        auto i = model_map_.find(hashCode);
        if (i != model_map_.end())
        {
            return i->second;
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Unknown container hash code {}", hashCode));
        }
    }


    void dumpMetadata(std::ostream& out)
    {
        for (auto pair: model_map_)
        {
            if (pair.second->getCtrInterface() != nullptr)
            {
                out << pair.first << ": " << pair.second->getCtrInterface()->ctr_type_name() << std::endl;
            }
            else {
                out << pair.first << ": " << "Composite" << std::endl;
            }
        }
    }
    virtual void registerMetadata(const ContainerMetadataPtr<Profile>& metadata)
    {
    	process_model(metadata);
    }

    virtual void unregisterMetadata(const ContainerMetadataPtr<Profile>& metadata) {}



private:
    uint64_t                        hash_;
    BlockMetadataMap<Profile>       block_map_;
    ContainerMetadataMap<Profile>   model_map_;
    
    std::mutex mutex_;

    void process_model(const ContainerMetadataPtr<Profile>& model)
    {
        if (model_map_.find(model->ctr_hash()) == model_map_.end())
        {
            hash_ = hash_ + model->ctr_hash();

            model_map_[model->ctr_hash()] = model;

            for (int32_t d = 0; d < model->size(); d++)
            {
                auto item = model->getItem(d);
                if (item->getTypeCode() == Metadata::PAGE)
                {
                    auto block = static_pointer_cast<BlockMetadata<Profile>> (item);
                    block_map_[block->hash() ^ model->ctr_hash()] = block;
                }
                else if (item->getTypeCode() == Metadata::CONTAINER)
                {
                    process_model(static_pointer_cast<ContainerMetadata<Profile>>(item));
                }
                else {
                    //exception ?
                }
            }
        }
    }
};



template <typename Profile>
class MetadataRepository {

public:

    static ContainerMetadataRepository<Profile>* getMetadata()
    {
        static thread_local ContainerMetadataRepository<Profile> metadata(TypeNameFactory<Profile>::name(), MetadataList());
        return &metadata;
    }

    static void registerMetadata(const ContainerMetadataPtr<Profile>& ctr_metadata)
    {
        getMetadata()->registerMetadata(ctr_metadata);
    }

    static void unregisterMetadata(const ContainerMetadataPtr<Profile>& ctr_metadata)
    {
        getMetadata()->unregisterMetadata(ctr_metadata);
    }
};


}}
