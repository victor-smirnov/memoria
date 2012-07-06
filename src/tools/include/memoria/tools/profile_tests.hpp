
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_PROFILE_TESTS_HPP
#define	_MEMORIA_TOOLS_PROFILE_TESTS_HPP


#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>
#include <memoria/memoria.hpp>

#include <map>
#include <memory>
#include <fstream>

namespace memoria {

using namespace std;





template <typename Profile_, typename Allocator_>
class ProfileTestTask: public TestTask {

	typedef Ctr<typename CtrTF<Profile_, Root>::CtrTypes>						RootMapType;

public:

	typedef Profile_ 															Profile;
	typedef Allocator_ 															Allocator;

	Int check_count;

	ProfileTestTask(StringRef name): TestTask(name)
	{
		RootMapType::initMetadata();

		Add("check_count", check_count);
	}

	virtual ~ProfileTestTask() throw () {};

	virtual TestReplayParams* createTestStep(StringRef name) const						= 0;
	virtual void 			Run(ostream& out)											= 0;
	virtual void 			Replay(ostream& out, TestReplayParams* step_params)			= 0;

	virtual void LoadAllocator(Allocator& allocator, StringRef file_name) const
	{
		unique_ptr <FileInputStreamHandler> in(FileInputStreamHandler::create(file_name.c_str()));
		allocator.load(in.get());
	}

	virtual void LoadAllocator(Allocator& allocator, const TestReplayParams* params) const
	{
		unique_ptr <FileInputStreamHandler> in(FileInputStreamHandler::create(params->getDumpName().c_str()));
		allocator.load(in.get());
	}

	virtual void StoreAllocator(Allocator& allocator, StringRef file_name) const
	{
		unique_ptr <FileOutputStreamHandler> out(FileOutputStreamHandler::create(file_name.c_str()));
		allocator.store(out.get());
	}



	virtual void Store(Allocator& allocator, TestReplayParams* params) const
	{
		Configure(params);

		String file_name = getAllocatorFileName(params, ".valid");
		StoreAllocator(allocator, file_name);
		params->setDumpName(file_name);

		String file_name_invalid = getAllocatorFileName(params, ".invalid");
		allocator.commit();
		StoreAllocator(allocator, file_name_invalid);

		String props_name = getPropertiesFileName();
		StoreProperties(params, props_name);
	}



	virtual String getAllocatorFileName(const TestReplayParams* params, StringRef infix = "") const
	{
		return getResourcePath(params->getName()+infix+".dump");
	}


};

template <typename T = EmptyType>
class SPTestTaskT: public ProfileTestTask<SmallProfile<>, SmallInMemAllocator> {

	typedef ProfileTestTask<SmallProfile<>, SmallInMemAllocator> Base;

public:
	SPTestTaskT(StringRef name): Base(name) {}
	virtual ~SPTestTaskT() throw () {};

	virtual TestReplayParams* createTestStep(StringRef name) const						= 0;
	virtual void 			Run(ostream& out)											= 0;
	virtual void 			Replay(ostream& out, TestReplayParams* step_params)			= 0;

	void check(Allocator& allocator, const char* source)
	{
		Int step_count = getParameters<>()->getcheckStep();

		if (step_count > 0 && (check_count % step_count == 0))
		{
			::memoria::check<Allocator>(allocator, "Allocator check failed", source);
		}

		check_count++;
	}

	void check(Allocator& allocator, const char* message, const char* source)
	{
		Int step_count = getParameters<>()->getcheckStep();

		if (check_count % step_count == 0)
		{
			::memoria::check<Allocator>(allocator, message, source);
		}
		check_count++;
	}

	template <typename CtrType>
	void checkCtr(CtrType& ctr, const char* message, const char* source)
	{
		Int step_count = getParameters<>()->getcheckStep();

		if (step_count > 0 && (check_count % step_count == 0))
		{
			::memoria::checkCtr<CtrType>(ctr, message, source);
		}

		check_count++;
	}

	template <typename CtrType>
	void checkCtr(CtrType& ctr, const char* source)
	{
		checkCtr(ctr, "Container check failed", source);
	}
};

typedef SPTestTaskT<> SPTestTask;




}
#endif
