
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/memoria.hpp>
#include <memoria/tools/tools.hpp>

using namespace memoria;
using namespace memoria::vapi;



using namespace std;

const Int SIZE = 1000;
const Int ArrayName = 1;
const Int MAX_BUFFER_SIZE = 4096 * 10;


typedef SmallCtrTypeFactory::Factory<Vector>::Type 	ByteArray;
typedef ByteArray::Iterator										BAIterator;
typedef SmallInMemAllocator 									SAllocator;


void Dump(SAllocator& allocator, const char* name = "array.dump")
{
	FileOutputStreamHandler* out = FileOutputStreamHandler::create(name);
	allocator.store(out);
	delete out;
}


BigInt GetRandomPosition(ByteArray& array)
{
	return GetRandom(array.Size());
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

void Insert(SAllocator& allocator, ByteArray& array, ArrayData& data)
{
	BigInt size = array.Size();

	Int data_len = data.size();
	BigInt pos = size != 0 ? GetRandom(size / data_len) * data_len : 0;

	auto iter = array.Seek(pos);
	iter.Insert(data);
}

MEMORIA_INIT();

int main(int argc, const char** argv, const char **envp) {

	long long t0 = GetTimeInMillis();

	try {
		logger.level() = Logger::NONE;



		SAllocator allocator;
		allocator.GetLogger()->level() = Logger::NONE;

		ByteArray dv(allocator, ArrayName, true);
		//dv.SetBranchingFactor(100);

		ArrayData data(4);

		try {
			for (Int c = 0; c < 1000000; c++)
			{
				//cout<<"C="<<c<<endl;

				Insert(allocator, dv, data);
			}

			cout<<"Size: "<<dv.Size()<<" bytes"<<endl;

//			Dump(allocator);
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

	cout<<"TREE MAP time: "<<(GetTimeInMillis()- t0)<<endl;
}
