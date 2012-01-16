
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

class UpdateOp: public ParametersSet {
public:
	UpdateOp(StringRef name): ParametersSet(name) {}
	virtual ~UpdateOp() throw () {}

	virtual void Run() = 0;
};


class TestTask: public Task {
	map<String, UpdateOp*> operations_;

public:
	TestTask(ParametersSet* parameters): Task(parameters) {}
	virtual ~TestTask() throw ();

	void RegisterUpdateOp(UpdateOp* op);
	void Replay(Configurator* cfg, StringRef dump_file_name);
	UpdateOp* GetUpdateOp(StringRef name) const;

	virtual void ExecuteUpdateOp(UpdateOp* op, StringRef dump_file_name) = 0;

	virtual void Run(ostream& out);

	virtual void Prepare() {}
	virtual void DumpAllocator() = 0;
			void RunOp(UpdateOp* op);
	virtual void Finish() {}
	virtual void ExecuteTask(ostream& out) = 0;
public:

	String GetFileName(StringRef name);
};


template <typename Profile_, typename Allocator_>
class ProfileUpdateOp: public UpdateOp {

	Allocator_* allocator_;

public:
	typedef Profile_ 								Profile;
	typedef Allocator_ 								Allocator;

	ProfileUpdateOp(StringRef name): UpdateOp(name) {}
	virtual ~ProfileUpdateOp() throw () {}

	Allocator* GetAllocator()
	{
		return allocator_;
	}

	void SetAllocator(Allocator* allocator)
	{
		this->allocator_ = allocator;
	}

	virtual void Run() = 0;
};

template <typename Profile_, typename Allocator_>
class ProfileTestTask: public TestTask {

	typedef ProfileUpdateOp<Profile_, Allocator_> 	BasicUpdateOp;

	Allocator_ allocator_;

public:

	typedef Profile_ 								Profile;
	typedef Allocator_ 								Allocator;


	ProfileTestTask(ParametersSet* parameters): TestTask(parameters) {}
	virtual ~ProfileTestTask() throw () {};

	virtual Allocator& GetAllocator()
	{
		return allocator_;
	}

	virtual void ExecuteTask(ostream& out) 	= 0;

	virtual void ExecuteUpdateOp(UpdateOp* op, StringRef dump_file_name)
	{
		unique_ptr <FileInputStreamHandler> in(FileInputStreamHandler::create(dump_file_name.c_str()));
		allocator_.load(in.get());

		BasicUpdateOp* profileOp = static_cast<BasicUpdateOp*>(op);

		profileOp->SetAllocator(&allocator_);
		profileOp->Run();
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

	typedef ProfileUpdateOp<StreamProfile<>, DefaultStreamAllocator > Base;

public:
	SPUpdateOp(String name): Base(name)  		{}
	virtual ~SPUpdateOp() throw () 				{}

	virtual void Run() = 0;
};


class SPTestTask: public ProfileTestTask<StreamProfile<>, DefaultStreamAllocator> {

	typedef ProfileTestTask<StreamProfile<>, DefaultStreamAllocator> Base;

public:
	SPTestTask(ParametersSet* parameters): Base(parameters) {}
	virtual ~SPTestTask() throw () {};
};






class TestRunner: public TaskRunner {
public:
	TestRunner(): TaskRunner() {}

	void Replay(Configurator* cfg, StringRef dump_file_name);
};


}
#endif
