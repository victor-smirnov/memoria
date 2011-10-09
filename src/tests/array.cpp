
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include "mtools.hpp"

#include <memoria/allocators/stream/factory.hpp>
#include <memoria/core/tools/bm_tools.hpp>

using namespace memoria;
using namespace memoria::vapi;


using namespace std;

const Int SIZE = 1000;

typedef StreamContainerTypesCollection::Factory<Array>::Type ByteArray;


void Dump(DefaultStreamAllocator& allocator)
{
	FileOutputStreamHandler* out = FileOutputStreamHandler::create("array.dump");
	allocator.store(out);
	delete out;
}


void Fill(char* buf, int size, char value)
{
	for (int c 	= 0; c < size; c++)
	{
		buf[c] = value;
	}
}


int main(int argc, const char** argv, const char **envp) {

	long long t0 = getTime();

	try {
		InitTypeSystem(argc, argv, envp, false);

		logger.level() = Logger::NONE;

		ContainerTypesCollection<StreamProfile<> >::Init();
		StreamContainerTypesCollection::Init();

		DefaultStreamAllocator allocator;
		allocator.GetLogger()->level() = Logger::NONE;

		ByteArray* dv = new ByteArray(allocator, 1, true);
		typedef ByteArray::Iterator Iterator;

		dv->SetMaxChildrenPerNode(3);

		Int size = 4096 * 10;

		char* buf = (char*)malloc(size);
		Fill(buf, size, 0xBB);

		Iterator iter = dv->Seek(0);
		iter.DumpState("empty array");

		ArrayData data(size, buf);
		iter.Insert(data, 0, 4060*4);
		iter.DumpState("After the first insert");

		Fill(buf, size, 0xAA);

		allocator.GetLogger()->level() = Logger::TRACE;
		iter = dv->Seek(4060 + 96);

		iter.DumpState("InsertInto");
		iter.Insert(data, 0, 4060*9 + 100);
		iter.DumpState("AfterInsert");

		allocator.GetLogger()->level() = Logger::ERROR;
		//Dump(allocator);

		free(buf);

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
