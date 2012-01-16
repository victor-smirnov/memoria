
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/tools/params.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

namespace memoria {



void ParametersSet::Process(Configurator* cfg)
{
	for (AbstractParamDescriptor* d: descriptors_)
	{
		d->Process(cfg);
	}
}

void ParametersSet::DumpProperties(std::ostream& os)
{
	for (AbstractParamDescriptor* d: descriptors_)
	{
		d->Dump(os);
	}
}






}
