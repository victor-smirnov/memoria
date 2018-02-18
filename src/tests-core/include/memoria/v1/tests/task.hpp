
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

#include <memoria/v1/tests/params.hpp>

#include <memoria/v1/core/types.hpp>
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
    int32_t     check_step;
    int64_t  memory_limit;
    bool    own_folder;
public:

    TaskParametersSet(U16StringRef name):
        ParametersSet(name),
        enabled(true),
        check_step(100),
        memory_limit(LLONG_MAX),
        own_folder(false)
    {
        Add(u"enabled", enabled);
        Add(u"check_step", check_step);
        Add(u"memory_limit", memory_limit);
        Add(u"own_folder", own_folder)->state();
    }

    virtual bool IsEnabled() const
    {
        return enabled;
    }

    void setEnabled(bool enabled)
    {
        this->enabled = enabled;
    }

    int32_t getcheckStep() const
    {
        return check_step;
    }
};


struct ExampleTaskParams: public TaskParametersSet {

    int32_t     size_;
    int32_t     btree_branching_;
    bool    btree_random_airity_;

    ExampleTaskParams(U16StringRef name): TaskParametersSet(name), size_(1024), btree_branching_(0), btree_random_airity_(true)
    {
        Add(u"size", size_);
        Add(u"btree_branching", btree_branching_);
        Add(u"btree_random_airity", btree_random_airity_);
    }
};



class Task: public TaskParametersSet {
protected:
    int32_t             iteration_;
    int64_t             duration_;
    U16String           output_folder_;

    fstream*            out_;

    int32_t seed_ = -1;

public:
    Task(U16StringRef name):
        TaskParametersSet(name),
        iteration_(0),
        duration_(0),
        out_(NULL)
    {}

    virtual ~Task() throw ();

    int32_t getSeed() const {
        return seed_;
    }

    void setSeed(int32_t v) {
        seed_ = v;
    }

    virtual bool IsGroup() const {
        return false;
    }

    ostream& out() {
        return *out_;
    }

    void setIteration(int32_t iteration)
    {
        iteration_ = iteration;
    }

    int32_t getIteration() const
    {
        return iteration_;
    }

    int64_t getDuration() const {
        return duration_;
    }

    void setDuration(int64_t duration) {
        duration_ = duration;
    }

    U16String getIterationAsString() const
    {
        return U16String(toString(iteration_));
    }

    U16StringRef getOutputFolder() const
    {
        return output_folder_;
    }

    void setOutputFolder(U16StringRef folder)
    {
        output_folder_ = folder;
    }

    U16String getResourcePath(U16StringRef name) const
    {
        return output_folder_ + Platform::getFilePathSeparator() + name;
    }

    bool IsResourceExists(U16StringRef name) const
    {
        U16String path = getResourcePath(name);
        return boost::filesystem::exists(path.to_u8().to_std_string());
    }

    virtual void BuildResources();
    virtual void releaseResources();

    virtual void Prepare() {
        Prepare(*out_);
    }

    virtual int32_t Run();

    virtual void release() {
        release(*out_);
    }

    virtual void Prepare(ostream& out) {}
    virtual void Run(ostream& out)              = 0;
    virtual void release(ostream& out) {}


    virtual void Configure(Configurator* cfg);

    virtual U16String getTaskPropertiesFileName() const {
        return getName() + u".properties";
    }

    virtual U16String getTaskParametersFilePath() {
        return getOutputFolder() + Platform::getFilePathSeparator() + getTaskPropertiesFileName();
    }

    virtual void storeAdditionalProperties(fstream& file) const {}

    virtual void debug1() {}
    virtual void debug2() {}
    virtual void debug3() {}

    virtual void StoreProperties(U16StringRef file_name)
    {
        fstream file;
        file.open(file_name.to_u8().data(), fstream::out | fstream::trunc | fstream::trunc);

        file << "task = " << this->getFullName() << endl;

        storeAdditionalProperties(file);

        this->dumpProperties(file, false, true);

        file.close();
    }

    virtual void LoadProperties(U16StringRef file_name)
    {
        fstream file;
        file.open(file_name.to_u8().data(), fstream::in | fstream::trunc | fstream::trunc);

        Configurator cfg;
        Configurator::Parse(file_name.to_u8().data(), &cfg);

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
        int32_t run_number;
        U16String task_name;

        FailureDescriptor() {}
        FailureDescriptor(int32_t number, U16StringRef name): run_number(number), task_name(name) {}
    };




    vector<FailureDescriptor> failures_;

public:

    TaskGroup(U16StringRef name): Task(name) {
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

    virtual int32_t Run()
    {
        Task::Run();

        int32_t failures = failures_.size();

        failures_.clear();

        return failures;
    }

    template <typename T>
    T* getTask(U16StringRef name)
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

    int32_t run_count;

    GroupRunner(U16StringRef name): TaskGroup(name), run_count(1)
    {
        Add(u"run_count", run_count);
    }

    virtual ~GroupRunner() noexcept {}

//    int32_t getSeed() const {
//      return seed_ >= 0 ? seed_ : (getTimeInMillis() % 1000000);
//    }



    U16StringRef getOutput() const
    {
        return getOutputFolder();
    }

    virtual U16String getTaskOutputFolder(U16String task_name, int32_t run) const
    {
        if (getOutput().is_empty())
        {
            return task_name + u"-" + U16String(toString(run));
        }
        else {
            return getOutput() + Platform::getFilePathSeparator() + task_name + u"-" + U16String(toString(run));
        }
    }

    void setOutput(U16StringRef out)
    {
        setOutputFolder(out);
    }

    int32_t getRunCount() const
    {
        return run_count;
    }

    void setRunCount(int32_t count)
    {
        run_count = count;
    }

    virtual int32_t Run();

    virtual void dumpProperties(std::ostream& os, bool dump_prefix = true, bool dump_all = false) const;
};


class MemoriaTaskRunner: public GroupRunner {
public:
    MemoriaTaskRunner(U16StringRef name = u""): GroupRunner(name) {}
    virtual ~MemoriaTaskRunner() throw ()           {}
};



}}
