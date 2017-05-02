
// Copyright 2012 Victor Smirnov
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


#include <memoria/v1/tests/params.hpp>
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
    for (uint32_t c = 0; c < descriptors_.size(); c++)
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
