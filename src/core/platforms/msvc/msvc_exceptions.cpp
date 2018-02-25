
// Copyright 2018 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <memoria\v1\core\exceptions\exceptions.hpp>

#include <Windows.h>
#include <WinBase.h>

namespace memoria {
namespace v1 {

SystemException::SystemException() 
{
	DWORD last_error = ::GetLastError();
	(*this) << SystemErrorCodeInfo(last_error) << SystemErrorStrInfo(GetErrorMessage(last_error));
}

SystemException::SystemException(DWORD error_code) 
{
	(*this) << SystemErrorCodeInfo(error_code) << SystemErrorStrInfo(GetErrorMessage(error_code));
}


std::string GetErrorMessage(DWORD error_code)
{
	void* cstr;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		error_code,
		MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
		(LPTSTR)&cstr,
		0,
		NULL
	);

	std::string str((char*)cstr);
	LocalFree(cstr);

	return str;
}

}
}