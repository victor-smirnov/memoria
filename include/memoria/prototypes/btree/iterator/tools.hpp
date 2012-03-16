
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BTREE_ITERATOR_TOOLS_H
#define __MEMORIA_PROTOTYPES_BTREE_ITERATOR_TOOLS_H

#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/prototypes/btree/names.hpp>

namespace memoria    {

using namespace memoria::btree;

MEMORIA_ITERATOR_PART_BEGIN(memoria::btree::IteratorToolsName)


    typedef typename Base::Container                                                Container;
	typedef typename Container::Accumulator                                  		Accumulator;
	typedef typename Container::Types::NodeBase                                     NodeBase;
	typedef typename Container::Types::NodeBaseG                                    NodeBaseG;

    typedef typename Container::Key                                                 Key;
    typedef typename Container::Value                                               Value;

    typedef typename Container::Types::TreePath                                     TreePath;


    Value GetData() const
    {
        return me()->model().GetLeafData(me()->page(), me()->key_idx());
    }

    Key GetKey(Int keyNum) const
    {
        return Container::GetKey(me()->page(), keyNum, me()->key_idx());
    }

    Accumulator GetKeys() const
    {
    	Accumulator accum;
    	Container::GetKeys(me()->page(), me()->key_idx(), accum);
    	return accum;
    }

    void Dump(ostream& out = cout, const char* header = NULL)
    {
    	out<<(header != NULL ? header : me()->GetDumpHeader())<<endl;

    	me()->DumpKeys(out);

    	me()->DumpBeforePath(out);
    	me()->DumpPath(out);

    	me()->DumpBeforePages(out);
    	me()->DumpPages(out);
    }

    String GetDumpHeader()
    {
    	return String(me()->model().type_name()) + " Iterator State";
    }

    void DumpPath(ostream& out)
    {
    	out<<"Path:"<<endl;

    	TreePath& path0 = me()->path();
    	for (int c = me()->path().GetSize() - 1; c >= 0; c--)
    	{
    		out<<"Node("<<c<<"): "<<IDValue(path0[c]->id())<<" idx="<<(c > 0 ? ToString(path0[c - 1].parent_idx()) : "")<<endl;
    	}
    }

    void DumpKeys(ostream& out)
    {
    	out<<"KeyIdx:  "<<me()->key_idx()<<endl;
    }

    void DumpBeforePath(ostream& out){}
    void DumpBeforePages(ostream& out){}

    void DumpPages(ostream& out)
    {
    	me()->model().Dump(me()->leaf().node(), out);
    }

MEMORIA_ITERATOR_PART_END

}

#endif
