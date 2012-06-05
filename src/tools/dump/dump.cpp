
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)





#include <memoria/allocators/inmem/factory.hpp>
#include <memoria/core/tools/file.hpp>
#include <memoria/core/tools/platform.hpp>

#include <iostream>
#include <set>


using namespace memoria;

using namespace std;

typedef SmallInMemAllocator VStreamAllocator;

VStreamAllocator* manager;
set<IDValue> processed;

void LoadFile(VStreamAllocator& allocator, const char* file)
{
	FileInputStreamHandler* in = FileInputStreamHandler::create(file);
	allocator.load(in);
	delete in;
}

struct NamedIDValue {
	const char*  	name;
	Int 			index;
	Int 			count;
	IDValue			id[10];
};

typedef vector<NamedIDValue> IDValueVector;

class IDSelector: public IPageDataEventHandler {
	IDValueVector values_;

	Int idx_;
	const char* name_;
	bool line_;

public:
	IDSelector():idx_(0), line_(false) {}

	virtual ~IDSelector() {}

	virtual void StartPage(const char* name) {}
	virtual void EndPage() {}

	virtual void StartLine(const char* name, Int size = -1)
	{
		line_ = true;
		name_ = name;
	}

	virtual void EndLine() {
		line_ = false;
		idx_++;
	}

	virtual void StartGroup(const char* name, Int elements = -1)
	{
		name_ = name;
		idx_ = 0;
	}

	virtual void EndGroup() {}

	virtual void Value(const char* name, const Byte* value, Int count = 1, Int kind = 0) 		{}
	virtual void Value(const char* name, const UByte* value, Int count = 1, Int kind = 0) 		{}
	virtual void Value(const char* name, const Short* value, Int count = 1, Int kind = 0)		{}
	virtual void Value(const char* name, const UShort* value, Int count = 1, Int kind = 0)		{}
	virtual void Value(const char* name, const Int* value, Int count = 1, Int kind = 0)			{}
	virtual void Value(const char* name, const UInt* value, Int count = 1, Int kind = 0)		{}
	virtual void Value(const char* name, const BigInt* value, Int count = 1, Int kind = 0)		{}
	virtual void Value(const char* name, const UBigInt* value, Int count = 1, Int kind = 0)		{}

	virtual void Value(const char* name, const IDValue* value, Int count = 1, Int kind = 0)
	{
		NamedIDValue entry;

		entry.count = count;
		entry.name 	= name_;
		entry.index	= idx_;

		for (Int c = 0; c < count; c++)
		{
			entry.id[c] = *value;
		}

		values_.push_back(entry);

		if (!line_)
		{
			idx_++;
		}
	}

	const IDValueVector& values() const
	{
		return values_;
	}
};


void DumpTree(const IDValue& id, const File& folder);

void DumpTree(PageMetadata* group, Page* page, const File& folder)
{
	IDSelector selector;

	group->GetPageOperations()->GenerateDataEvents(page->Ptr(), DataEventsParams(), &selector);

	for (const NamedIDValue& entry: selector.values())
	{
		for (Int c = 0; c < entry.count; c++)
		{
			const IDValue& id = entry.id[c];

			IDValue idv = id;
			if ((!idv.IsNull()) && processed.find(id) == processed.end())
			{
				stringstream str;

				str<<entry.name<<"-"<<entry.index;

				if (entry.count > 1)
				{
					str<<"."<<c;
				}

				str<<"___"<<id;

				File folder2(folder.GetPath() + Platform::GetFilePathSeparator() + str.str());
				folder2.MkDirs();

				DumpTree(id, folder2);
			}
		}
	}


//	for (int c = 0; c < group->Size(); c++)
//	{
//		Metadata* item = group->GetItem(c);
//
//		if (item->GetTypeCode() == Metadata::GROUP)
//		{
//			DumpTree((MetadataGroup*)item, page, folder, cnt);
//		}
//		else if (item->GetTypeCode() == Metadata::ID)
//		{
//
//		}
//	}
}

void DumpTree(const IDValue& id, const File& folder)
{
	processed.insert(id);
	Page* page = manager->CreatePageWrapper();

//	FIXME: IDValue

	try {
		manager->GetPage(page, id);

		ofstream pagebin((folder.GetPath() + Platform::GetFilePathSeparator() + "page.bin").c_str());
		for (Int c = 0; c < page->Size(); c++)
		{
			pagebin<<(Byte)page->GetByte(c);
		}
		pagebin.close();

		ofstream pagetxt((folder.GetPath() + Platform::GetFilePathSeparator() + "page.txt").c_str());

		PageMetadata* meta = manager->GetMetadata()->GetPageMetadata(page->GetPageTypeHash());

		DumpPage(meta, page, pagetxt);

		DumpTree(meta, page, folder);

		pagetxt.close();
	}
	catch (NullPointerException e)
	{
		cout<<"NullPointerException: "<<e.message()<<" at "<<e.source()<<endl;
	}

	delete page;
}


String GetPath(String dump_name)
{
	if (IsEndsWith(dump_name, ".dump"))
	{
		auto idx = dump_name.find_last_of(".");
		String name = dump_name.substr(0, idx);
		return name;
	}
	else {
		return dump_name+".data";
	}
}


MEMORIA_INIT();

int main(int argc, const char** argv, const char** envp)
{
	SmallCtrTypeFactory::Factory<Root>::Type::Init();
	SmallCtrTypeFactory::Factory<Map1>::Type::Init();
	SmallCtrTypeFactory::Factory<Vector>::Type::Init();
	SmallCtrTypeFactory::Factory<VectorMap>::Type::Init();
	SmallCtrTypeFactory::Factory<Set1>::Type::Init();


	try {
		logger.level() = Logger::NONE;

		if (argc != 3 && argc != 2)
		{
			cerr<<"Usage: dump file.dump [/path/to/folder/to/dump/into]"<<endl;
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

		File path(argc == 3 ? String(argv[2]) : GetPath(argv[1]));
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

		LoadFile(allocator, file.GetPath().c_str());

		VStreamAllocator::RootMapType* root = manager->roots();
		auto iter = root->Begin();

		while (!iter.IsEnd())
		{
			BigInt  name 	= iter.GetKey(0);

			BigInt  value 	= iter.GetValue();
			IDValue id(value);

			cout<<"Dumping name="<<name<<" root="<<id<<endl;

			stringstream str;
			str<<name<<"___"<<id;

			File folder(path.GetPath() + "/" + str.str());

			if (folder.IsExists())
			{
				if (!folder.DelTree())
				{
					throw Exception("dump.cpp", SBuf()<<"can't remove file "<<folder.GetPath());
				}
			}

			folder.MkDirs();

			DumpTree(id, folder);

			iter.Next();
		}
	}
	catch (MemoriaThrowable ex) {
		cout<<"Exception "<<ex.source()<<" "<<ex<<endl;
	}
	catch (MemoriaThrowable *ex) {
		cout<<"Exception* "<<ex->source()<<" "<<*ex<<endl;
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

