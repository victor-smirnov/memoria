
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)





#include <memoria/allocators/stream/factory.hpp>
#include <memoria/core/tools/file.hpp>

#include <iostream>
#include <set>


using namespace memoria;

using namespace std;

typedef DefaultStreamAllocator VStreamAllocator;

VStreamAllocator* manager;
set<IDValue> processed;

void LoadFile(VStreamAllocator& allocator, const char* file)
{
//	allocator->logger().level() = Logger::TRACE;
	FileInputStreamHandler* in = FileInputStreamHandler::create(file);
	allocator.load(in);
	delete in;
}

void DumpTree(const IDValue& id, const File& folder, int& idx);

void DumpTree(MetadataGroup* group, Page* page, const File& folder, Int& cnt)
{
	for (int c = 0; c < group->Size(); c++)
	{
		Metadata* item = group->GetItem(c);

		if (item->GetTypeCode() == Metadata::GROUP)
		{
			DumpTree((MetadataGroup*)item, page, folder, cnt);
		}
		else if (item->GetTypeCode() == Metadata::ID)
		{
			IDValue id;
			IDField* idField = (IDField*) item;

			//FIXME: IDValue
			idField->GetValue(page, 0, id, false);

			IDValue idv = id;
			if ((!idv.IsNull()) && processed.find(id) == processed.end())
			{
				stringstream str;
				str<<cnt<<"___";
				str<<id;

				File folder2(folder.GetPath() + "/" + str.str());
				folder2.MkDirs();

				int cnt0 = 0;
				DumpTree(id, folder2, cnt0);
				cnt++;
			}
		}
	}
}

void DumpTree(const IDValue& id, const File& folder, int& idx)
{
	processed.insert(id);
	Page* page = manager->CreatePageWrapper();

//	FIXME: IDValue
	manager->GetPage(page, id);

	ofstream pagebin((folder.GetPath() + "/page.bin").c_str());
	for (Int c = 0; c < page->Size(); c++)
	{
		pagebin<<(Byte)page->GetByte(c);
	}
	pagebin.close();

	ofstream pagetxt((folder.GetPath() + "/page.txt").c_str());

	PageMetadata* meta = manager->GetMetadata()->GetPageMetadata(page->GetPageTypeHash());
	pagetxt<<meta->Name()<<endl;

	DumpGroup(meta, page, pagetxt, 0, 0);
	DumpTree(meta, page, folder, idx);

	pagetxt.close();
	delete page;
}

MEMORIA_INIT();

int main(int argc, const char** argv, const char** envp)
{

	try {
//		StreamContainerTypesCollection::Init();

		logger.level() = Logger::NONE;

		if (argc != 3) {
			cerr<<"Usage: dump file.dump /path/to/folder/to/dump/in"<<endl;
			return 1;
		}

		File file(argv[1]);
		if (file.IsDirectory())
		{
			cerr<<"ERROR: "<<file.GetPath()<<" is a directory"<<endl;
			return 1;
		}
		else if (!file.IsExists())
		{
			cerr<<"ERROR: "<<file.GetPath()<<" does not exists"<<endl;
			return 1;
		}

		File path(argv[2]);
		if (path.IsExists() && !path.IsDirectory())
		{
			cerr<<"ERROR: "<<path.GetPath()<<" is not a directory"<<endl;
			return 1;
		}

		if (!path.IsExists())
		{
			path.MkDirs();
		}
		else {
			path.DelTree();
			path.MkDirs();
		}


		VStreamAllocator allocator;
		manager = &allocator;

		cout<<"Load file: "+file.GetPath()<<endl;

//		char cwd_buf[16384];
//		::getcwd(cwd_buf, 16384);
//
//		cout<<"CWD: "<<cwd_buf<<endl;

		LoadFile(allocator, file.GetPath().c_str());



		VStreamAllocator::RootMapType* root = manager->roots();
		auto iter = root->Begin();

		while (!iter.IsFlag(memoria::vapi::Iterator::ITEREND))
		{
			BigInt  name 	= iter.GetKey(0);

			BigInt  value 	= iter.GetData();
			IDValue id(value);

//			iter.model().Dump(iter.page());

			cout<<"Dumping name="<<name<<" root="<<id<<endl;

			stringstream str;
			str<<name<<"___"<<id;

			File folder(path.GetPath() + "/" + str.str());

			if (folder.IsExists())
			{
				if (!folder.DelTree())
				{
					throw MemoriaException("dump.cpp", "can't remove file "+folder.GetPath());
				}
			}

			folder.MkDirs();

			int idx = 0;
			DumpTree(id, folder, idx);

			iter.Next();
		}
	}
	catch (MemoriaException ex) {
		cout<<"MemoriaException "<<ex.source()<<" "<<ex.message()<<endl;
	}
	catch (MemoriaException *ex) {
		cout<<"MemoriaException* "<<ex->source()<<" "<<ex->message()<<endl;
	}
	catch (int i) {
		cout<<"IntegerEx: "<<i<<endl;
	}
	catch (exception e) {
		cout<<"StdEx: "<<e.what()<<endl;
	}
	catch(...) {
		cout<<"Unrecognized exception"<<endl;
	}
}

