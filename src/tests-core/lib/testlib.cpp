
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


#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/tools.hpp>
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

        BigInt t0 = getTimeInMillis();

        try {
            out<<"Test: "<<current_test_name_<<std::endl;
            descr->run(this, out);

            BigInt t1 = getTimeInMillis();

            out<<"TEST PASSED in "<<FormatTime(t1 - t0)<<std::endl<<std::endl;

            this->tearDown();
        }
        catch (...)
        {
            BigInt t1 = getTimeInMillis();

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

    String test_name = cfg->getValue<String>("test");
    const TestDescriptor* descr = findTestDescriptor(test_name);

    current_test_name_ = descr->name();

    if (descr->hasReplay())
    {
        Base::Configure(configurator_);

        this->prepareReplay();

        descr->replay(this, out);
    }
    else {
        throw Exception(MEMORIA_SOURCE, SBuf()<<"Replay method for test "<<test_name<<" is not specified");
    }
}

void TestTask::storeAdditionalProperties(fstream& file) const
{
    file<<"test = "<<current_test_name_<<endl;
}

const TestTask::TestDescriptor* TestTask::findTestDescriptor(StringRef name) const
{
    for (const TestDescriptor* descr: tests_)
    {
        if (descr->name() == name)
        {
            return descr;
        }
    }

    throw Exception(MEMORIA_SOURCE, SBuf()<<"Test "<<name<<" is not found");
}



String TestTask::getFileName(StringRef name) const
{
    return name + ".properties";
}


void MemoriaTestRunner::Replay(ostream& out, StringRef task_folder)
{
    Configurator cfg;
    Configurator task_cfg;

    String replay_file_name;
    String task_file_name;

    if (bf::exists(task_folder))
    {
        replay_file_name = task_folder + Platform::getFilePathSeparator() + "ReplayTask.properties";
        
        if (!bf::exists(replay_file_name))
        {
            throw Exception(MEMORIA_SOURCE, SBuf()<<"File "<<replay_file_name<<" does not exists");
        }

        task_file_name = replay_file_name;
    }
    else {
        throw Exception(MEMORIA_SOURCE, SBuf()<<"File "<<task_folder<<" does not exists");
    }


    Configurator::Parse(replay_file_name.c_str(), &cfg);

    String name = cfg.getProperty("task");
    TestTask* task = getTask<TestTask>(name);
    if (task != NULL)
    {
        try {
            out<<"Task: "<<task->getFullName()<<endl;
            task->setOut(&out);
            task->setReplayMode();
            task->LoadProperties(task_file_name);
            task->configureSeed();
            task->setUp();
            try {
                task->Replay(out, &cfg);
                out<<"PASSED"<<endl;
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
        catch (const Exception& e)
        {
            out << "FAILED: " << e.source() << " " << e << endl;
        }
        catch (const MemoriaThrowable& e)
        {
            out << "FAILED: " << e.source() << " " << e << endl;
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


Int MemoriaTestRunner::Run()
{
    Int result = MemoriaTaskRunner::Run();
    return result;
}


MemoriaTestRunner& tests_runner() {
    static MemoriaTestRunner runner;
    return runner;
}

}}
