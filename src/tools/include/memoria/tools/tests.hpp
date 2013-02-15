
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_TESTS_HPP
#define _MEMORIA_TOOLS_TESTS_HPP

#include <memoria/tools/task.hpp>

#include <vector>
#include <ostream>
#include <fstream>
#include <limits.h>

namespace memoria {

using namespace std;

#define MEMORIA_ADD_TEST_PARAM(paramName)\
    Add(#paramName, paramName)


#define MEMORIA_ADD_TEST(testMethodName)\
    MyType::addTest(#testMethodName, &MyType::testMethodName)

#define MEMORIA_ADD_TEST_WITH_REPLAY(testMethodName, replayMethodName)\
	MyType::addTest(#testMethodName, &MyType::testMethodName, &MyType::replayMethodName)



class TestTask: public Task {

    bool    replay_;

protected:
    typedef Task Base;

    Int     size_;
    Int     btree_branching_;
    bool    btree_random_branching_;

    String  current_test_name_;

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
        btree_branching_(0),
        btree_random_branching_(true),
        configurator_(nullptr)
    {
        own_folder = true;

        Add("size", size_);
        Add("btree_branching", btree_branching_);
        Add("btree_random_branching", btree_random_branching_);
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
        Process(cfg);
    }

    virtual void setUp() {}
    virtual void tearDown() {}

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

    virtual void            Replay(ostream& out, Configurator* cfg);
    virtual void            Run(ostream& out);


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
