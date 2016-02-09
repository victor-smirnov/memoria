
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_TESTS_HPP
#define _MEMORIA_TOOLS_TESTS_HPP

#include <memoria/tools/task.hpp>

#include <memoria/core/tools/random.hpp>

#include <vector>
#include <ostream>
#include <fstream>
#include <limits.h>

namespace memoria {

using namespace std;

template <typename ChildType = void>
class TestProfile  {};


#define MEMORIA_ADD_TEST_PARAM(paramName)\
    this->Add(#paramName, paramName)


#define MEMORIA_ADD_TEST(testMethodName)\
    this->addTest(#testMethodName, &MyType::testMethodName)

#define MEMORIA_ADD_TEST_WITH_REPLAY(testMethodName, replayMethodName)\
    this->addTest(#testMethodName, &MyType::testMethodName, &MyType::replayMethodName)



class TestTask: public Task {

    bool    replay_;

protected:
    typedef Task Base;

    Int     size_;

    String  current_test_name_;

    RngInt  	int_generator_;
    RngBigInt  	bigint_generator_;

    size_t 		soft_memlimit_;
    size_t 		hard_memlimit_;


    struct TestDescriptor {
        String name_;

        StringRef name() const {
            return name_;
        }
        TestDescriptor(StringRef name): name_(name) {}
        virtual ~TestDescriptor() throw () {}

        virtual void run(TestTask* test, ostream&) const        = 0;
        virtual void replay(TestTask* test, ostream&) const     = 0;
        virtual bool hasReplay() const                          = 0;
    };

    template <typename T>
    class TypedTestDescriptor: public TestDescriptor {
        typedef void (T::*TestMethod)();

        TestMethod run_test_;
        TestMethod replay_test_;
    public:
        TypedTestDescriptor(StringRef name, TestMethod run_test, TestMethod replay_test):
            TestDescriptor(name),
            run_test_(run_test), replay_test_(replay_test) {}

        virtual ~TypedTestDescriptor() throw () {}

        virtual void run(TestTask* test, ostream& out) const {
            T* casted = T2T<T*>(test);
            (casted->*run_test_)();
        }

        virtual void replay(TestTask* test, ostream& out) const {
            T* casted = T2T<T*>(test);
            (casted->*replay_test_)();
        }

        virtual bool hasReplay() const {
            return replay_test_ != nullptr;
        }
    };
private:

    vector<TestDescriptor*>     tests_;
    Configurator*               configurator_;

    ostream* out_;

public:

    TestTask(StringRef name):
        Task(name),
        replay_(false),
        size_(200),
        configurator_(nullptr)
    {
        own_folder = true;

        Add("size", size_);
        Add("size_", size_);
        Add("seed_", seed_);

        Add("soft_memlimit_", soft_memlimit_);
        Add("hard_memlimit_", hard_memlimit_);
    }


    virtual ~TestTask() throw ();

    void setOut(ostream* out)
    {
        out_ = out;
    }

    ostream& out()
    {
        return *out_;
    }

    virtual void Configure(Configurator* cfg)
    {
        configurator_ = cfg;

        String coverage 	= cfg->getValue<String>("coverage", "small");
        Int coverage_size	= cfg->getValue<Int>("coverage_size", 1);

        if (coverage == "smoke")
        {
        	this->smokeCoverage(coverage_size);
        }
        else if (coverage == "small")
        {
        	this->smallCoverage(coverage_size);
        }
        else if (coverage == "normal")
        {
        	this->normalCoverage(coverage_size);
        }
        else if (coverage == "large")
        {
        	this->largeCoverage(coverage_size);
        }
        else {
            throw vapi::Exception(MA_SRC, SBuf()<<"Coverage type "+coverage+" is not recognized");
        }

        soft_memlimit_ = cfg->getValue<size_t>("soft_memlimit", static_cast<size_t>(1) * 1024 * 1024 * 1024);
        hard_memlimit_ = cfg->getValue<size_t>("hard_memlimit", static_cast<size_t>(2) * 1024 * 1024 * 1023);

        Process(cfg);
    }

    virtual void defaultCoverage(Int size) {}

    virtual void smokeCoverage(Int size) {
    	defaultCoverage(size);
    }

    virtual void smallCoverage(Int size) {
    	defaultCoverage(size);
    }

    virtual void normalCoverage(Int size) {
    	defaultCoverage(size);
    }

    virtual void largeCoverage(Int size) {
    	defaultCoverage(size);
    }

    RngInt& getIntTestGenerator() {
    	return int_generator_;
    }
    RngBigInt& getBigIntTestGenerator() {
    	return bigint_generator_;
    }

    const RngInt& getIntGenerator() const {
    	return int_generator_;
    }
    const RngBigInt& getBigIntGenerator() const {
    	return bigint_generator_;
    }

    virtual void configureSeed()
    {
    	Int seed = this->getSeed();
    	if (seed == -1)
    	{
    		seed = getTimeInMillis() % 1000000;
    		setSeed(seed);
    	}

    	std::seed_seq ss({seed});
    	int_generator_.engine().seed(ss);
    	bigint_generator_.engine().seed(ss);

    	this->out()<<"seed = "<<seed<<endl;
    }

    Int getRandom()
    {
    	return int_generator_();
    }

    Int getRandom(Int max)
    {
    	return int_generator_(max);
    }

    BigInt getBIRandom()
    {
    	return bigint_generator_();
    }

    BigInt getBIRandom(BigInt max)
    {
    	return bigint_generator_(max);
    }

    virtual void setUp() {}

    virtual void tearDown() {}

    virtual void onException() {}
    virtual void prepareReplay() {}

    template <typename T>
    using TaskMethodPtr = void (T::*) ();

    template <typename T>
    void addTest(StringRef name, TaskMethodPtr<T> run_test, TaskMethodPtr<T> replay_test = nullptr)
    {
        String tmp;

        if (isStartsWith(name, "run")) {
            tmp = name.substr(3);
        }
        else {
            tmp = name;
        }

        tests_.push_back(new TypedTestDescriptor<T>(tmp, run_test, replay_test));
    }

    virtual void Replay(ostream& out, Configurator* cfg);
    virtual void Run(ostream& out);


    virtual void setReplayMode()
    {
        replay_ = true;
    }

    virtual bool isReplayMode() const
    {
        return replay_;
    }

    virtual String getPropertiesFileName(StringRef infix = "") const
    {
        return getResourcePath("Replay"+infix+".properties");
    }

    virtual String getParametersFilePath() {
        return getResourcePath("Task");
    }

    virtual String getTaskPropertiesFileName() const {
        return "ReplayTask.properties";
    }

    String getFileName(StringRef name) const;

protected:

    const TestDescriptor* findTestDescriptor(StringRef name) const;
    virtual void storeAdditionalProperties(fstream& file) const;

};



class TestSuite: public TaskGroup {
public:
    TestSuite(StringRef name): TaskGroup(name)
    {
    }

    virtual ~TestSuite() throw() {}
};



class MemoriaTestRunner: public MemoriaTaskRunner {
public:
    MemoriaTestRunner(): MemoriaTaskRunner("Tests")         {}
    virtual ~MemoriaTestRunner() throw ()                   {}

    void Replay(ostream& out, StringRef replay_file);

    virtual Int Run();
};




}
#endif
