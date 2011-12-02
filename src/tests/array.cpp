
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
const Int MAX_BUFFER_SIZE 	= 4096 * 10;


typedef StreamContainerTypesCollection::Factory<Array>::Type 	ByteArray;
typedef ByteArray::Iterator										BAIterator;
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

BigInt GetRandomPosition(ByteArray& array)
{
	return get_random(array.Size());
}

void CheckAllocator(SAllocator &allocator, const char* err_msg)
{
	Int src_level = ByteArray::class_logger().level();
	ByteArray::class_logger().level() = Logger::TRACE;

	memoria::StreamContainersChecker checker(allocator);

	if (checker.CheckAll())
	{
		throw MemoriaException(MEMORIA_SOURCE, err_msg);
	}

	ByteArray::class_logger().level() = src_level;
}

bool CompareBuffer(BAIterator& iter, ArrayData& data)
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

void CheckBufferWritten(BAIterator& iter, ArrayData& data, const char* err_msg)
{
	if (!CompareBuffer(iter, data))
	{
		throw MemoriaException(MEMORIA_SOURCE, err_msg);
	}
}

void Build(SAllocator& allocator, ByteArray& array, UByte value)
{
	ArrayData data = CreateRandomBuffer(value);

	if (array.Size() == 0)
	{
		//Insert buffer into an empty array
		auto iter = array.Seek(0);

		iter.Insert(data);

		CheckAllocator(allocator, "Insertion into an empty array failed. See the dump for details.");

		auto iter1 = array.Seek(0);
		CheckBufferWritten(iter1, data, "AAA");
	}
	else {
		int op = get_random(3);

		if (op == 0)
		{
			//Insert at the start of the array
			auto iter = array.Seek(0);

			BigInt len = array.Size();
			if (len > 100) len = 100;

			ArrayData postfix(len);
			iter.Read(postfix);

			iter.Skip(-len);

			iter.Insert(data);

			CheckAllocator(allocator, "Insertion at the start of the array failed. See the dump for details.");

			iter.Skip(-data.size());

			CheckBufferWritten(iter, data, "Failed to read and compare buffer from array");
			CheckBufferWritten(iter, postfix, "Failed to read and compare buffer postfix from array");
		}
		else if (op == 1)
		{
			//Insert at the end of the array
			auto iter = array.Seek(array.Size());

			BigInt len = array.Size();
			if (len > 100) len = 100;

			ArrayData prefix(len);
			iter.Skip(-len);
			iter.Read(prefix);

			iter.Insert(data);

			CheckAllocator(allocator, "Insertion at the end of the array failed. See the dump for details.");

			iter.Skip(-data.size() - len);

			CheckBufferWritten(iter, prefix, "Failed to read and compare buffer prefix from array");
			CheckBufferWritten(iter, data, "Failed to read and compare buffer from array");
		}
		else {
			//Insert at the middle of the array

			Int pos = GetRandomPosition(array);
			auto iter = array.Seek(pos);

			if (get_random(2) == 0)
			{
				iter.Skip(-iter.data_pos());
				pos = iter.pos();
			}


			BigInt prefix_len = pos;
			if (prefix_len > 100) prefix_len = 100;

			BigInt postfix_len = array.Size() - pos;
			if (postfix_len > 100) postfix_len = 100;

			ArrayData prefix(prefix_len);
			ArrayData postfix(postfix_len);

			iter.Skip(-prefix_len);

			iter.Read(prefix);
			iter.Read(postfix);

			iter.Skip(-postfix.size());

			iter.Insert(data);

			CheckAllocator(allocator, "Insertion at the middle of the array failed. See the dump for details.");

			iter.Skip(- data.size() - prefix_len);

			CheckBufferWritten(iter, prefix, 	"Failed to read and compare buffer prefix from array");
			CheckBufferWritten(iter, data, 		"Failed to read and compare buffer from array");
			CheckBufferWritten(iter, postfix, 	"Failed to read and compare buffer postfix from array");
		}
	}
}

bool Remove(SAllocator& allocator, ByteArray& array)
{
	if (array.Size() < 20000)
	{
		auto iter = array.Seek(0);
		iter.Remove(array.Size());

		CheckAllocator(allocator, "Remove ByteArray");
		return array.Size() > 0;
	}
	else {
		BigInt size = get_random(array.Size() < 40000 ? array.Size() : 40000);
		int op = get_random(3);

		if (op == 0)
		{
			//Remove at the start of the array
			auto iter = array.Seek(0);

			BigInt len = array.Size() - size;
			if (len > 100) len = 100;

			ArrayData postfix(len);
			iter.Skip(size);
			iter.Read(postfix);
			iter.Skip(-len - size);

			iter.Remove(size);

			CheckAllocator(allocator, "Removing region at the start of the array failed. See the dump for details.");

			CheckBufferWritten(iter, postfix, "Failed to read and compare buffer postfix from array");
		}
		else if (op == 1)
		{
			//Remove at the end of the array
			auto iter = array.Seek(array.Size() - size);

			BigInt len = iter.pos();
			if (len > 100) len = 100;

			ArrayData prefix(len);
			iter.Skip(-len);
			iter.Read(prefix);

			iter.Remove(size);

			CheckAllocator(allocator, "Removing region at the end of the array failed. See the dump for details.");

			iter.Skip(-len);

			CheckBufferWritten(iter, prefix, "Failed to read and compare buffer prefix from array");
		}
		else {
			//Remove at the middle of the array

			Int pos = get_random(array.Size() - size);
			auto iter = array.Seek(pos);

			if (get_random(2) == 0)
			{
				iter.Skip(-iter.data_pos());
				pos = iter.pos();
			}

			BigInt prefix_len = pos;
			if (prefix_len > 100) prefix_len = 100;

			BigInt postfix_len = array.Size() - (pos + size);
			if (postfix_len > 100) postfix_len = 100;

			ArrayData prefix(prefix_len);
			ArrayData postfix(postfix_len);

			iter.Skip(-prefix_len);

			iter.Read(prefix);

			iter.Skip(size);

			iter.Read(postfix);

			iter.Skip(-postfix.size() - size);

			iter.Remove(size);

			CheckAllocator(allocator, "Removing region at the middle of the array failed. See the dump for details.");

			iter.Skip(-prefix_len);

			CheckBufferWritten(iter, prefix, 	"Failed to read and compare buffer prefix from array");
			CheckBufferWritten(iter, postfix, 	"Failed to read and compare buffer postfix from array");
		}

		return array.Size() > 0;
	}
}


int main(int argc, const char** argv, const char **envp) {

	long long t0 = getTime(), t1;

	try {
		logger.level() = Logger::NONE;

		StreamContainerTypesCollection::Init();

		SAllocator allocator;
		allocator.GetLogger()->level() = Logger::NONE;

		ByteArray dv(allocator, ArrayName, true);
//		dv.SetMaxChildrenPerNode(100);


		try {
			cout<<"Insert data"<<endl;

			for (Int c = 0; c < SIZE; c++)
			{
//				cout<<"C="<<c<<endl;
				Build(allocator, dv, c + 1);
			}

			t1 = getTime();

			cout<<"Remove data. ByteArray contains "<<dv.Size()/1024/1024<<" Mbytes"<<endl;

			for (Int c = 0; ; c++)
			{
//				cout<<"C="<<c<<endl;
//				if (c == 82)
//				{
//					dv.debug() = true;
//				}
//				else {
//					dv.debug() = false;
//				}

				if (!Remove(allocator, dv))
				{
					break;
				}
			}

			Dump(allocator);
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

	Int CtrTotal = 0, DtrTotal = 0;
	for (Int c = 0; c < (Int)(sizeof(PageCtrCnt)/sizeof(Int)); c++)
	{
		cout<<c<<" "<<PageCtrCnt[c]<<" "<<PageDtrCnt[c]<<" "<<(PageCtrCnt[c] + PageDtrCnt[c])<<endl;
		CtrTotal += PageCtrCnt[c];
		DtrTotal += PageDtrCnt[c];
	}

	cout<<"Total: "<<CtrTotal<<" "<<DtrTotal<<" "<<(CtrTotal + DtrTotal)<<endl;
	cout<<"Total: "<<PageCtr<<" "<<PageDtr<<" "<<(PageCtr + PageDtr)<<endl;
}
