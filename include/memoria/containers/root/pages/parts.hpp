
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_ROOT_PAGES_PARTS_HPP
#define _MEMORIA_CONTAINERS_ROOT_PAGES_PARTS_HPP


#include <memoria/prototypes/btree/pages/parts.hpp>

namespace memoria    {

using namespace memoria::btree;



template <typename ID>
class RootCtrMetadata: public BTreeMetadata<ID>
{
    typedef BTreeMetadata<ID> Base;

    BigInt model_name_counter_;

public:
    RootCtrMetadata() {}

    BigInt &model_name_counter() {
        return model_name_counter_;
    }

    const BigInt &model_name_counter() const {
        return model_name_counter_;
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        handler->value("MODEL_NAME_COUNTER",        &model_name_counter_);
    }

    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        FieldFactory<BigInt>::serialize(buf, model_name_counter_);
    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        FieldFactory<BigInt>::deserialize(buf, model_name_counter_);
    }
};


}

#endif
