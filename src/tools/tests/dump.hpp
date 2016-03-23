
// Copyright 2011 Victor Smirnov
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

#include <memoria/v1/allocators/persistent-inmem/factory.hpp>
#include <memoria/v1/metadata/container.hpp>

#include <memory>

namespace memoria {
namespace v1 {

template <typename AllocatorT>
static void LoadFile(const std::shared_ptr<AllocatorT>& allocator, const char* file)
{
    auto in = FileInputStreamHandler::create(file);
    allocator->load(in.get());
}

static String getPath(String dump_name)
{
    if (isEndsWith(dump_name, ".dump"))
    {
        auto idx = dump_name.find_last_of(".");
        String name = dump_name.substr(0, idx);
        return name;
    }
    else {
        return dump_name+".data";
    }
}

static Int DumpAllocator(String file_name)
{
    try {
        logger.level() = Logger::NONE;

        File file(file_name);
        if (file.isDirectory())
        {
            cerr<<"ERROR: "<<file.getPath()<<" is a directory"<<endl;
            return 1;
        }
        else if (!file.isExists())
        {
            cerr<<"ERROR: "<<file.getPath()<<" does not exists"<<endl;
            return 1;
        }

        File path(getPath(file_name));
        if (path.isExists() && !path.isDirectory())
        {
            cerr<<"ERROR: "<<path.getPath()<<" is not a directory"<<endl;
            return 1;
        }
        
            auto is = FileInputStreamHandler::create(file.getPath().c_str());

            auto allocator = PersistentInMemAllocator<>::load(is.get());
            
            cout<<"Load InMemAllocator file: "+file.getPath()<<endl;

            auto start = getTimeInMillis();

            LoadFile(allocator, file.getPath().c_str());

            auto end = getTimeInMillis();

            cout<<"Loading time: "<<FormatTime(end-start)<<endl;

            FSDumpAllocator(allocator, path.getAbsolutePath());
    }
    catch (Exception& ex) {
        cout<<"Exception "<<ex.source()<<" "<<ex<<endl;
    }
    catch (MemoriaThrowable* ex) {
        cout<<"Exception* "<<ex->source()<<" "<<*ex<<endl;
    }
    catch (MemoriaThrowable& ex) {
        cout<<"Exception "<<ex.source()<<" "<<ex<<endl;
    }
    catch (exception& e) {
        cout<<"StdEx: "<<e.what()<<endl;
    }
    catch(...) {
        cout<<"Unrecognized exception"<<endl;
    }

    return 0;
}

}}
