
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_TESTS_HPP
#define	_MEMORIA_TOOLS_TESTS_HPP


#include <memoria/tools/task.hpp>
#include <memoria/memoria.hpp>

#include <map>
#include <memory>

namespace memoria {

using namespace std;

class UpdateOp {

	String name_;

public:
	UpdateOp() {}
	virtual ~UpdateOp() throw () {}


	StringRef GetName() const {
		return name_;
	}

	virtual void Configure(Configurator* cfg) = 0;
};


class TestTask: public Task {
	map<String, UpdateOp*> operations_;

public:
	TestTask(TaskParametersSet* parameters): Task(parameters) {}
	virtual ~TestTask() throw ();

	void RegisterUpdateOp(UpdateOp* op);
	void Replay(Configurator* cfg, StringRef dump_file_name);
	UpdateOp* GetUpdateOp(StringRef name) const;

	virtual void ExecuteUpdateOp(UpdateOp* op, StringRef dump_file_name) = 0;

	virtual void Run(ostream& out);

	virtual void Prepare() {}
	virtual void DumpAllocator() = 0;
	virtual void Finish() {}
	virtual void ExecuteTask(ostream& out) = 0;
};


template <typename Profile, typename Allocator>
class ProfileUpdateOp: public UpdateOp {
public:
	ProfileUpdateOp() {}
	virtual ~ProfileUpdateOp() throw () {}

	virtual void Execute(Allocator& allocator) = 0;
};

template <typename Profile_, typename Allocator_>
class ProfileTestTask: public TestTask {

	typedef ProfileUpdateOp<Profile_, Allocator_> 	BasicUpdateOp;

public:

	typedef Profile_ 								Profile;
	typedef Allocator_ 								Allocator;


	ProfileTestTask(TaskParametersSet* parameters): TestTask(parameters) {}
	virtual ~ProfileTestTask() throw () {};

	virtual Allocator& GetAllocator() 		= 0;
	virtual void ExecuteTask(ostream& out) 	= 0;

	virtual void ExecuteUpdateOp(UpdateOp* op, StringRef dump_file_name)
	{
		Allocator allocator;
		unique_ptr <FileInputStreamHandler> in(FileInputStreamHandler::create(dump_file_name.c_str()));
		allocator.load(in.get());

		BasicUpdateOp* profileOp = static_cast<BasicUpdateOp*>(op);

		profileOp->Execute(allocator);
	}

	virtual void DumpAllocator()
	{
		Allocator& allocator = GetAllocator();

		String name_before = GetTaskName() + ".before.dump";
		unique_ptr <FileOutputStreamHandler> out1(FileOutputStreamHandler::create(name_before.c_str()));
		allocator.store(out1.get());

		allocator.commit();

		String name_after = GetTaskName() + ".after.dump";
		unique_ptr <FileOutputStreamHandler> out2(FileOutputStreamHandler::create(name_after.c_str()));
		allocator.store(out2.get());
	}
};



class SPUpdateOp: public ProfileUpdateOp<StreamProfile<>, DefaultStreamAllocator > {
public:
	SPUpdateOp() {}
	virtual ~SPUpdateOp() throw () {}
};


class SPTestTask: public ProfileTestTask<StreamProfile<>, DefaultStreamAllocator> {

	typedef ProfileTestTask<StreamProfile<>, DefaultStreamAllocator> Base;

public:
	SPTestTask(TaskParametersSet* parameters): Base(parameters) {}
	virtual ~SPTestTask() throw () {};
};






class TestRunner: public TaskRunner {
public:
	TestRunner(): TaskRunner() {}

	void Replay(Configurator* cfg, StringRef dump_file_name);
};


}
#endif
