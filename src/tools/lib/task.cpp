
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/v1/tools/task.hpp>
#include <memoria/v1/tools/tools.hpp>
#include <memoria/v1/core/tools/file.hpp>

#include <algorithm>
#include <memory>
#include <sstream>
#include <map>

namespace memoria {
namespace v1 {

using std::map;
using namespace std;

Task::~Task() throw () {

}


void Task::Configure(Configurator* cfg)
{
    this->Process(cfg);
}


void Task::BuildResources()
{
    File output_f(output_folder_);

    bool own_folder = this->own_folder;

    if (!output_f.isExists())
    {
        output_f.mkDirs();
    }

    String out_file_name = output_folder_
                           + Platform::getFilePathSeparator()
                           + (own_folder ? "" : getName() + ".")
                           + "output.txt";

    out_ = new fstream();
    out_->exceptions ( fstream::failbit | fstream::badbit );
    out_->open(out_file_name, fstream::out);
}

void Task::releaseResources()
{
    out_->close();
    delete out_;
}


Int Task::Run()
{
    BuildResources();

    bool result;

    try {
        BigInt t0  = getTimeInMillis();

        Run(*out_);

        BigInt t1 = getTimeInMillis();

        (*out_)<<"PASSED in "<<FormatTime(t1 - t0)<<endl;

        result = false;
    }
    catch (const std::exception& e)
    {
        (*out_)<<"FAILED: STL exception: "<<e.what()<<" "<<endl;

        String path = getTaskParametersFilePath();

        StoreProperties(path);

        result = true;
    }
    catch (const Exception& e) {
        (*out_)<<"FAILED: "<<e.source()<<": "<<e<<endl;

        String path = getTaskParametersFilePath();

        StoreProperties(path);

        result = true;
    }
    catch (const MemoriaThrowable& e) {
        (*out_)<<"FAILED: "<<e.source()<<": "<<e<<endl;

        String path = getTaskParametersFilePath();

        StoreProperties(path);

        result = true;
    }
    catch (...)
    {
        (*out_)<<"FAILED: Unknown Exception"<<endl;

        String path = getTaskParametersFilePath();

        StoreProperties(path);

        result = true;
    }

    releaseResources();

    return result;
}



TaskGroup::~TaskGroup() throw ()
{
    try {
        for (auto t: tasks_)
        {
            delete t;
        }
    }
    catch (...) {}
}

void TaskGroup::Run(ostream& out)
{
    for (auto t: tasks_)
    {
        if (t->IsEnabled())
        {
            String folder;

            if (t->own_folder)
            {
                folder = output_folder_ + Platform::getFilePathSeparator() + t->getName();
            }
            else {
                folder = output_folder_;
            }

            t->setOutputFolder(folder);
            t->setIteration(1);

            t->setSeed(this->getSeed());

            if (t->Run())
            {
                failures_.push_back(FailureDescriptor(t->getIteration(), t->getName()));
            }
        }
    }

    cout<<getIteration()<<") "<<getName();

    if (failures_.size() > 0)
    {
        cout<<Term::red_s()<<" FAILED: "<<Term::red_e();
        out<<"FAILED: ";

        for (UInt c = 0; c < failures_.size(); c++)
        {
            out<<failures_[c].task_name;
            cout<<failures_[c].task_name;

            if (c < failures_.size() - 1) {
                out<<", ";
                cout<<", ";
            }
        }
    }
    else {
        cout<<Term::green_s()<<" PASSED"<<Term::green_e();
    }

    out<<endl;
    cout<<endl;
}

void TaskGroup::registerTask(Task* task)
{
    for (auto t: tasks_)
    {
        if (t->getName() == task->getName())
        {
            throw Exception(MEMORIA_SOURCE, SBuf()<<"Task "<<task->getName()<<" is already registered");
        }
    }

    task->setContext(this);

    tasks_.push_back(task);
}

void TaskGroup::Configure(Configurator* cfg)
{
    Process(cfg);
    for (auto t: tasks_)
    {
        t->Configure(cfg);
    }
}

void TaskGroup::BuildResources()
{
    Task::BuildResources();
}

void TaskGroup::releaseResources()
{
    Task::releaseResources();
}

void TaskGroup::dumpProperties(std::ostream& os, bool dump_prefix, bool dump_all) const
{
    os<<"# ============== "<<this->getFullName()<<" ===================="<<endl<<endl;
    for (auto t: tasks_)
    {
        t->dumpProperties(os, dump_prefix, dump_all);
    }
    os<<endl<<endl;
}

bool TaskGroup::IsEnabled() const
{
    for (auto t: tasks_)
    {
        if (t->IsEnabled())
        {
            return true;
        }
    }

    return false;
}




Int GroupRunner::Run()
{
    BuildResources();

    Int counter = 0;

    Int run_count = getRunCount();

    for (Int c = 1; c <= run_count; c++)
    {
        String task_folder;
        if (run_count > 1)
        {
            String folder_name = "run-" + toString(c);
            task_folder = output_folder_ + Platform::getFilePathSeparator() + folder_name;
        }
        else {
            task_folder = output_folder_;
        }

        File folder(task_folder);
        if (!folder.isExists())
        {
            folder.mkDirs();
        }

        for (auto t: tasks_)
        {
            if (t->IsEnabled())
            {
                String folder;

                if (t->own_folder)
                {
                    folder = task_folder + Platform::getFilePathSeparator() + t->getName();
                }
                else {
                    folder = task_folder;
                }

                t->setOutputFolder(folder);
                t->setIteration(c);

                Int seed = this->getSeed();

                t->setSeed(seed);

                if (Int failures = t->Run() > 0)
                {
                    failures_.push_back(FailureDescriptor(t->getIteration(), t->getName()));

                    counter += failures;
                }
            }
        }
    }

    ostream& out = *out_;

    if (failures_.size() > 0)
    {
        cout<<Term::red_s()<<"FAILURES: "<<Term::red_e()<<endl;
        out<<"FAILURES: "<<endl;

        map<String, vector<Int>> failures;

        for (FailureDescriptor descr: failures_)
        {
            failures[descr.task_name].push_back(descr.run_number);
        }

        for (auto failure: failures)
        {
            stringstream list;

            vector<Int>& numbers = failure.second;

            for (UInt c = 0; c < numbers.size(); c++)
            {
                list<<numbers[c];

                if (c < numbers.size() - 1)
                {
                    list<<", ";
                }
            }

            cout<<failure.first<<": "<<list.str()<<endl;
            out<<failure.first<<": "<<list.str()<<endl;
        }
    }
    else {
        cout<<Term::green_s()<<"PASSED: ALL"<<Term::green_e()<<endl;
        out<<"PASSED: ALL"<<endl;
    }

    releaseResources();

    return counter;
}

void GroupRunner::dumpProperties(std::ostream& os, bool dump_prefix, bool dump_all) const
{
    for (auto t: tasks_)
    {
        t->dumpProperties(os, dump_prefix, dump_all);
    }
}

}}