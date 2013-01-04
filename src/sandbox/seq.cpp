

#include <memoria/memoria.hpp>

#include <memoria/core/pmap/packed_seq.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>

using namespace std;
using namespace memoria;
using namespace memoria::vapi;


int main(void) {

	typedef PackedSeq<PackedSeqTypes<>> Seq;

	try {
		Byte buffer[4096];

		memset(buffer, 0, sizeof(buffer));

		Seq* seq = T2T<Seq*>(buffer);

		seq->initByBlock(sizeof(buffer) - sizeof(Seq));

		seq->size() = seq->maxSize();
		seq->reindex();

		cout<<seq->indexSize()<<endl;
		cout<<seq->maxSize()<<endl;
	}
	catch (Exception ex) {
		cout<<"Exception: "<<ex.source()<<": "<<ex.message()<<endl;
	}

    return 0;
}

