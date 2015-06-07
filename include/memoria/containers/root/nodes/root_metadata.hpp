
// Copyright Victor Smirnov 2012-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_ROOT_NODES_ROOTMETADATA_HPP
#define _MEMORIA_CONTAINERS_ROOT_NODES_ROOTMETADATA_HPP


#include <memoria/prototypes/bt/nodes/tree_metadata.hpp>

namespace memoria    {


template <typename ID, Int Streams>
class RootCtrMetadata: public BalancedTreeMetadata<ID, Streams>
{
    static const UInt VERSION = 1;

    typedef BalancedTreeMetadata<ID, Streams> Base;

    BigInt model_name_counter_;

public:

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
                decltype(model_name_counter_)
    >;

    RootCtrMetadata() = default;

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

template <typename ID, Int Streams>
struct TypeHash<RootCtrMetadata<ID, Streams>>: UIntValue<
    HashHelper<2600, TypeHash<ID>::Value, Streams>::Value
> {};


}

#endif
