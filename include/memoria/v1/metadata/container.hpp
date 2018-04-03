
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





template <typename PageType>
class FSDumpContainerWalker: public ContainerWalker {

    using Page = PageType;
    using ID   = typename Page::ID;

    ContainerMetadataRepository* metadata_;
    std::stack<bf::path> path_;

public:
    FSDumpContainerWalker(ContainerMetadataRepository* metadata, U16StringRef root_u16):
        metadata_(metadata)
    {
        U8String root = root_u16.to_u8();

        if (!bf::exists(root.to_std_string()))
        {
            bf::create_directories(root.to_std_string());
        }
        else {
            bf::remove_all(root.to_std_string());
            bf::create_directories(root.to_std_string());
        }

        path_.push(bf::path(root.to_std_string()));
    }

    virtual void beginSnapshotSet(const char16_t* descr, size_t number)
    {
        pushFolder(descr);
    }

    virtual void endSnapshotSet()
    {
        path_.pop();
    }

    virtual void beginAllocator(const char16_t* type, const char16_t* desc)
    {
        pushFolder(type);
    }

    virtual void endAllocator()
    {
        path_.pop();
    }

    virtual void beginSnapshot(const char16_t* descr)
    {
        pushFolder(descr);
    }

    virtual void endSnapshot()
    {
        path_.pop();
    }

    virtual void beginCompositeCtr(const char16_t* descr, const UUID& name)
    {
        stringstream str;

        str << shorten(descr) <<": " << name;

        pushFolder(U8String(str.str()).to_u16().data());

        dumpDescription(u"ctr_name", descr);
    }

    virtual void endCompositeCtr() {
        path_.pop();
    }

    virtual void beginCtr(const char16_t* descr, const UUID& name, const UUID& root)
    {
        stringstream str;

        str<<shorten(descr)<<": "<<name;

        pushFolder(U8String(str.str()).to_u16().data());

        dumpDescription(u"ctr_name", descr);
    }

    virtual void endCtr() {
        path_.pop();
    }

    virtual void rootLeaf(int32_t idx, const void* page_data)
    {
        const Page* page = T2T<Page*>(page_data);

        U8String file_name = U8String(path_.top().string()) + Platform::getFilePathSeparator().to_u8() + "root_leaf.txt";

        dumpPage(file_name.to_u16(), page);
    }

    virtual void leaf(int32_t idx, const void* page_data)
    {
        const Page* page = T2T<Page*>(page_data);

        U16String description = getNodeName(u"Leaf", idx, page->id());

        U16String file_name = U8String(path_.top().string()).to_u16() + Platform::getFilePathSeparator() + description + u".txt";

        dumpPage(file_name, page);
    }

    virtual void beginRoot(int32_t idx, const void* page_data)
    {
        beginNonLeaf(u"Root", idx, page_data);
    }

    virtual void endRoot()
    {
        path_.pop();
    }

    virtual void beginNode(int32_t idx, const void* page_data)
    {
        beginNonLeaf(u"Node", idx, page_data);
    }

    virtual void endNode()
    {
        path_.pop();
    }

    virtual void singleNode(const char16_t* description, const void* page_data)
    {
        const Page* page = T2T<Page*>(page_data);

        U16String file_name = U8String(path_.top().string()).to_u16() + Platform::getFilePathSeparator() + description + u".txt";

        dumpPage(file_name, page);
    }


    virtual void beginSection(const char16_t* name)
    {
        pushFolder(name);
    }

    virtual void endSection() {
        path_.pop();
    }

    virtual void content(const char16_t* name, const char16_t* content)
    {
        dumpDescription(name, content);
    }

private:

    void beginNonLeaf(const char16_t* type, int32_t idx, const void* page_data)
    {
        const Page* page = T2T<Page*>(page_data);

        U16String folder_name = getNodeName(type, idx, page->id());
        pushFolder(folder_name.data());

        U16String file_name = U8String(path_.top().string()).to_u16() + Platform::getFilePathSeparator() + u"0_page.txt";

        dumpPage(file_name, page);
    }


    void dumpPage(U16StringRef file, const Page* page)
    {
        std::ofstream pagetxt(file.to_u8().data());

        auto meta = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

        dumpPageData(meta.get(), page, pagetxt);
    }

    void dumpDescription(U16StringRef type, U16StringRef content)
    {
        U8String file_name = U8String(path_.top().parent_path().string()) + Platform::getFilePathSeparator().to_u8() + type.to_u8() + ".txt";

        std::ofstream file(file_name.data());

        file << content;
    }

    void pushFolder(const char16_t* descr)
    {
        U8String name = U8String(path_.top().string()) + Platform::getFilePathSeparator().to_u8() + U16String(descr).to_u8();
        bf::path file(name.to_std_string());
        
        auto res = bf::create_directory(name.to_std_string());
        
        MEMORIA_V1_ASSERT_TRUE(res);
        path_.push(file);
    }

    U16String getNodeName(const char16_t* name, int32_t index, const ID& id)
    {
        std::stringstream str;

        str << U16String(name) << "-";

        char prev = str.fill();

        str.fill('0');
        str.width(4);

        str << index;

        str.fill(prev);

        str << "___" << id;

        return U8String(str.str()).to_u16();
    }

private:
    U16String shorten(const char16_t* txt)
    {
        U16String text = U16String(txt);

        auto start = text.find_first_of("<");

        if (start != U16String::NPOS)
        {
            text.to_std_string().erase(start);
        }

        return text;
    }
};




template <typename Allocator>
void FSDumpAllocator(Allocator& allocator, U16StringRef path)
{
    using Walker = FSDumpContainerWalker<typename Allocator::Page>;

    Walker walker(allocator.metadata(), path);
    allocator.walk_containers(&walker);
}



template <typename PageType>
class FiberFSDumpContainerWalker: public ContainerWalker {

    using Page = PageType;
    using ID   = typename Page::ID;

    ContainerMetadataRepository* metadata_;
    std::stack<filesystem::path> path_;

public:
    FiberFSDumpContainerWalker(ContainerMetadataRepository* metadata, filesystem::path root):
        metadata_(metadata)
    {
        filesystem::path root_path = std::move(root);
        
        if (!filesystem::exists(root_path))
        {
            filesystem::create_directories(root_path);
        }
        else {
            filesystem::remove_all(root_path);
            filesystem::create_directories(root_path);
        }

        path_.push(root_path);
    }

    virtual void beginSnapshotSet(const char16_t* descr, size_t number)
    {
        pushFolder(descr);
    }

    virtual void endSnapshotSet()
    {
        path_.pop();
    }

    virtual void beginAllocator(const char16_t* type, const char16_t* desc)
    {
        pushFolder(type);
    }

    virtual void endAllocator()
    {
        path_.pop();
    }

    virtual void beginSnapshot(const char16_t* descr)
    {
        pushFolder(descr);
    }

    virtual void endSnapshot()
    {
        path_.pop();
    }

    virtual void beginCompositeCtr(const char16_t* descr, const UUID& name)
    {
        stringstream str;

        str << shorten(descr) << ": " << name;

        pushFolder(U8String(str.str()).to_u16().data());

        dumpDescription(u"ctr_name", U16String(descr).to_u8());
    }

    virtual void endCompositeCtr() {
        path_.pop();
    }

    virtual void beginCtr(const char16_t* descr, const UUID& name, const UUID& root)
    {
        stringstream str;

        str << shorten(descr) << ": " << name;

        pushFolder(U8String(str.str()).to_u16().data());

        dumpDescription(u"ctr_name", U16String(descr).to_u8());
    }

    virtual void endCtr() {
        path_.pop();
    }

    virtual void rootLeaf(int32_t idx, const void* page_data)
    {
        const Page* page = T2T<Page*>(page_data);

        U16String file_name = path_.top().to_u16() + Platform::getFilePathSeparator() + u"root_leaf.txt";

        dumpPage(file_name, page);
    }

    virtual void leaf(int32_t idx, const void* page_data)
    {
        const Page* page = T2T<Page*>(page_data);

        U16String description = getNodeName(u"Leaf", idx, page->id());

        U16String file_name = path_.top().to_u16() + Platform::getFilePathSeparator() + description + u".txt";

        dumpPage(file_name, page);
    }

    virtual void beginRoot(int32_t idx, const void* page_data)
    {
        beginNonLeaf(u"Root", idx, page_data);
    }

    virtual void endRoot()
    {
        path_.pop();
    }

    virtual void beginNode(int32_t idx, const void* page_data)
    {
        beginNonLeaf(u"Node", idx, page_data);
    }

    virtual void endNode()
    {
        path_.pop();
    }

    virtual void singleNode(const char16_t* description, const void* page_data)
    {
        const Page* page = T2T<Page*>(page_data);

        U16String file_name = path_.top().to_u16() + Platform::getFilePathSeparator() + description + u".txt";

        dumpPage(file_name, page);
    }


    virtual void beginSection(const char16_t* name)
    {
        pushFolder(name);
    }

    virtual void endSection() {
        path_.pop();
    }

    virtual void content(const char16_t* name, const char16_t* content)
    {
        dumpDescription(name, U16String(content).to_u8());
    }

private:

    void beginNonLeaf(const char16_t* type, int32_t idx, const void* page_data)
    {
        const Page* page = T2T<Page*>(page_data);

        U16String folder_name = getNodeName(type, idx, page->id());
        pushFolder(folder_name.data());

        U16String file_name = path_.top().to_u16() + Platform::getFilePathSeparator() + u"0_page.txt";

        dumpPage(file_name, page);
    }


    void dumpPage(filesystem::path file, const Page* page)
    {
        reactor::bfstream pagetxt(
            reactor::open_buffered_file(
                file, 
                reactor::FileFlags::WRONLY | reactor::FileFlags::CREATE | reactor::FileFlags::TRUNCATE
            )
        );

        auto meta = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

        dumpPageData(meta.get(), page, pagetxt);
    }

    void dumpDescription(U16StringRef type, U8StringRef content)
    {
        U16String file_name = path_.top().parent_path().to_u16() + Platform::getFilePathSeparator() + type + u".txt";

        reactor::bfstream file(
            reactor::open_buffered_file(
                file_name,
                reactor::FileFlags::WRONLY | reactor::FileFlags::CREATE | reactor::FileFlags::TRUNCATE
            )
        );

        file << content;
    }

    void pushFolder(const char16_t* descr)
    {
        U16String name = path_.top().to_u16() + Platform::getFilePathSeparator() + descr;
        filesystem::path file(name);
        
        auto res = filesystem::create_directory(name);
        
        MEMORIA_V1_ASSERT_TRUE(res);
        path_.push(file);
    }

    U16String getNodeName(const char16_t* name, int32_t index, const ID& id)
    {
        std::stringstream str;

        str << U16String(name) << "-";

        char prev = str.fill();

        str.fill('0');
        str.width(4);

        str << index;

        str.fill(prev);

        str << "___" << id;

        return U8String(str.str()).to_u16();
    }

private:
    U16String shorten(const char16_t* txt)
    {
        U16String text = txt;

        auto start = text.find_first_of("<");

        if (start != U16String::NPOS)
        {
            text.to_std_string().erase(start);
        }

        return text;
    }
};


template <typename Allocator>
void FiberFSDumpAllocator(Allocator& allocator, U16StringRef path)
{
    using Walker = FiberFSDumpContainerWalker<typename Allocator::Page>;

    Walker walker(allocator.metadata(), path);
    allocator.walk_containers(&walker);
}




}}
