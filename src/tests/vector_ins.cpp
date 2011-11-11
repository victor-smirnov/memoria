
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include "mtools.hpp"

#include <memoria/allocators/stream/factory.hpp>
#include <memoria/core/tools/bm_tools.hpp>

#include <vector>

using namespace memoria;
using namespace memoria::vapi;



using namespace std;

const Int SIZE = 1000;
const Int ArrayName = 1;
const Int MAX_BUFFER_SIZE = 4096 * 10;


typedef vector<UByte> ByteArray;

void Fill(char* buf, int size, char value)
{
	for (int c 	= 0; c < size; c++)
	{
		buf[c] = value;
	}
}




void Insert(ByteArray& array, ArrayData& data)
{
	BigInt size = array.size();

	Int data_len = data.size();
	BigInt pos = size != 0 ? get_random(size / data_len) * data_len : 0;


	array.insert(array.begin() + pos, data.size(), 0);

	for (int c = 0; c < data.size(); c++)
	{
		array[pos + c] = *(data.data() + c);
	}

}

int main(int argc, const char** argv, const char **envp) {

	long long t0 = getTime();

	try {
		ByteArray dv;

		ArrayData data(4);

		try {
			for (Int c = 0; c < 1000000; c++)
			{
				Insert(dv, data);
			}

			cout<<"Size: "<<dv.size()<<" bytes"<<endl;
		}
		catch (MemoriaException ex)
		{
			throw;
		}
	}
	catch (MemoriaException ex) {
		cout<<"MemoriaException "<<ex.source()<<" "<<ex.message()<<endl;
	}
	catch (MemoriaException *ex) {
		cout<<"MemoriaException* "<<ex->source()<<" "<<ex->message()<<endl;
	}
	catch (int i) {
		cout<<"IntegerEx: "<<i<<endl;
	}
	catch (exception e) {
		cout<<"StdEx: "<<e.what()<<endl;
	}
	catch(...) {
		cout<<"Unrecognized exception"<<endl;
	}

	cout<<"TREE MAP time: "<<(getTime()- t0)<<endl;
}
