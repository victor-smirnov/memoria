
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

#include <memoria/v1/allocators/inmem/threads/thread_inmem_allocator_impl.hpp>
#include <memoria/v1/metadata/container.hpp>

#include <boost/filesystem.hpp>

#include <memory>

namespace memoria {
namespace v1 {

namespace bf = boost::filesystem;    
    
template <typename AllocatorT>
static void LoadFile(AllocatorT& allocator, const char* file)
{
	auto in = FileInputStreamHandler::create(file);
	allocator.load(in.get());
}

static U16String getPath(U16String dump_name)
{
    if (dump_name.ends_with(u".dump"))
	{
        auto idx = dump_name.find_last_of(u".");
        U16String name = dump_name.substring(0, idx);
		return name;
	}
	else {
        return dump_name + u".data";
	}
}

static int32_t DumpAllocator(U16String file_name)
{
	try {
		logger.level() = Logger::NONE;

        bf::path file(file_name.to_u8().to_std_string());
		if (bf::is_directory(file))
		{
			std::cerr << "ERROR: " << file << " is a directory" << std::endl;
			return 1;
		}
		else if (!bf::exists(file))
		{
			std::cerr << "ERROR: "<< file << " does not exists" << std::endl;
			return 1;
		}

        bf::path path(getPath(file_name).to_u8().to_std_string());
		if (bf::exists(path) && !bf::is_directory(path))
		{
			std::cerr << "ERROR: " << path << " is not a directory" << std::endl;
			return 1;
		}

		auto is = FileInputStreamHandler::create(file.string().c_str());

		auto allocator = ThreadInMemAllocator<>::load(is.get());

		std::cout << "Load InMemAllocator file: " << file << std::endl;

		auto start = getTimeInMillis();

		LoadFile(allocator, file.string().c_str());

		auto end = getTimeInMillis();

        std::cout << "Loading time: " << FormatTime(end-start) << std::endl;

        FSDumpAllocator(allocator, U8String(bf::absolute(path).string()).to_u16());
	}
    catch (MemoriaThrowable* ex) {
        ex->dump(std::cout);
	}
    catch (MemoriaThrowable& ex) {
        ex.dump(std::cout);
	}
	catch (exception& e) {
		std::cout<<"StdEx: "<<e.what()<<std::endl;
	}
	catch(...) {
		std::cout<<"Unrecognized exception"<<std::endl;
	}

	return 0;
}

}}
