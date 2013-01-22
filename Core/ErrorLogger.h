#ifndef _ERROR_LOGGER_H_
#define _ERROR_LOGGER_H_

#include <sstream>
#include "CommonConstants.h"
using namespace std;

namespace ErrorLogger
{
    bool Init();
//class ErrorLogger
//{
//public:
 //   ErrorLogger();

    void Log(const string &errorMsg);

//};
}

#endif //_ERROR_LOGGER_H_