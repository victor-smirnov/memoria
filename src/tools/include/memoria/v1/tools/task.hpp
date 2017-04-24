
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/tools/params.hpp>
#include <memoria/v1/core/tools/time.hpp>

#include <boost/filesystem.hpp>

#include <vector>
#include <ostream>
#include <fstream>
#include <limits.h>

namespace memoria {
namespace v1 {




class TaskParametersSet: public ParametersSet {
public:
    bool    enabled;
    Int     check_step;
    BigInt  memory_limit;
    bool    own_folder;
public:

    TaskParametersSet(StringRef name):
        ParametersSet(name),
        enabled(true),
        check_step(100),
        memory_limit(LLONG_MAX),
        own_folder(false)
    {
        Add("enabled", enabled);
        Add("check_step", check_step);
        Add("memory_limit", memory_limit);
        Add("own_folder", own_folder)->state();
    }

    virtual bool IsEnabled() const
    {
        return enabled;
    }

    void setEnabled(bool enabled)
    {
        this->enabled = enabled;
    }

    Int getcheckStep() const
    {
        return check_step;
    }
};


struct ExampleTaskParams: public TaskParametersSet {

    Int     size_;
    Int     btree_branching_;
    bool    btree_random_airity_;

    ExampleTaskParams(StringRef name): TaskParametersSet(name), size_(1024), btree_branching_(0), btree_random_airity_(true)
    {
        Add("size", size_);
        Add("btree_branching", btree_branching_);
        Add("btree_random_airity", btree_random_airity_);
    }
};



class Task: public TaskParametersSet {
protected:
    Int                 iteration_;
    BigInt              duration_;
    String              output_folder_;

    fstream*            out_;

    Int seed_ = -1;

public:
    Task(StringRef name):
        TaskParametersSet(name),
        iteration_(0),
        duration_(0),
        out_(NULL)
    {}

    virtual ~Task() throw ();

    Int getSeed() const {
        return seed_;
    }

    void setSeed(Int v) {
        seed_ = v;
    }

    virtual bool IsGroup() const {
        return false;
    }

    ostream& out() {
        return *out_;
    }

    void setIteration(Int iteration)
    {
        iteration_ = iteration;
    }

    Int getIteration() const
    {
        return iteration_;
    }

    BigInt getDuration() const {
        return duration_;
    }

    void setDuration(BigInt duration) {
        duration_ = duration;
    }

    String getIterationAsString() const
    {
        return toString(iteration_);
    }

    StringRef getOutputFolder() const
    {
        return output_folder_;
    }

    void setOutputFolder(StringRef folder)
    {
        output_folder_ = folder;
    }

    String getResourcePath(StringRef name) const
    {
        return output_folder_ + Platform::getFilePathSeparator() + name;
    }

    bool IsResourceExists(StringRef name) const
    {
        String path = getResourcePath(name);
        return boost::filesystem::exists(path);
    }

    virtual void BuildResources();
    virtual void releaseResources();

    virtual void Prepare() {
        Prepare(*out_);
    }

    virtual Int Run();

    virtual void release() {
        release(*out_);
    }

    virtual void Prepare(ostream& out) {}
    virtual void Run(ostream& out)              = 0;
    virtual void release(ostream& out) {}


    virtual void Configure(Configurator* cfg);

    virtual String getTaskPropertiesFileName() const {
        return getName()+".properties";
    }

    virtual String getTaskParametersFilePath() {
        return getOutputFolder() + Platform::getFilePathSeparator() + getTaskPropertiesFileName();
    }

    virtual void storeAdditionalProperties(fstream& file) const {}

    virtual void debug1() {}
    virtual void debug2() {}
    virtual void debug3() {}

    virtual void StoreProperties(StringRef file_name)
    {
        fstream file;
        file.open(file_name.c_str(), fstream::out | fstream::trunc | fstream::trunc);

        file<<"task = "<<this->getFullName()<<endl;

        storeAdditionalProperties(file);

        this->dumpProperties(file, false, true);

        file.close();
    }

    virtual void LoadProperties(StringRef file_name)
    {
        fstream file;
        file.open(file_name.c_str(), fstream::in | fstream::trunc | fstream::trunc);

        Configurator cfg;
        Configurator::Parse(file_name.c_str(), &cfg);

        this->Process(&cfg);

        file.close();
    }

    virtual void Process(Configurator* cfg) {
        TaskParametersSet::Process(cfg);
    }
};


class TaskGroup: public Task {
public:
    typedef vector<Task*>  Tasks;

protected:
    Tasks   tasks_;



    struct FailureDescriptor {
        Int run_number;
        String task_name;

        FailureDescriptor() {}
        FailureDescriptor(Int number, StringRef name): run_number(number), task_name(name) {}
    };




    vector<FailureDescriptor> failures_;

public:

    TaskGroup(StringRef name): Task(name) {
        own_folder = true;
    }

    virtual ~TaskGroup() throw ();




    virtual bool IsGroup() const {
        return true;
    }

    virtual void Run(ostream& out);

    virtual void registerTask(Task* task);

    virtual void BuildResources();
    virtual void releaseResources();

    virtual void OnFailure(Task* task) {}

    virtual void Configure(Configurator* cfg);

    virtual void dumpProperties(std::ostream& os, bool dump_prefix = true, bool dump_all = false) const;

    virtual Int Run()
    {
        Task::Run();

        Int failures = failures_.size();

        failures_.clear();

        return failures;
    }

    template <typename T>
    T* getTask(StringRef name)
    {
        for (auto t: tasks_)
        {
            if (t->getFullName() == name)
            {
                return T2T_S<T*>(t);
            }
            else if (t->IsGroup())
            {
                TaskGroup* group = T2T_S<TaskGroup*>(t);
                T* task = group->getTask<T>(name);
                if (task != NULL)
                {
                    return task;
                }
            }
        }

        return NULL;
    }

    virtual bool IsEnabled() const;
};


class GroupRunner: public TaskGroup {
public:

    Int run_count;

    GroupRunner(StringRef name): TaskGroup(name), run_count(1)
    {
        Add("run_count", run_count);
    }

    virtual ~GroupRunner() noexcept {}

//    Int getSeed() const {
//      return seed_ >= 0 ? seed_ : (getTimeInMillis() % 1000000);
//    }



    StringRef getOutput() const
    {
        return getOutputFolder();
    }

    virtual String getTaskOutputFolder(String task_name, Int run) const
    {
        if (isEmpty(getOutput()))
        {
            return task_name +"-" + toString(run);
        }
        else {
            return getOutput() + Platform::getFilePathSeparator() + task_name +"-" + toString(run);
        }
    }

    void setOutput(StringRef out)
    {
        setOutputFolder(out);
    }

    Int getRunCount() const
    {
        return run_count;
    }

    void setRunCount(Int count)
    {
        run_count = count;
    }

    virtual Int Run();

    virtual void dumpProperties(std::ostream& os, bool dump_prefix = true, bool dump_all = false) const;
};


class MemoriaTaskRunner: public GroupRunner {
public:
    MemoriaTaskRunner(StringRef name = ""): GroupRunner(name) {}
    virtual ~MemoriaTaskRunner() throw ()           {}
};



}}
