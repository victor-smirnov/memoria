
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_DUMP_HPP
#define MEMORIA_TESTS_DUMP_HPP

#include <memoria/allocators/file/factory.hpp>
#include <memoria/allocators/inmem/factory.hpp>
#include <memoria/allocators/mvcc/mvcc_allocator.hpp>

#include <memoria/metadata/container.hpp>

namespace memoria {


static void LoadFile(SmallInMemAllocator& allocator, const char* file)
{
    FileInputStreamHandler* in = FileInputStreamHandler::create(file);
    allocator.load(in);
    delete in;
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
        
        Int status = GenericFileAllocator::testFile(file.getPath());  
        
        if (status == 7) 
        {
        	cout<<"Load FileAllocator file: "+file.getPath()<<endl;
        	GenericFileAllocator allocator(file.getPath(), OpenMode::READ);
        	
        	GenericFileAllocator::initMetadata();

        	if (allocator.properties().isMVCC())
        	{
        		cout<<"Dump MVCCV FileAllocator"<<endl;

        		typedef MVCCAllocator<FileProfile<>, GenericFileAllocator::Page> 	TxnMgr;

        		TxnMgr::initMetadata();

        		TxnMgr mvcc_allocator(&allocator);

        		FSDumpMVCCAllocator<GenericFileAllocator>(&mvcc_allocator, path.getAbsolutePath());
        	}
        	else {
        		FSDumpAllocator(&allocator, path.getAbsolutePath());
        	}
        }
        else {
        	SmallInMemAllocator allocator;
        	
        	cout<<"Load InMemAllocator file: "+file.getPath()<<endl;

        	LoadFile(allocator, file.getPath().c_str());

        	FSDumpAllocator(&allocator, path.getAbsolutePath());
        }

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

#endif
