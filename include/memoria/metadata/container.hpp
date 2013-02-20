
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_CONTAINER_HPP
#define _MEMORIA_VAPI_METADATA_CONTAINER_HPP

#include <memoria/metadata/group.hpp>
#include <memoria/metadata/page.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

namespace memoria    {
namespace vapi       {

struct MEMORIA_API ContainerMetadataRepository: public MetadataGroup {

    public:

        ContainerMetadataRepository(StringRef name, const MetadataList &content);

        virtual ~ContainerMetadataRepository() throw () {}

        virtual Int hash() const {
            return hash_;
        }

        PageMetadata* getPageMetadata(Int model_hash, Int page_hash) const;
        ContainerMetadata* getContainerMetadata(Int model_hash) const;


        virtual void registerMetadata(ContainerMetadata* metadata)
        {
            process_model(metadata);
        }

        virtual void unregisterMetadata(ContainerMetadata* metadata) {}


    private:
        Int                     hash_;
        PageMetadataMap         page_map_;
        ContainerMetadataMap    model_map_;

        void process_model(ContainerMetadata* model);
};




struct ContainerInterface {
    virtual bool check(const void* id, void* allocator) const   = 0;

    virtual ~ContainerInterface() {}
};

struct MEMORIA_API ContainerMetadata: public MetadataGroup {
public:

    ContainerMetadata(StringRef name, const MetadataList &content, Int ctr_hash, ContainerInterface* container_interface):
        MetadataGroup(name, content),
        container_interface_(container_interface),
        ctr_hash_(ctr_hash)
    {
        MetadataGroup::set_type() = MetadataGroup::CONTAINER;
        for (UInt c = 0; c < content.size(); c++)
        {
            if (content[c]->getTypeCode() == Metadata::PAGE)
            {
                PageMetadata *page = static_cast<PageMetadata*> (content[c]);
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

    virtual ~ContainerMetadata() throw () {}

    virtual Int ctr_hash() const {
        return ctr_hash_;
    }

    virtual PageMetadata* getPageMetadata(Int model_hash, Int page_hash) const
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

    virtual ContainerInterface* getCtrInterface() const
    {
        return container_interface_;
    }

private:

    PageMetadataMap         page_map_;
    ContainerInterface*     container_interface_;

    Int                     ctr_hash_;
};






}}


#endif
