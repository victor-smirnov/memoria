
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)





#include <memoria/allocators/stream/factory.hpp>
#include <memoria/core/tools/file.hpp>

#include "tools.hpp"

#include <iostream>


using namespace memoria;

using namespace std;

typedef DefaultStreamAllocator VStreamAllocator;

VStreamAllocator* manager;
set<IDValue> processed;

void LoadFile(VStreamAllocator *allocator, const char* file)
{
	FileInputStreamHandler* in = FileInputStreamHandler::create(file);
	allocator->load(in);
	delete in;
}

void Expand(ostream& os, Int level)
{
	for (Int c = 0; c < level; c++) os<<" ";
}

void DumpField(FieldMetadata* field, Page* page, ostream &out, Int level, Int idx)
{
	stringstream str;
	Expand(str, level);
	str<<"FIELD: ";
	str<<idx<<" "<<field->Name()<<" "<<field->Ptr();

	if (field->Count() > 1) {
		str<<" "<<field->Count();
	}

	int size = str.str().size();
	Expand(str, 30 - size);
	out<<str.str();

	if (field->GetTypeCode() == FieldMetadata::ID)
	{

		IDField* idField = (IDField*) field;

		Expand(out,12);
		for (Int c = 0; c < field->Count(); c++)
		{
			IDValue id;
			idField->GetValue(page, c, id, false);
			out<<id<<" ";
		}
	}
	else if (field->GetTypeCode() == FieldMetadata::BITMAP || field->GetTypeCode() == FieldMetadata::FLAG)
	{
		BitmapField* bitField = (BitmapField*) field;
		Expand(out,12);
		for (Int c = 0; c < field->Count(); c++)
		{
			out<<bitField->GetBits(page, c, 1, false);
		}
	}
	else
	{
		NumberField* number = static_cast<NumberField*>(field);

		for (Int c = 0; c < field->Count(); c++)
		{
			BigInt val = number->GetValue(page, c, false);
			out.width(12);
			out<<val;
			out<<" (";
			out<<hex;
			out.width(12);
			out<<val;
			out<<dec;
			out<<") ";
		}
	}

	out<<endl;
}

void DumpMap(MetadataGroup* group, Page* page, const File& folder, ostream &out, Int level, Int idx);
void DumpData(MetadataGroup* group, Page* page, const File& folder, ostream &out, Int level, Int idx);

void DumpGroup(MetadataGroup* group, Page* page, const File& folder, ostream &out, Int level, Int idx, Int size = -1)
{
	Expand(out, level);
	out<<group->Name()<<": "<<idx;

	Int size0 = size >= 0 ? size : group->Size();

	out<<" "<<size0<<endl;

	for (Int c = 0;c < size0; c++)
	{
		Metadata* item = group->GetItem(c);
		if (item->IsGroup())
		{
			if (item->Name() == "MAP")
			{
				DumpMap((MetadataGroup*)item, page, folder, out, level + 1, c);
			}
			else if (item->Name() == "DATA")
			{
				DumpData((MetadataGroup*)item, page, folder, out, level + 1, c);
			}
			else {
				DumpGroup((MetadataGroup*)item, page, folder, out, level + 1, c);
			}
		}
		else if (item->IsField())
		{
			DumpField((FieldMetadata*)item, page, out, level + 1, c);
		}
	}
}



void DumpMap(MetadataGroup* group, Page* page, const File& folder, ostream &out, Int level, Int idx)
{
	Expand(out, level);
	out<<group->Name()<<endl;

	NumberField* size = static_cast<NumberField*>(group->FindFirst("SIZE", true));

	Int size0 = size->GetValue(page, 0, false);

	for (Int c = 0;c < group->Size(); c++)
	{
		Metadata* item = group->GetItem(c);
		if (item->IsGroup())
		{
			if (item->Name() == "ITEMS")
			{
				DumpGroup((MetadataGroup*)item, page, folder, out, level + 1, c, size0);
			}
			else {
				DumpGroup((MetadataGroup*)item, page, folder, out, level + 1, c);
			}
		}
		else if (item->IsField())
		{
			DumpField((FieldMetadata*)item, page, out, level + 1, c);
		}
	}
}

void DumpData(MetadataGroup* group, Page* page, const File& folder, ostream &out, Int level, Int idx)
{
	Expand(out, level);
	out<<group->Name()<<endl;

	NumberField* size = static_cast<NumberField*>(group->FindFirst("SIZE", true));
	Int size0 = size->GetValue(page, 0, false);

	for (Int c = 0;c < group->Size(); c++)
	{
		Metadata* item = group->GetItem(c);
		if (item->IsField())
		{
			FieldMetadata* field = (FieldMetadata*)item;
			if (item->Name() == "VALUE" && item->IsNumber())
			{
				NumberField* number = (NumberField*)field;

				out<<endl;
				Expand(out, 24);
				for (int c = 0; c < 32; c++)
				{
					out.width(3);
					out<<hex<<c;
				}
				out<<endl;

				for (Int c = 0; c < size0; c+= 32)
				{
					Expand(out, 12);
					out<<" ";
					out.width(4);
					out<<dec<<c<<" "<<hex;
					out.width(4);
					out<<c<<": ";

					for (Int d = 0; d < 32 && c + d < size0; d++)
					{
						UByte data = number->GetValue(page, c + d, false);
						out<<hex;
						out.width(3);
						out<<(Int)data;
					}

					out<<dec<<endl;
				}
			}
			else {
				DumpField((FieldMetadata*)item, page, out, level + 1, c);
			}
		}
	}
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

	DumpGroup(meta, page, folder, pagetxt, 0, 0);
	DumpTree(meta, page, folder, idx);

	pagetxt.close();
	delete page;
}



int main(int argc, const char** argv, const char** envp)
{

	try {
		//InitTypeSystem(argc, argv, envp, false);
		StreamContainerTypesCollection::Init();

		logger.level() = Logger::TRACE;

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


		manager = new VStreamAllocator();

		cout<<"Load file: "+file.GetPath()<<endl;

//		char cwd_buf[16384];
//		::getcwd(cwd_buf, 16384);
//
//		cout<<"CWD: "<<cwd_buf<<endl;

		LoadFile(manager, file.GetPath().c_str());



		VStreamAllocator::RootMapType* root = manager->roots();
		auto iter = root->Begin();

		while (!iter.IsFlag(memoria::vapi::Iterator::ITER_EOF))
		{
			BigInt  name 	= iter.GetKey(0);
			BigInt  value 	= iter.GetData().value();
			IDValue id(value);

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

		delete manager;
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

