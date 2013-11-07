
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_DUMP_HPP
#define MEMORIA_TESTS_DUMP_HPP

#include <memoria/allocators/file/factory.hpp>
#include <memoria/allocators/inmem/factory.hpp>

typedef memoria::SmallInMemAllocator VStreamAllocator;

void LoadFile(VStreamAllocator& allocator, const char* file)
{
    FileInputStreamHandler* in = FileInputStreamHandler::create(file);
    allocator.load(in);
    delete in;
}

struct NamedIDValue {
    const char*     name;
    Int             index;
    Int             count;
    IDValue         id[10];
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

    virtual void startPage(const char* name) {}
    virtual void endPage() {}

    virtual void startLine(const char* name, Int size = -1)
    {
        line_ = true;
        name_ = name;
    }

    virtual void endLine() {
        line_ = false;
        idx_++;
    }

    virtual void startGroup(const char* name, Int elements = -1)
    {
        name_ = name;
        idx_ = 0;
    }

    virtual void endGroup() {}

    virtual void value(const char* name, const Byte* value, Int count = 1, Int kind = 0)        {}
    virtual void value(const char* name, const UByte* value, Int count = 1, Int kind = 0)       {}
    virtual void value(const char* name, const Short* value, Int count = 1, Int kind = 0)       {}
    virtual void value(const char* name, const UShort* value, Int count = 1, Int kind = 0)      {}
    virtual void value(const char* name, const Int* value, Int count = 1, Int kind = 0)         {}
    virtual void value(const char* name, const UInt* value, Int count = 1, Int kind = 0)        {}
    virtual void value(const char* name, const BigInt* value, Int count = 1, Int kind = 0)      {}
    virtual void value(const char* name, const UBigInt* value, Int count = 1, Int kind = 0)     {}

    virtual void value(const char* name, const IDValue* value, Int count = 1, Int kind = 0)
    {
    	if (strcmp(name, "GID") != 0)
    	{
    		NamedIDValue entry;

    		entry.count = count;
    		entry.name  = name_;
    		entry.index = idx_;

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
    }

    virtual void symbols(const char* name, const UBigInt* value, Int count, Int bits_per_symbol) {}
    virtual void symbols(const char* name, const UByte* value, Int count, Int bits_per_symbol) {}

    const IDValueVector& values() const
    {
        return values_;
    }
};


template <typename Allocator>
void dumpTree(
		const IDValue& id, 
		const File& folder, 
		std::set<IDValue>& processed,
		Allocator* dumpmanager
);




template <typename Allocator, typename Page>
void dumpTree(
		PageMetadata* group, 
		Page& page, 
		const File& folder, 
		std::set<IDValue>& processed,
		Allocator* manager
	)
{
    IDSelector selector;

    group->getPageOperations()->generateDataEvents(page.page(), DataEventsParams(), &selector);

    for (const NamedIDValue& entry: selector.values())
    {
        for (Int c = 0; c < entry.count; c++)
        {
            const IDValue& id = entry.id[c];

            IDValue idv = id;
            if ((!idv.isNull()) && processed.find(id) == processed.end())
            {
                stringstream str;

                str<<entry.name<<"-";

                char prev = str.fill();

                str.fill('0');
                str.width(4);

                str<<entry.index;

                str.fill(prev);

                if (entry.count > 1)
                {
                    str<<"."<<c;
                }

                str<<"___"<<id;

                File folder2(folder.getPath() + Platform::getFilePathSeparator() + str.str());
                folder2.mkDirs();

                dumpTree(id, folder2, processed, manager);
            }
        }
    }
}

template <typename Allocator>
void dumpTree(
		const IDValue& id, 
		const File& folder, 
		std::set<IDValue>& processed, 
		Allocator* allocator
	)
{
    processed.insert(id);

    try {
    	auto page = allocator->getPage(id, Allocator::READ, -1);
    	
        ofstream pagetxt((folder.getPath() + Platform::getFilePathSeparator() + "page.txt").c_str());

        PageMetadata* meta = allocator->getMetadata()->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

        dumpPageData(meta, page, pagetxt);

        dumpTree(meta, page, folder, processed, allocator);

        pagetxt.close();
    }
    catch (NullPointerException e)
    {
        cout<<"NullPointerException: "<<e.message()<<" at "<<e.source()<<endl;
    }
}


String getPath(String dump_name)
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

template <typename Allocator>
void DumpAllocator(Allocator& allocator, File& path) 
{
	auto* root = allocator.roots();
	auto iter = root->Begin();

	while (!iter.isEnd())
	{
		BigInt name    	= iter.key();
		BigInt value   	= iter.getValue();

		IDValue id(value);

		cout<<"dumping name="<<name<<" root="<<id<<endl;

		stringstream str;
		str<<name<<"___"<<id;

		File folder(path.getPath() + "/" + str.str());

		if (folder.isExists())
		{
			if (!folder.delTree())
			{
				throw Exception("dump.cpp", SBuf()<<"can't remove file "<<folder.getPath());
			}
		}

		folder.mkDirs();

		std::set<IDValue> processed;

		dumpTree(id, folder, processed, &allocator);

		iter.next();
	}
}

Int DumpAllocator(String file_name)
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

        if (!path.isExists())
        {
            path.mkDirs();
        }
        else {
            path.delTree();
            path.mkDirs();
        }
        
        Int status = GenericFileAllocator::testFile(file.getPath());  
        
        if (status == 7) 
        {
        	cout<<"Load FileAllocator file: "+file.getPath()<<endl;
        	GenericFileAllocator allocator(file.getPath(), OpenMode::READ);
        	
        	if (allocator.properties().isMVCC())
        	{
        		cout<<"Dump MVCCV FileAllocator"<<endl;
        		typedef MVCCAllocator<FileProfile<>, GenericFileAllocator::Page> TxnMgr;

        		TxnMgr::initMetadata();

        		TxnMgr mvcc_allocator(&allocator);

        		DumpAllocator(mvcc_allocator, path);
        	}
        	else {
        		DumpAllocator(allocator, path);
        	}
        }
        else {
        	VStreamAllocator allocator;
        	
        	cout<<"Load InMemAllocator file: "+file.getPath()<<endl;

        	LoadFile(allocator, file.getPath().c_str());

        	DumpAllocator(allocator, path);
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


#endif
