
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/tools/tests.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

#include <algorithm>
#include <fstream>
#include <memory>

namespace memoria {

TestTask::~TestTask() throw ()
{
    for (TestDescriptor* descr: tests_)
    {
        delete(descr);
    }
}

void TestTask::Run(std::ostream& out)
{
    for (TestDescriptor* descr: tests_)
    {
        Base::Configure(configurator_);

        current_test_name_ = descr->name();

        this->setUp(out);

        try {

            descr->run(this, out);

            this->tearDown(out);
        }
        catch (...) {
            this->tearDown(out);
            throw;
        }
    }
}

void TestTask::Replay(ostream& out, Configurator* cfg)
{
    setReplayMode();

    configurator_ = cfg;

    String test_name = cfg->getValue<String>("test");
    const TestDescriptor* descr = findTestDescriptor(test_name);

    current_test_name_ = descr->name();

    if (descr->hasReplay())
    {
        Base::Configure(configurator_);
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
    File folder(task_folder);

    Configurator cfg;
    Configurator task_cfg;

    String replay_file_name;
    String task_file_name;

    if (folder.isExists())
    {
        replay_file_name = task_folder + Platform::getFilePathSeparator() + "ReplayTask.properties";
        File file(replay_file_name);

        if (!file.isExists())
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
            task->LoadProperties(task_file_name);
            task->Replay(out, &cfg);
            out<<"PASSED"<<endl;
        }
        catch (std::exception& e)
        {
        	out<<"FAILED: STL exception: "<<e.what()<<" "<<endl;
        }
        catch (Exception& e)
        {
            out<<"FAILED: "<<e.source()<<" "<<e<<endl;
        }
        catch (MemoriaThrowable& e)
        {
            out<<"FAILED: "<<e.source()<<" "<<e<<endl;
        }
        catch (...)
        {
            out<<"FAILED"<<endl;
        }
    }
    else {
        out<<"FAILED: Task '"<<name<<"' is not found"<<endl;
    }
}


Int MemoriaTestRunner::Run()
{
    Int result = MemoriaTaskRunner::Run();
    return result;
}


}
