
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/tools/tests.hpp>


#include <algorithm>
#include <fstream>
#include <memory>

namespace memoria {


void TestTask::Replay(ostream& out, Configurator* cfg)
{
    setReplayMode();
	unique_ptr<TestReplayParams> params(cfg != NULL ? ReadTestStep(cfg) : NULL);
    Replay(out, params.get());
}

String TestTask::getFileName(StringRef name) const
{
    return name + ".properties";
}

TestReplayParams* TestTask::ReadTestStep(Configurator* cfg) const
{
    String name = cfg->getProperty("name");
    TestReplayParams* params = createTestStep(name);
    Configure(params);

    params->Process(cfg);

    return params;
}

void TestTask::Configure(TestReplayParams* params) const
{
    params->setTask(getFullName());
    params->setReplay(true);
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
            task->LoadProperties(task, task_file_name);
            task->Replay(out, &cfg);
            out<<"PASSED"<<endl;
        }
        catch (Exception e)
        {
        	out<<"FAILED: "<<e.source()<<" "<<e<<endl;
        }
        catch (MemoriaThrowable e)
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
