/*
 * tools.hpp
 *
 *  Created on: 17.02.2011
 *      Author: Victor
 */

#ifndef MEMOTIA_TESTS_MTOOLS_HPP_
#define MEMOTIA_TESTS_MTOOLS_HPP_

#include <memoria/allocators/stream/factory.hpp>

static bool check(memoria::DefaultStreamAllocator &allocator)
{
	memoria::StreamContainersChecker checker(allocator);
	return checker.CheckAll();
}


#endif
