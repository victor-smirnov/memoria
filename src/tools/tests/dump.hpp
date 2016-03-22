
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once


#include <memoria/allocators/persistent-inmem/factory.hpp>
#include <memoria/metadata/container.hpp>

#include <memory>

namespace memoria {

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

}
