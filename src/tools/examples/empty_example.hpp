// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_EXAMPLES_EXPTY_EXAMPLE_HPP_
#define MEMORIA_EXAMPLES_EXPTY_EXAMPLE_HPP_

#include "examples.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {




class EmptyExample: public ExampleTask {

public:

	Int param;

public:


	EmptyExample() :
		ExampleTask("EmptyExample"), param(-1)
	{
		Add("param", param)->SetMandatory(true);
	}

	virtual ~EmptyExample() throw ()
	{
	}


	virtual void Run(ostream& out)
	{
		out<<"Ok! "<<param<<endl;
	}
};

}

#endif
