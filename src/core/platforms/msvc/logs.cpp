#include <memoria/v1/vapi/models/logs.hpp>



namespace memoria {

const char* ExtractFunctionName(const char* full_name)
{
    const char* start = NULL;

    for (const char* tmp = full_name; *tmp != 0; tmp++)
    {
        if (*tmp == ':')
        {
            start = tmp;
        }
    }

    if (start == NULL) {
        start = full_name;
    }
    else {
        start++;
    }

    return start;
}

}

