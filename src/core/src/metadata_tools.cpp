
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/metadata/tools.hpp>

#include <sstream>
#include <string>

namespace memoria {namespace vapi    {

using namespace std;

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


void DumpGroup(MetadataGroup* group, Page* page, ostream &out, Int level, Int idx, Int size)
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
				DumpMap((MetadataGroup*)item, page, out, level + 1, c);
			}
			else if (item->Name() == "DATA")
			{
				DumpData((MetadataGroup*)item, page, out, level + 1, c);
			}
			else {
				DumpGroup((MetadataGroup*)item, page, out, level + 1, c);
			}
		}
		else if (item->IsField())
		{
			DumpField((FieldMetadata*)item, page, out, level + 1, c);
		}
	}
}



void DumpMap(MetadataGroup* group, Page* page, ostream &out, Int level, Int idx)
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
				DumpGroup((MetadataGroup*)item, page, out, level + 1, c, size0);
			}
			else {
				DumpGroup((MetadataGroup*)item, page, out, level + 1, c);
			}
		}
		else if (item->IsField())
		{
			DumpField((FieldMetadata*)item, page, out, level + 1, c);
		}
	}
}

void DumpData(MetadataGroup* group, Page* page, ostream &out, Int level, Int idx)
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



void DumpPage(PageMetadata* meta, Page* page, std::ostream& out)
{
	DumpGroup(meta, page, out, 0, 0);
}


}}


