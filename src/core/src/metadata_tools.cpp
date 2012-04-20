
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

class TextPageDumper: public IPageDataEventHandler {
	std::ostream& out_;

	Int level_;
	Int cnt_;

	bool line_;

public:
	TextPageDumper(std::ostream& out): out_(out), level_(0), cnt_(0), line_(false) {}
	virtual ~TextPageDumper() {}

	virtual void StartPage(const char* name)
	{
		out_<<name<<endl;
		level_++;
	}

	virtual void EndPage()
	{
		out_<<endl;
		level_--;
	}

	virtual void StartGroup(const char* name, Int elements = -1)
	{
		cnt_ = 0;
		Expand(out_, level_++);

		out_<<name;

		if (elements >= 0)
		{
			out_<<": "<<elements;
		}

		out_<<endl;
	};

	virtual void EndGroup()
	{
		level_--;
	}


	virtual void StartLine(const char* name, Int size = -1)
	{
		DumpLineHeader(out_, level_, cnt_++, name);
		line_ = true;
	}

	virtual void EndLine()
	{
		line_ = false;
		out_<<endl;
	}



	virtual void Value(const char* name, const Byte* value, Int count = 1, Int kind = 0)
	{
		if (kind == BYTE_ARRAY)
		{
			DumpByteArray("DATA", value, count);
		}
		else {
			OutNumber(name, value, count, kind);
		}
	}

	virtual void Value(const char* name, const UByte* value, Int count = 1, Int kind = 0)
	{
		OutNumber(name, value, count, kind);
	}

	virtual void Value(const char* name, const Short* value, Int count = 1, Int kind = 0)
	{
		OutNumber(name, value, count, kind);
	}


	virtual void Value(const char* name, const UShort* value, Int count = 1, Int kind = 0)
	{
		OutNumber(name, value, count, kind);
	}

	virtual void Value(const char* name, const Int* value, Int count = 1, Int kind = 0)
	{
		OutNumber(name, value, count, kind);
	}


	virtual void Value(const char* name, const UInt* value, Int count = 1, Int kind = 0)
	{
		OutNumber(name, value, count, kind);
	}

	virtual void Value(const char* name, const BigInt* value, Int count = 1, Int kind = 0)
	{
		OutNumber(name, value, count, kind);
	}

	virtual void Value(const char* name, const UBigInt* value, Int count = 1, Int kind = 0)
	{
		OutNumber(name, value, count, kind);
	}



	virtual void Value(const char* name, const IDValue* value, Int count = 1, Int kind = 0)
	{
		if (!line_)
		{
			DumpFieldHeader(out_, level_, cnt_++, name);
		}
		else {
			out_<<"    "<<name<<" ";
		}

		for (Int c = 0; c < count; c++)
		{
			out_<<*value;

			if (c < count - 1)
			{
				out_<<", ";
			}
		}

		if (!line_)
		{
			out_<<endl;
		}
	}

private:



	void DumpFieldHeader(ostream &out, Int level, Int idx, StringRef name)
	{
		stringstream str;
		Expand(str, level);
		str<<"FIELD: ";
		str<<idx<<" "<<name;

		int size = str.str().size();
		Expand(str, 30 - size);
		out<<str.str();
	}

	void DumpLineHeader(ostream &out, Int level, Int idx, StringRef name)
	{
		stringstream str;
		Expand(str, level);
		str<<name<<": ";
		str<<idx<<" ";

		int size = str.str().size();
		Expand(str, 15 - size);
		out<<str.str();
	}


	void DumpByteArray(const char* name, const Byte* data, Int count)
	{
		out_<<endl;
		Expand(out_, 24);
		for (int c = 0; c < 32; c++)
		{
			out_.width(3);
			out_<<hex<<c;
		}
		out_<<endl;

		for (Int c = 0; c < count; c+= 32)
		{
			Expand(out_, 12);
			out_<<" ";
			out_.width(4);
			out_<<dec<<c<<" "<<hex;
			out_.width(4);
			out_<<c<<": ";

			for (Int d = 0; d < 32 && c + d < count; d++)
			{
				UByte udata = data[c + d];
				out_<<hex;
				out_.width(3);
				out_<<(Int)udata;
			}

			out_<<dec<<endl;
		}
	}


	template <typename T>
	void OutNumber(const char* name, const T* value, Int count, Int kind)
	{
		if (!line_)
		{
			DumpFieldHeader(out_, level_, cnt_++, name);
		}
		else {
			out_<<"    "<<name<<" ";
		}

		for (Int c = 0; c < count; c++)
		{
			out_.width(12);
			out_<<*value;

			if (c < count - 1)
			{
				out_<<",";
			}

			out_<<" ";
		}

		if (!line_)
		{
			out_<<endl;
		}
	}
};



void DumpPage(PageMetadata* meta, Page* page, std::ostream& out)
{
	TextPageDumper dumper(out);

	meta->GetPageOperations()->GenerateDataEvents(page->Ptr(), DataEventsParams(), &dumper);
}


}}


