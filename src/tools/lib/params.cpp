
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/tools/params.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

namespace memoria {



void Parametersset::Process(Configurator* cfg)
{
	for (AbstractParamDescriptor* d: descriptors_)
	{
		d->Process(cfg);
	}
}

void Parametersset::DumpProperties(std::ostream& os) const
{
	for (AbstractParamDescriptor* d: descriptors_)
	{
		d->Dump(os);
	}
}


AbstractParamDescriptor* Parametersset::put(AbstractParamDescriptor* descr)
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



}
