
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/v1/tools/params.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>

namespace memoria {
namespace v1 {



void ParametersSet::Process(Configurator* cfg)
{
    for (AbstractParamDescriptor* d: descriptors_)
    {
        d->Process(cfg);
    }
}

void ParametersSet::dumpProperties(std::ostream& os, bool dump_prefix, bool dump_all) const
{
    for (AbstractParamDescriptor* d: descriptors_)
    {
        if (dump_all || !d->isStateParameter())
        {
            d->dump(os, dump_prefix);
        }
    }
}


AbstractParamDescriptor* ParametersSet::put(AbstractParamDescriptor* descr)
{
    for (UInt c = 0; c < descriptors_.size(); c++)
    {
        if (descriptors_[c]->getName() == descr->getName())
        {
            delete descriptors_[c];
            descriptors_[c] = descr;
            return descr;
        }
    }

    descriptors_.push_back(descr);

    return descr;
}



}}