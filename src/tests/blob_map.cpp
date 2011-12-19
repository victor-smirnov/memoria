
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

const Int SIZE 				= 10000;
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
//	Int src_level = BlobMap0::class_logger().level();
//	BlobMap0::class_logger().level() = Logger::TRACE;

//	memoria::StreamContainersChecker checker(allocator);

//	if (checker.CheckAll())
//	{
//		throw MemoriaException(MEMORIA_SOURCE, err_msg);
//	}

//	BlobMap0::class_logger().level() = src_level;
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
//	cerr<<"Size "<<data.size()<<endl;
	i.Insert(data);
	CheckAllocator(allocator, "Insert new LOB");
}

bool Remove(SAllocator& allocator, BlobMap0& array)
{
	Int size 	= array.set().GetSize();

	if (size > 0)
	{
		Int idx = get_random(size);
		array.RemoveByIndex(idx);

		return size > 1;
	}
	else {
		return false;
	}
}


int main(int argc, const char** argv, const char **envp) {

	long long t0 = getTime(), t1 = t0, t2 = t0, t3 = t0, t4 = t0;

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
			t0 = getTime();

			for (Int c = 0; c < SIZE; c++)
			{
				Build(allocator, dv, c + 1);
				if (c % 100 == 0) allocator.commit();
			}
			allocator.commit();

			t1 = getTime();

			cout<<"Read data sequentially. ByteArray contains "<<dv.array().Size()/1024/1024<<" Mbytes"<<endl;

			ArrayData data(MAX_BUFFER_SIZE);

//			for (auto i = dv.Begin(); i.IsNotEnd(); i.Next())
//			{
//				i.Read(data);
//			}

			t2 = getTime();

			cout<<"Read data randomly"<<endl;

//			for (Int c = 0; c < SIZE; c++)
//			{
//				Int key = get_random(SIZE);
//
//				auto i = dv.Find(key);
//
//				i.Read(data);
//			}

			t3 = getTime();

			cout<<"Remove data"<<endl;

			for (Int c = 0; ;c++)
			{
				if (!Remove(allocator, dv))
				{
					break;
				}

				if (c % 100 == 0) allocator.commit();
			}

			allocator.commit();

			t4 = getTime();



//

//			Dump(allocator);

//			allocator.DumpPages();
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

	cout<<"ARRAY TEST times: RND Remove: "<<(t4 - t3)<<" RND Read: "<<(t3 - t2)<<" LNR Read: "<<(t2-t1)<<" Insert: "<<(t1 - t0)<<endl;

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
