
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/metadata/tools.hpp>
#include <memoria/core/tools/array_data.hpp>

namespace memoria {


BigInt DebugCounter = 0;

Int CtrRefCounters = 0;
Int CtrUnrefCounters = 0;

namespace vapi {

LogHandler* Logger::default_handler_ = new DefaultLogHandlerImpl();
Logger logger("Memoria", Logger::INFO, NULL);

void ArrayData::Dump(std::ostream& out) {
	out<<endl;
	Expand(out, 24);
	for (int c = 0; c < 32; c++)
	{
		out.width(3);
		out<<hex<<c;
	}
	out<<endl;

	Int size0 = size();

	for (Int c = 0; c < size0; c+= 32)
	{
		Expand(out, 12);
		out<<" ";
		out.width(4);
		out<<dec<<c<<" "<<hex;
		out.width(4);
		out<<c<<": ";

		for (Int d = 0; d < 32 && c + d < size0; d++)
		{
			UByte data0 = *(this->data() + c + d);
			out<<hex;
			out.width(3);
			out<<(Int)data0;
		}

		out<<dec<<endl;
	}

}


}}


