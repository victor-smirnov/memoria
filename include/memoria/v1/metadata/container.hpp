
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
    virtual void beginAllocator(const char* type, const char* desc)             = 0;
    virtual void endAllocator()                                                 = 0;

    virtual void beginSnapshot(const char* descr)                               = 0;
    virtual void endSnapshot()                                                  = 0;

    virtual void beginSnapshotSet(const char* descr, size_t number)             = 0;
    virtual void endSnapshotSet()                                               = 0;

    virtual void beginCompositeCtr(const char* descr, const UUID& name)         = 0;
    virtual void endCompositeCtr()                                              = 0;

    virtual void beginCtr(const char* descr, const UUID& name, const UUID& root)= 0;
    virtual void endCtr()                                                       = 0;

    virtual void beginRoot(Int idx, const void* page)                           = 0;
    virtual void endRoot()                                                      = 0;

    virtual void beginNode(Int idx, const void* page)                           = 0;
    virtual void endNode()                                                      = 0;

    virtual void rootLeaf(Int idx, const void* page)                            = 0;
    virtual void leaf(Int idx, const void* page)                                = 0;

    virtual void singleNode(const char* descr, const void* page)                = 0;

    virtual void beginSection(const char* name)                                 = 0;
    virtual void endSection()                                                   = 0;

    virtual void content(const char* name, const char* content)                 = 0;

    virtual ~ContainerWalker() {}
};

struct ContainerWalkerBase: ContainerWalker {
    virtual void beginAllocator(const char* type, const char* desc) {}
    virtual void endAllocator() {}

    virtual void beginSnapshot(const char* descr) {}
    virtual void endSnapshot() {}

    virtual void beginSnapshotSet(const char* descr, size_t number) {}
    virtual void endSnapshotSet() {}

    virtual void beginCompositeCtr(const char* descr, const UUID& name) {}
    virtual void endCompositeCtr() {}

    virtual void beginCtr(const char* descr, const UUID& name, const UUID& root) {}
    virtual void endCtr() {}

    virtual void beginRoot(Int idx, const void* page) {}
    virtual void endRoot() {}

    virtual void beginNode(Int idx, const void* page) {}
    virtual void endNode() {}

    virtual void rootLeaf(Int idx, const void* page) {}
    virtual void leaf(Int idx, const void* page) {}

    virtual void singleNode(const char* descr, const void* page) {}

    virtual void beginSection(const char* name) {}
    virtual void endSection() {}

    virtual void content(const char* name, const char* content) {}

    virtual ~ContainerWalkerBase() {}
};


struct ContainerInterface {

	// uuid, id, page data
	using BlockCallbackFn = std::function<void(const UUID&, const UUID&, const void*)>;

    // FIXME: remove name from parameters, it's already in Ctr's page root metadata

    virtual String ctr_name() = 0;

    virtual bool check(const UUID& root_id, const UUID& name, void* allocator) const = 0;
    virtual void walk(
            const UUID& root_id,
            const UUID& name,
            void* allocator,
            ContainerWalker* walker
    ) const                                                                     = 0;

    virtual void walk(
            const UUID& name,
            void* allocator,
            ContainerWalker* walker
    ) const                                                                     = 0;


    virtual String ctr_type_name() const                                        = 0;

    virtual void drop(
            const UUID& root_id,
            const UUID& name,
            void* allocator
    )                                                                           = 0;

    virtual void for_each_ctr_node(const UUID& name, void* allocator, BlockCallbackFn consumer) = 0;

    virtual ~ContainerInterface() {}
};

using ContainerInterfacePtr = std::shared_ptr<ContainerInterface>;


template <typename Profile> class MetadataRepository;


struct ContainerMetadata: public MetadataGroup {
public:

    ContainerMetadata(StringRef name, const MetadataList &content, Int ctr_hash, ContainerInterfacePtr container_interface):
        MetadataGroup(name, content),
        container_interface_(container_interface),
        ctr_hash_(ctr_hash)
    {
    	MetadataGroup::set_type() = MetadataGroup::CONTAINER;
        for (UInt c = 0; c < content.size(); c++)
        {
            if (content[c]->getTypeCode() == Metadata::PAGE)
            {
                PageMetadataPtr page = static_pointer_cast<PageMetadata> (content[c]);
                page_map_[page->hash() ^ ctr_hash] = page;
            }
            else if (content[c]->getTypeCode() == Metadata::CONTAINER) {
                // nothing to do
            }
            else {
                //exception;
            }
        }
    }

    template <typename Types>
    ContainerMetadata(StringRef name, Types* nothing, Int ctr_hash, ContainerInterfacePtr container_interface):
        MetadataGroup(name, buildPageMetadata<Types>()),
        container_interface_(container_interface),
        ctr_hash_(ctr_hash)
    {
    	MetadataGroup::set_type() = MetadataGroup::CONTAINER;
        for (UInt c = 0; c < content_.size(); c++)
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

    virtual Int ctr_hash() const {
        return ctr_hash_;
    }

    virtual const PageMetadataPtr& getPageMetadata(Int model_hash, Int page_hash) const
    {
        PageMetadataMap::const_iterator i = page_map_.find(model_hash ^ page_hash);
        if (i != page_map_.end())
        {
            return i->second;
        }
        else {
            throw Exception(MEMORIA_SOURCE, "Unknown page type hash code");
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

    Int                     ctr_hash_;
};






struct ContainerMetadataRepository: public MetadataGroup {

public:

    ContainerMetadataRepository(StringRef name, const MetadataList &content);

    virtual ~ContainerMetadataRepository() throw ()
    {
    }

    virtual Int hash() const {
        return hash_;
    }

    const PageMetadataPtr& getPageMetadata(Int model_hash, Int page_hash) const;
    const ContainerMetadataPtr& getContainerMetadata(Int model_hash) const;


    virtual void registerMetadata(const ContainerMetadataPtr& metadata)
    {
    	process_model(metadata);
    }

    virtual void unregisterMetadata(const ContainerMetadataPtr& metadata) {}

    void dumpMetadata(std::ostream& out);

private:
    Int                     hash_;
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

    typedef PageType                                                            Page;
    typedef typename Page::ID                                                   ID;

    ContainerMetadataRepository* metadata_;
    std::stack<bf::path> path_;

public:
    FSDumpContainerWalker(ContainerMetadataRepository* metadata, StringRef root):
        metadata_(metadata)
    {
        if (!bf::exists(root))
        {
            bf::create_directories(root);
        }
        else {
            bf::remove_all(root);
            bf::create_directories(root);
        }

        path_.push(bf::path(root));
    }

    virtual void beginSnapshotSet(const char* descr, size_t number)
    {
        pushFolder(descr);
    }

    virtual void endSnapshotSet()
    {
        path_.pop();
    }

    virtual void beginAllocator(const char* type, const char* desc)
    {
        pushFolder(type);
    }

    virtual void endAllocator()
    {
        path_.pop();
    }

    virtual void beginSnapshot(const char* descr)
    {
        pushFolder(descr);
    }

    virtual void endSnapshot()
    {
        path_.pop();
    }

    virtual void beginCompositeCtr(const char* descr, const UUID& name)
    {
        stringstream str;

        str << shorten(descr) <<": " << name;

        pushFolder(str.str().c_str());

        dumpDescription("ctr_name", String(descr));
    }

    virtual void endCompositeCtr() {
        path_.pop();
    }

    virtual void beginCtr(const char* descr, const UUID& name, const UUID& root)
    {
        stringstream str;

        str<<shorten(descr)<<": "<<name;

        pushFolder(str.str().c_str());

        dumpDescription("ctr_name", String(descr));
    }

    virtual void endCtr() {
        path_.pop();
    }

    virtual void rootLeaf(Int idx, const void* page_data)
    {
        const Page* page = T2T<Page*>(page_data);

        String file_name = path_.top().string() + Platform::getFilePathSeparator() + "root_leaf.txt";

        dumpPage(file_name, page);
    }

    virtual void leaf(Int idx, const void* page_data)
    {
        const Page* page = T2T<Page*>(page_data);

        String description = getNodeName("Leaf", idx, page->id());

        String file_name = path_.top().string() + Platform::getFilePathSeparator() + description + ".txt";

        dumpPage(file_name, page);
    }

    virtual void beginRoot(Int idx, const void* page_data)
    {
        beginNonLeaf("Root", idx, page_data);
    }

    virtual void endRoot()
    {
        path_.pop();
    }

    virtual void beginNode(Int idx, const void* page_data)
    {
        beginNonLeaf("Node", idx, page_data);
    }

    virtual void endNode()
    {
        path_.pop();
    }

    virtual void singleNode(const char* description, const void* page_data)
    {
        const Page* page = T2T<Page*>(page_data);

        String file_name = path_.top().string() + Platform::getFilePathSeparator() + description + ".txt";

        dumpPage(file_name, page);
    }


    virtual void beginSection(const char* name)
    {
        pushFolder(name);
    }

    virtual void endSection() {
        path_.pop();
    }

    virtual void content(const char* name, const char* content)
    {
        dumpDescription(name, content);
    }

private:

    void beginNonLeaf(const char* type, Int idx, const void* page_data)
    {
        const Page* page = T2T<Page*>(page_data);

        String folder_name = getNodeName(type, idx, page->id());
        pushFolder(folder_name.c_str());

        String file_name = path_.top().string() + Platform::getFilePathSeparator() + "0_page.txt";

        dumpPage(file_name, page);
    }


    void dumpPage(StringRef file, const Page* page)
    {
        std::ofstream pagetxt(file.c_str());

        auto meta = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

        dumpPageData(meta.get(), page, pagetxt);
    }

    void dumpDescription(StringRef type, StringRef content)
    {
        String file_name = path_.top().parent_path().string() + Platform::getFilePathSeparator() + type + ".txt";

        std::ofstream file(file_name.c_str());

        file<<content;
    }

    void pushFolder(const char* descr)
    {
        String name = path_.top().string() + Platform::getFilePathSeparator() + String(descr);
        bf::path file(name);
        
        auto res = bf::create_directory(name);
        
        MEMORIA_V1_ASSERT_TRUE(res);
        path_.push(file);
    }

    String getNodeName(const char* name, Int index, const ID& id)
    {
        std::stringstream str;

        str<<name<<"-";

        char prev = str.fill();

        str.fill('0');
        str.width(4);

        str<<index;

        str.fill(prev);

        str<<"___"<<id;

        return str.str();
    }

private:
    String shorten(const char* txt)
    {
        String text = txt;

        auto start = text.find_first_of("<");

        if (start != String::npos)
        {
            text.erase(start);
        }

        return text;
    }
};


template <typename Allocator>
void FSDumpAllocator(Allocator& allocator, StringRef path)
{
    using Walker = FSDumpContainerWalker<typename Allocator::Page>;

    Walker walker(allocator.metadata(), path);
    allocator.walk_containers(&walker);
}




}}
