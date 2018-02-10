
// Copyright 2012 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.



#pragma once

#include <memoria/v1/tests/task.hpp>

#include <memoria/v1/core/tools/random.hpp>

#include <vector>
#include <ostream>
#include <fstream>
#include <limits.h>

namespace memoria {
namespace v1 {

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

    int64_t  size_;
    U16String  current_test_name_;

    RngInt      int_generator_;
    RngInt64   bigint_generator_;

    size_t      soft_memlimit_;
    size_t      hard_memlimit_;


    struct TestDescriptor {
        U16String name_;

        U16StringRef name() const {
            return name_;
        }
        TestDescriptor(U16StringRef name): name_(name) {}
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
        TypedTestDescriptor(U16StringRef name, TestMethod run_test, TestMethod replay_test):
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

    TestTask(U16StringRef name):
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

        String coverage     = cfg->getValue<String>("coverage", "small");
        int32_t coverage_size   = cfg->getValue<int32_t>("coverage_size", 1);

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
            throw Exception(MA_SRC, SBuf()<<"Coverage type "+coverage+" is not recognized");
        }

        soft_memlimit_ = cfg->getValue<size_t>("soft_memlimit", static_cast<size_t>(1) * 1024 * 1024 * 1024);
        hard_memlimit_ = cfg->getValue<size_t>("hard_memlimit", static_cast<size_t>(2) * 1024 * 1024 * 1023);

        Process(cfg);
    }

    virtual void defaultCoverage(int32_t size) {}

    virtual void smokeCoverage(int32_t size) {
        defaultCoverage(size);
    }

    virtual void smallCoverage(int32_t size) {
        defaultCoverage(size);
    }

    virtual void normalCoverage(int32_t size) {
        defaultCoverage(size);
    }

    virtual void largeCoverage(int32_t size) {
        defaultCoverage(size);
    }

    RngInt& getIntTestGenerator() {
        return int_generator_;
    }
    RngInt64& getInt64TestGenerator() {
        return bigint_generator_;
    }

    const RngInt& getIntGenerator() const {
        return int_generator_;
    }
    const RngInt64& getInt64Generator() const {
        return bigint_generator_;
    }

    virtual void configureSeed()
    {
        int32_t seed = this->getSeed();
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

    int32_t getRandom()
    {
        return int_generator_();
    }

    int32_t getRandom(int32_t max)
    {
        return int_generator_(max);
    }

    int64_t getBIRandom()
    {
        return bigint_generator_();
    }

    int64_t getBIRandom(int64_t max)
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
    void addTest(U16StringRef name, TaskMethodPtr<T> run_test, TaskMethodPtr<T> replay_test = nullptr)
    {
        U16String tmp;

        if (name.starts_with(u"run")) {
            tmp = name.substring(3);
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

    virtual U16String getPropertiesFileName(U16StringRef infix = "") const
    {
        return getResourcePath(U16String(u"Replay") + infix + u".properties");
    }

    virtual U16String getParametersFilePath() {
        return getResourcePath(u"Task");
    }

    virtual U16String getTaskPropertiesFileName() const {
        return u"ReplayTask.properties";
    }

    U16String getFileName(U16StringRef name) const;

protected:

    const TestDescriptor* findTestDescriptor(U16StringRef name) const;
    virtual void storeAdditionalProperties(fstream& file) const;

};



class TestSuite: public TaskGroup {
public:
    TestSuite(U16StringRef name): TaskGroup(name)
    {
    }

    virtual ~TestSuite() noexcept {}
};



class MemoriaTestRunner: public MemoriaTaskRunner {
public:
    MemoriaTestRunner(): MemoriaTaskRunner(u"Tests")         {}
    virtual ~MemoriaTestRunner() throw ()                   {}

    void Replay(ostream& out, U16StringRef replay_file);

    virtual int32_t Run();
};

MemoriaTestRunner& tests_runner();

template <typename SuiteClass>
struct TestSuiteInit {
    TestSuiteInit() {
        tests_runner().registerTask(new SuiteClass());
    }
}; 

#define MMA1_REGISTER_TEST_SUITE(SuiteClass)    \
namespace {                                     \
    TestSuiteInit<SuiteClass> init;             \
}

}}
