
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/allocators/stream/factory.hpp>
#include <memoria/core/tools/bm_tools.hpp>

#include <vector>

#include <stdlib.h>

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




void Read(ByteArray& array, ArrayData& data)
{
	BigInt size = array.size();

	Int data_len = data.size();
	BigInt pos = size != 0 ? (random() % (size / data_len)) * data_len : 0;

//	cout<<"pos="<<pos<<endl;

	//array.insert(array.begin() + pos, data.size(), 0);

	for (int c = 0; c < data.size(); c++)
	{
		*(data.data() + c) = array[pos + c];
	}

}

int main(int argc, const char** argv, const char **envp) {

	Int SIZE = 300000000;

	long long t0;

	try {
		ByteArray dv;

		for (int c = 0; c < SIZE; c++) dv.push_back(0);

		cout<<"Size: "<<dv.size()<<" bytes"<<endl;


		t0 =  getTime();
		ArrayData data(4);

		try {
			for (Int c = 0; c < 1000000; c++)
			{
				Read(dv, data);
			}


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
