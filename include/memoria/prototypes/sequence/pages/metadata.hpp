
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQDENSE_PAGES_METADATA_HPP
#define _MEMORIA_CONTAINERS_SEQDENSE_PAGES_METADATA_HPP

#include <memoria/prototypes/btree/pages/parts.hpp>

namespace memoria    {


template <typename ID>
class SequenceMetadata: public BTreeMetadata<ID>
{
    static const UInt VERSION = 1;

    typedef BTreeMetadata<ID> Base;

public:

    typedef typename AppendToList<
            typename Base::FieldsList,
            ConstValue<UInt, VERSION>
    >::Result                                                                   FieldsList;

    SequenceMetadata() {}

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);
    }

    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);
    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);
    }
};

}

#endif
