
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
const Int ArrayName = 1;
const Int MAX_BUFFER_SIZE = 4096*10;


typedef StreamContainerTypesCollection::Factory<Array>::Type 	ByteArray;
typedef ByteArray::Iterator										BAIterator;
typedef DefaultStreamAllocator 									SAllocator;


void Dump(SAllocator& allocator)
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

ArrayData CreateBuffer(Int size)
{
	char* buf = (char*)malloc(size);
	UByte cnt = 0;

	for (Int c = 0;c < size; c++)
	{
		buf[c] = cnt++;
	}

	return ArrayData(size, buf);
}

Int GetNonZeroRandom(Int size)
{
	Int value = get_random(size);
	return value != 0 ? value : GetNonZeroRandom(size);
}

ArrayData CreateRandomBuffer()
{
	return CreateBuffer(GetNonZeroRandom(MAX_BUFFER_SIZE));
}




BigInt GetRandomPosition(ByteArray& array)
{
	return get_random(array.Size());
}

void DeleteBuffer(ArrayData& data)
{
	::free(data.data());
}

bool CheckAllocator(SAllocator &allocator)
{
	memoria::StreamContainersChecker checker(allocator);
	return checker.CheckAll();
}

bool CompareBuffer(BAIterator& iter, ArrayData& data)
{
	for (Int c = 0; c < data.size(); c++)
	{

	}

	return true;
}

void Build(SAllocator& allocator, ByteArray& array)
{
	if (array.Size() == 0)
	{
		//Insert buffer into an empty array
		auto iter = array.Seek(0);
		ArrayData data = CreateRandomBuffer();
		iter.Insert(data, 0, data.size());

		auto iter1 = array.Seek(0);

		CompareBuffer(iter1, data);

		if (CheckAllocator(allocator))
		{
			cout<<"Insertion into an empty array failed. See the dump for details."<<endl;
			Dump(allocator);
		}
	}
	else {
		SAllocator copy(allocator);
		ByteArray copyArray(copy, ArrayName);

		int op = get_random(3);

		if (op == 0)
		{
			//Insert at the start of the array
			auto iter = array.Seek(0);

		}
	}
}

int main(int argc, const char** argv, const char **envp) {

	long long t0 = getTime();

	try {
		logger.level() = Logger::NONE;

		ContainerTypesCollection<StreamProfile<> >::Init();
		StreamContainerTypesCollection::Init();

		SAllocator allocator;
		allocator.GetLogger()->level() = Logger::NONE;

		ByteArray* dv = new ByteArray(allocator, 1, true);
		typedef ByteArray::Iterator Iterator;

		dv->SetMaxChildrenPerNode(3);

		Int size = 4096 * 10;

		char* buf = (char*)malloc(size);
		Fill(buf, size, 0xBB);

		auto iter = dv->Seek(0);
		iter.DumpState("empty array");

		ArrayData data(size, buf);
		iter.Insert(data, 0, 4060*4);
		iter.DumpState("After the first insert");

		Fill(buf, size, 0xAA);

		allocator.GetLogger()->level() = Logger::ERROR;
		iter = dv->Seek(4060 + 96);

		iter.DumpState("InsertInto");
		iter.Insert(data, 0, 4060*9 + 100);
		iter.DumpState("AfterInsert");

		allocator.GetLogger()->level() = Logger::TRACE;
		Dump(allocator);

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
