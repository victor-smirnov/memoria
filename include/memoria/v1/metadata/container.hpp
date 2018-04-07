
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
#include <memoria/v1/metadata/page.hpp>
#include <memoria/v1/metadata/tools.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/tools/assert.hpp>
#include <memoria/v1/core/tools/platform.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/memory.hpp>
#include <memoria/v1/core/container/ctr_referenceable.hpp>

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

struct ContainerWalker {
    virtual void beginAllocator(const char16_t* type, const char16_t* desc)         = 0;
    virtual void endAllocator()                                                     = 0;

    virtual void beginSnapshot(const char16_t* descr)                               = 0;
    virtual void endSnapshot()                                                      = 0;

    virtual void beginSnapshotSet(const char16_t* descr, size_t number)             = 0;
    virtual void endSnapshotSet()                                                   = 0;

    virtual void beginCompositeCtr(const char16_t* descr, const UUID& name)         = 0;
    virtual void endCompositeCtr()                                                  = 0;

    virtual void beginCtr(const char16_t* descr, const UUID& name, const UUID& root)= 0;
    virtual void endCtr()                                                       = 0;

    virtual void beginRoot(int32_t idx, const void* page)                       = 0;
    virtual void endRoot()                                                      = 0;

    virtual void beginNode(int32_t idx, const void* page)                       = 0;
    virtual void endNode()                                                      = 0;

    virtual void rootLeaf(int32_t idx, const void* page)                        = 0;
    virtual void leaf(int32_t idx, const void* page)                            = 0;

    virtual void singleNode(const char16_t* descr, const void* page)            = 0;

    virtual void beginSection(const char16_t* name)                             = 0;
    virtual void endSection()                                                   = 0;

    virtual void content(const char16_t* name, const char16_t* content)         = 0;

    virtual ~ContainerWalker() {}
};

struct ContainerWalkerBase: ContainerWalker {
    virtual void beginAllocator(const char16_t* type, const char16_t* desc) {}
    virtual void endAllocator() {}

    virtual void beginSnapshot(const char16_t* descr) {}
    virtual void endSnapshot() {}

    virtual void beginSnapshotSet(const char16_t* descr, size_t number) {}
    virtual void endSnapshotSet() {}

    virtual void beginCompositeCtr(const char16_t* descr, const UUID& name) {}
    virtual void endCompositeCtr() {}

    virtual void beginCtr(const char16_t* descr, const UUID& name, const UUID& root) {}
    virtual void endCtr() {}

    virtual void beginRoot(int32_t idx, const void* page) {}
    virtual void endRoot() {}

    virtual void beginNode(int32_t idx, const void* page) {}
    virtual void endNode() {}

    virtual void rootLeaf(int32_t idx, const void* page) {}
    virtual void leaf(int32_t idx, const void* page) {}

    virtual void singleNode(const char16_t* descr, const void* page) {}

    virtual void beginSection(const char16_t* name) {}
    virtual void endSection() {}

    virtual void content(const char16_t* name, const char16_t* content) {}

    virtual ~ContainerWalkerBase() {}
};

struct AllocatorBase;

struct ContainerInterface {

    virtual ~ContainerInterface() noexcept {}

    // uuid, id, page data
    using BlockCallbackFn = std::function<void(const UUID&, const UUID&, const void*)>;

    virtual Vertex describe_page(const UUID& page_id, const UUID& name, const SnpSharedPtr<AllocatorBase>& allocator) = 0;
    virtual Collection<Edge> describe_page_links(const UUID& page_id, const UUID& name, const SnpSharedPtr<AllocatorBase>& allocator, Direction direction) = 0;
    virtual Collection<VertexProperty> page_properties(const Vertex& vx, const UUID& page_id, const UUID& name, const SnpSharedPtr<AllocatorBase>& allocator) = 0;


    // FIXME: remove name from parameters, it's already in Ctr's page root metadata

    virtual U16String ctr_name() = 0;

    virtual bool check(
        const UUID& root_id, 
        const UUID& name, 
        const SnpSharedPtr<AllocatorBase>& allocator
    ) const                                                                     = 0;
    
    virtual void walk(
            const UUID& root_id,
            const UUID& name,
            const SnpSharedPtr<AllocatorBase>& allocator,
            ContainerWalker* walker
    ) const                                                                     = 0;

    virtual void walk(
            const UUID& name,
            const SnpSharedPtr<AllocatorBase>& allocator,
            ContainerWalker* walker
    ) const                                                                     = 0;


    virtual U16String ctr_type_name() const                                        = 0;

    virtual void drop(
            const UUID& root_id,
            const UUID& name,
            const SnpSharedPtr<AllocatorBase>& allocator
    )                                                                           = 0;

    virtual void for_each_ctr_node(
        const UUID& name, 
        const SnpSharedPtr<AllocatorBase>& allocator,
        BlockCallbackFn consumer
    )                                                                           = 0;
    
    virtual CtrSharedPtr<CtrReferenceable> new_ctr_instance(
        const UUID& root_id, 
        const UUID& name, 
        const SnpSharedPtr<AllocatorBase>& allocator
    ) = 0;
};

using ContainerInterfacePtr = std::shared_ptr<ContainerInterface>;


template <typename Profile> class MetadataRepository;


struct ContainerMetadata: public MetadataGroup {
public:

    template <typename Types>
    ContainerMetadata(U16StringRef name, Types* nothing, uint64_t ctr_hash, ContainerInterfacePtr container_interface):
        MetadataGroup(name, buildPageMetadata<Types>()),
        container_interface_(container_interface),
        ctr_hash_(ctr_hash)
        
    {
    	MetadataGroup::set_type() = MetadataGroup::CONTAINER;
        for (uint32_t c = 0; c < content_.size(); c++)
        {
            if (content_[c]->getTypeCode() == Metadata::PAGE)
            {
                PageMetadataPtr page = static_pointer_cast<PageMetadata> (content_[c]);
                page_map_[page->hash() ^ ctr_hash] = page;
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

    virtual const PageMetadataPtr& getPageMetadata(uint64_t model_hash, uint64_t page_hash) const
    {
        PageMetadataMap::const_iterator i = page_map_.find(model_hash ^ page_hash);
        if (i != page_map_.end())
        {
            return i->second;
        }
        else {
            MMA1_THROW(Exception()) << WhatCInfo("Unknown page type hash code");
        }
    }

    virtual const ContainerInterfacePtr& getCtrInterface() const
    {
        return container_interface_;
    }
    
    template <typename Profile>
    struct RegistrationHelper {
        RegistrationHelper(const ContainerMetadataPtr& ptr) {
            MetadataRepository<Profile>::registerMetadata(ptr);
        }
    };

private:
    
    template <typename Types>
    static auto buildPageMetadata() 
    {
        MetadataList list;
        Types::Pages::NodeDispatcher::buildMetadataList(list);
        return list;
    }

    PageMetadataMap         page_map_;
    ContainerInterfacePtr   container_interface_;

    uint64_t                ctr_hash_;
};






struct ContainerMetadataRepository: public MetadataGroup {

public:

    ContainerMetadataRepository(U16StringRef name, const MetadataList &content);

    virtual ~ContainerMetadataRepository() throw ()
    {
    }

    virtual uint64_t hash() const {
        return hash_;
    }

    const PageMetadataPtr& getPageMetadata(uint64_t ctr_hash, uint64_t page_hash) const;
    const ContainerMetadataPtr& getContainerMetadata(uint64_t ctr_hash) const;

    virtual void registerMetadata(const ContainerMetadataPtr& metadata)
    {
    	process_model(metadata);
    }

    virtual void unregisterMetadata(const ContainerMetadataPtr& metadata) {}

    void dumpMetadata(std::ostream& out);

private:
    uint64_t                hash_;
    PageMetadataMap         page_map_;
    ContainerMetadataMap    model_map_;
    
    std::mutex mutex_;

    void process_model(const ContainerMetadataPtr& model);
};



template <typename Profile>
class MetadataRepository {

public:

    static ContainerMetadataRepository* getMetadata()
    {
    	static thread_local ContainerMetadataRepository metadata(TypeNameFactory<Profile>::name(), MetadataList());
        return &metadata;
    }

    static void registerMetadata(const ContainerMetadataPtr& ctr_metadata)
    {
        getMetadata()->registerMetadata(ctr_metadata);
    }

    static void unregisterMetadata(const ContainerMetadataPtr& ctr_metadata)
    {
        getMetadata()->unregisterMetadata(ctr_metadata);
    }
};


}}
