#include "ErrorLogger.h"
#include <iostream>
#include <fstream>

static ofstream logFileWriter;
static bool hasInited = false;

bool ErrorLogger::Init()
{
    logFileWriter.open(ERROR_LOG_FILE.string(), ios_base::trunc);
    hasInited = logFileWriter.is_open();
    return hasInited;
}

void ErrorLogger::Log(const string &errorMsg)
{
    if(!hasInited)
    {
        cerr << "ERROR: LogError: logging system has not been initialized."
            << endl;
        return;
    }
    logFileWriter << errorMsg << endl;
    cerr << errorMsg << endl;
    logFileWriter.close();
}