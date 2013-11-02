
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_ALLOCATOR_MVCC_TXNMGR_HPP_
#define MEMORIA_ALLOCATOR_MVCC_TXNMGR_HPP_

#include <memoria/core/tools/pool.hpp>
#include <memoria/core/tools/stream.hpp>
#include <memoria/core/tools/file.hpp>
#include <memoria/core/tools/config.hpp>
#include <memoria/core/tools/assert.hpp>
#include <memoria/core/types/types.hpp>

#include <memoria/core/container/allocator.hpp>
#include <memoria/core/container/page.hpp>
#include <memoria/core/container/container.hpp>

namespace memoria {

template <typename PageType>
class MVCCTxnMgr: public IMVCCTxnMgr<PageType> {
	typedef IMVCCTxnMgr<PageType>												Base;
public:
	typedef IAllocator<PageType>												Allocator;
	typedef typename Base::Txn													Txn;
	typedef typename Base::TxnPtr												TxnPtr;

private:

	Allocator* allocator_;

public:

	MVCCTxnMgr(Allocator* allocator): allocator_(allocator) {}

	virtual ~MVCCTxnMgr() {}

	virtual TxnPtr begin() {

	}
};

}


#endif
