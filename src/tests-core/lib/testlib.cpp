
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


#include <memoria/v1/tests/tests.hpp>
#include <memoria/v1/tests/tools.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <boost/filesystem.hpp>

#include <algorithm>
#include <fstream>
#include <memory>

namespace memoria {
namespace v1 {

namespace bf = boost::filesystem;      
    
TestTask::~TestTask() throw ()
{
    for (TestDescriptor* descr: tests_)
    {
        delete(descr);
    }
}

void TestTask::Run(std::ostream& out)
{
    setOut(&out);

    for (TestDescriptor* descr: tests_)
    {
        Base::Configure(configurator_);

        current_test_name_ = descr->name();

        this->configureSeed();
        this->setUp();

        int64_t t0 = getTimeInMillis();

        try {
            out<<"Test: "<<current_test_name_<<std::endl;
            descr->run(this, out);

            int64_t t1 = getTimeInMillis();

            out<<"TEST PASSED in "<<FormatTime(t1 - t0)<<std::endl<<std::endl;

            this->tearDown();
        }
        catch (...)
        {
            int64_t t1 = getTimeInMillis();

            out<<"TEST FAILED in "<<FormatTime(t1 - t0)<<std::endl;
            this->onException();
            this->tearDown();
            throw;
        }
    }
}

void TestTask::Replay(ostream& out, Configurator* cfg)
{
    setOut(&out);
    setReplayMode();

    configurator_ = cfg;

    U16String test_name = cfg->getValue<U16String>(u"test");
    const TestDescriptor* descr = findTestDescriptor(test_name);

    current_test_name_ = descr->name();

    if (descr->hasReplay())
    {
        Base::Configure(configurator_);

        this->prepareReplay();

        descr->replay(this, out);
    }
    else {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"Replay method for test {} is not specified", test_name));
    }
}

void TestTask::storeAdditionalProperties(fstream& file) const
{
    file<<"test = "<<current_test_name_<<endl;
}

const TestTask::TestDescriptor* TestTask::findTestDescriptor(U16StringRef name) const
{
    for (const TestDescriptor* descr: tests_)
    {
        if (descr->name() == name)
        {
            return descr;
        }
    }

    MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"Test {} is not found", name));
}



U16String TestTask::getFileName(U16StringRef name) const
{
    return name + u".properties";
}


void MemoriaTestRunner::Replay(ostream& out, U16StringRef task_folder)
{
    Configurator cfg;
    Configurator task_cfg;

    U16String replay_file_name;
    U16String task_file_name;

    if (bf::exists(task_folder.to_u8().to_std_string()))
    {
        replay_file_name = task_folder + Platform::getFilePathSeparator() + u"ReplayTask.properties";
        
        if (!bf::exists(replay_file_name.to_u8().to_std_string()))
        {
            MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"File {} does not exists", replay_file_name));
        }

        task_file_name = replay_file_name;
    }
    else {
        MMA1_THROW(TestException()) << WhatInfo(fmt::format8(u"File {} does not exists", task_folder));
    }


    Configurator::Parse(replay_file_name, &cfg);

    U16String name = cfg.getProperty(u"task");
    TestTask* task = getTask<TestTask>(name);
    if (task != NULL)
    {
        try {
            out << "Task: " << task->getFullName() << endl;
            task->setOut(&out);
            task->setReplayMode();
            task->LoadProperties(task_file_name);
            task->configureSeed();
            task->setUp();
            try {
                task->Replay(out, &cfg);
                out << "PASSED" << endl;
            }
            catch (...) {
                task->tearDown();
                throw;
            }
        }
        catch (const std::exception& e)
        {
            out << "FAILED: STL exception: " << e.what() << " " << endl;
        }
        catch (const MemoriaThrowable& e)
        {
            e.dump(out);
        }
        catch (...)
        {
            out << "FAILED: Unknown Exception" << endl;
        }
    }
    else {
        out << "FAILED: Task '" << name << "' is not found" << endl;
    }
}


int32_t MemoriaTestRunner::Run()
{
    int32_t result = MemoriaTaskRunner::Run();
    return result;
}


MemoriaTestRunner& tests_runner() {
    static MemoriaTestRunner runner;
    return runner;
}

}}
