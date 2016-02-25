
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_GROUP_HPP
#define _MEMORIA_VAPI_METADATA_GROUP_HPP

#include <memoria/metadata/metadata.hpp>

namespace memoria    {

struct MEMORIA_API MetadataGroup: public Metadata {
public:

    MetadataGroup(StringRef name, const MetadataList &content, Int block_size = 0, Int type = Metadata::GROUP):
        Metadata(name, type), content_(content.size()), block_size_(block_size)
    {
        for (UInt c = 0; c < content.size(); c++) {
            content_[c] = content[c];
        }
    }

    MetadataGroup(StringRef name, Int block_size = 0, Int type = Metadata::GROUP):
        Metadata(name, type), block_size_(block_size)
    {}

    virtual ~MetadataGroup() throw ()
    {
        for (UInt c = 0; c < content_.size(); c++)
        {
            delete content_[c];
        }
    }


    virtual Int size() const {
        return content_.size();
    }

    virtual Metadata* getItem(Int idx) const {
        return content_[idx];
    }



    virtual void putAll(MetadataList& target) const
    {
        for (auto i = content_.begin(); i != content_.end(); i++)
        {
            target.push_back(*i);
        }
    }

    virtual Int getBlockSize() const
    {
        return block_size_;
    }

protected:
    MetadataList    content_;
    Int             block_size_;

};



inline bool isGroup(Metadata *meta)
{
    Int type = meta->getTypeCode();
    return type == Metadata::GROUP || type == Metadata::CONTAINER || type == Metadata::PAGE;
}



}

#endif
