
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_EXAMPLES_HPP
#define	_MEMORIA_TOOLS_EXAMPLES_HPP


#include <memoria/tools/task.hpp>
#include <memoria/tools/tools.hpp>
#include <memoria/memoria.hpp>

#include <map>
#include <memory>
#include <fstream>

namespace memoria {

using namespace std;


class ExampleTask: public Task {

public:
	ExampleTask(TaskParametersSet* parameters): Task(parameters) {}
	virtual ~ExampleTask() throw () {}

	virtual void Run(ostream& out)											= 0;

public:

	String GetFileName(StringRef name) const;
};


template <typename Profile_, typename Allocator_>
class ProfileExampleTask: public ExampleTask {

public:

	typedef Profile_ 								Profile;
	typedef Allocator_ 								Allocator;


	ProfileExampleTask(TaskParametersSet* parameters): ExampleTask(parameters) {}
	virtual ~ProfileExampleTask() throw () {};


	virtual void Run(ostream& out)											= 0;

	virtual void LoadAllocator(Allocator& allocator, StringRef file_name) const
	{
		unique_ptr <FileInputStreamHandler> in(FileInputStreamHandler::create(file_name.c_str()));
		allocator.load(in.get());
	}

	virtual void StoreAllocator(Allocator& allocator, StringRef file_name) const
	{
		unique_ptr <FileOutputStreamHandler> out(FileOutputStreamHandler::create(file_name.c_str()));
		allocator.store(out.get());
	}

};


class SPExampleTask: public ProfileExampleTask<SmallProfile<>, SmallInMemAllocator> {

	typedef ProfileExampleTask<SmallProfile<>, SmallInMemAllocator> Base;

public:
	SPExampleTask(TaskParametersSet* parameters): Base(parameters) {}
	virtual ~SPExampleTask() throw () {};

	virtual void Run(ostream& out)											= 0;

	void Check(Allocator& allocator, const char* source)
	{
		::memoria::Check<Allocator>(allocator, "Allocator check failed", source);
	}

	void Check(Allocator& allocator, const char* message, const char* source)
	{
		::memoria::Check<Allocator>(allocator, message, source);
	}

	template <typename CtrType>
	void CheckCtr(CtrType& ctr, const char* message, const char* source)
	{
		::memoria::CheckCtr<CtrType>(ctr, message, source);
	}

	template <typename CtrType>
	void CheckCtr(CtrType& ctr, const char* source)
	{
		CheckCtr(ctr, "Container check failed", source);
	}
};


class ExampleRunner: public TaskRunner {
public:
	ExampleRunner(): TaskRunner() 	{}
	virtual ~ExampleRunner() 		{}
};


struct ExampleTaskParams: public TaskParametersSet {

	Int 	size_;
	Int 	btree_airity_;
	bool 	btree_random_airity_;

	ExampleTaskParams(StringRef name): TaskParametersSet(name)
	{
		Add("size", size_, 1024);
		Add("btreeAirity", btree_airity_, -1);
		Add("btreeRandomAirity", btree_random_airity_, true);
	}
};


}
#endif
