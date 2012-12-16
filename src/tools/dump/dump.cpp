
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)





#include <memoria/memoria.hpp>

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

    const IDValueVector& values() const
    {
        return values_;
    }
};


void dumpTree(const IDValue& id, const File& folder);

void dumpTree(PageMetadata* group, Page* page, const File& folder)
{
    IDSelector selector;

    group->getPageOperations()->generateDataEvents(page->Ptr(), DataEventsParams(), &selector);

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

                dumpTree(id, folder2);
            }
        }
    }
}

void dumpTree(const IDValue& id, const File& folder)
{
    processed.insert(id);
    Page* page = manager->createPageWrapper();

//  FIXME: IDValue

    try {
        manager->getPage(page, id);

        ofstream pagebin((folder.getPath() + Platform::getFilePathSeparator() + "page.bin").c_str());
        for (Int c = 0; c < page->size(); c++)
        {
            pagebin<<(Byte)page->getByte(c);
        }
        pagebin.close();

        ofstream pagetxt((folder.getPath() + Platform::getFilePathSeparator() + "page.txt").c_str());

        PageMetadata* meta = manager->getMetadata()->getPageMetadata(page->getContainerHash(), page->getPageTypeHash());

        dumpPage(meta, page, pagetxt);

        dumpTree(meta, page, folder);

        pagetxt.close();
    }
    catch (NullPointerException e)
    {
        cout<<"NullPointerException: "<<e.message()<<" at "<<e.source()<<endl;
    }

    delete page;
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


int main(int argc, const char** argv, const char** envp)
{
    MetadataInitializer<SmallProfile<> >::init();

    try {
        logger.level() = Logger::NONE;

        if (argc != 3 && argc != 2)
        {
            cerr<<"Usage: dump file.dump [/path/to/folder/to/dump/into]"<<endl;
            return 1;
        }

        File file(argv[1]);
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

        File path(argc == 3 ? String(argv[2]) : getPath(argv[1]));
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


        VStreamAllocator allocator;
        manager = &allocator;

        cout<<"Load file: "+file.getPath()<<endl;

        LoadFile(allocator, file.getPath().c_str());

        VStreamAllocator::RootMapType* root = manager->roots();
        auto iter = root->Begin();

        while (!iter.isEnd())
        {
            BigInt  name    = iter.getKey(0);

            BigInt  value   = iter.getValue();
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

            dumpTree(id, folder);

            iter.next();
        }
    }
    catch (Exception ex) {
        cout<<"Exception "<<ex.source()<<" "<<ex<<endl;
    }
    catch (MemoriaThrowable *ex) {
        cout<<"Exception* "<<ex->source()<<" "<<*ex<<endl;
    }
    catch (MemoriaThrowable ex) {
    	cout<<"Exception "<<ex.source()<<" "<<ex<<endl;
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

