
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

const Int SIZE 				= 10;
const Int ArrayName 		= 1;
const Int MAX_BUFFER_SIZE 	= 1024;


typedef StreamContainerTypesCollection::Factory<BlobMap>::Type 	BlobMap0;
typedef BlobMap0::Iterator										BMIterator;
typedef DefaultStreamAllocator 									SAllocator;


void Dump(SAllocator& allocator, const char* name = "array.dump")
{
	FileOutputStreamHandler* out = FileOutputStreamHandler::create(name);
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

ArrayData CreateBuffer(Int size, UByte value)
{
	char* buf = (char*)malloc(size);

	for (Int c = 0;c < size; c++)
	{
		buf[c] = value;
	}

	return ArrayData(size, buf, true);
}



Int GetNonZeroRandom(Int size)
{
	Int value = get_random(size);
	return value != 0 ? value : GetNonZeroRandom(size);
}

ArrayData CreateRandomBuffer(UByte fill_value)
{
	return CreateBuffer(GetNonZeroRandom(MAX_BUFFER_SIZE), fill_value);
}

//BigInt GetRandomPosition(BlobMap0& array)
//{
//	return get_random(array.Size());
//}

void CheckAllocator(SAllocator &allocator, const char* err_msg)
{
	Int src_level = BlobMap0::class_logger().level();
	BlobMap0::class_logger().level() = Logger::TRACE;

	memoria::StreamContainersChecker checker(allocator);

	if (checker.CheckAll())
	{
		throw MemoriaException(MEMORIA_SOURCE, err_msg);
	}

	BlobMap0::class_logger().level() = src_level;
}

bool CompareBuffer(BMIterator& iter, ArrayData& data)
{
	ArrayData buf(data.size());

	iter.Read(buf, 0, buf.size());

	const UByte* buf0 = buf.data();
	const UByte* buf1 = data.data();

	for (Int c = 0; c < data.size(); c++)
	{
		if (buf0[c] != buf1[c])
		{
			return false;
		}
	}

	return true;
}

void CheckBufferWritten(BMIterator& iter, ArrayData& data, const char* err_msg)
{
	if (!CompareBuffer(iter, data))
	{
		throw MemoriaException(MEMORIA_SOURCE, err_msg);
	}
}

void Build(SAllocator& allocator, BlobMap0& array, UByte value)
{
	BMIterator i = array.Create();
	ArrayData data = CreateRandomBuffer(value);
	cerr<<"Size "<<data.size()<<endl;
	i.Insert(data);
	CheckAllocator(allocator, "Insert new LOB");
}

bool Remove(SAllocator& allocator, BlobMap0& array)
{
	return false;
}


int main(int argc, const char** argv, const char **envp) {

	long long t0 = getTime(), t1 = t0;

	try {
		logger.level() = Logger::NONE;

		StreamContainerTypesCollection::Init();

		SAllocator allocator;
		allocator.commit();

		allocator.GetLogger()->level() = Logger::NONE;

		BlobMap0 dv(allocator, ArrayName, true);
//		dv.SetMaxChildrenPerNode(100);

		try {
			cout<<"Insert data"<<endl;

			for (Int c = 0; c < SIZE; c++)
			{
				Build(allocator, dv, c + 1);
			}

			t1 = getTime();

			cout<<"Remove data. ByteArray contains "<<dv.array().Size()<<" Mbytes"<<endl;

			allocator.commit();

//			allocator.DumpPages();

//			for (Int c = 0; ; c++)
//			{
//				if (!Remove(allocator, dv))
//				{
//					break;
//				}
//			}

			cout<<"Remove data. ByteArray contains "<<dv.array().Size()<<" Mbytes"<<endl;

//			allocator.commit();

			Dump(allocator);

			allocator.DumpPages();
		}
		catch (MemoriaException ex)
		{
			Dump(allocator);
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

	cout<<"ARRAY TEST time: remove "<<(getTime()- t1)<<" insert "<<(t1 - t0)<<endl;

//	Int CtrTotal = 0, DtrTotal = 0;
//	for (Int c = 0; c < (Int)(sizeof(PageCtrCnt)/sizeof(Int)); c++)
//	{
//		cout<<c<<" "<<PageCtrCnt[c]<<" "<<PageDtrCnt[c]<<" "<<(PageCtrCnt[c] + PageDtrCnt[c])<<endl;
//		CtrTotal += PageCtrCnt[c];
//		DtrTotal += PageDtrCnt[c];
//	}
//
//	cout<<"Total: "<<CtrTotal<<" "<<DtrTotal<<" "<<(CtrTotal + DtrTotal)<<endl;
//	cout<<"Total: "<<PageCtr<<" "<<PageDtr<<" "<<(PageCtr + PageDtr)<<endl;
}
