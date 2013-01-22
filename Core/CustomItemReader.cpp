#include "CustomItemReader.h"
#include "boost/foreach.hpp"
#include "boost/regex.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/tokenizer.hpp"
#include "boost/filesystem.hpp"
#include <iostream>
#include <fstream>
#include "CommonConstants.h"
#include "ErrorLogger.h"
using namespace std;

CustomItemReader *CustomItemReader::_instance = NULL;

CustomItemReader *CustomItemReader::GetInstance()
{
    if(!_instance)
    {
        _instance = new CustomItemReader;
    }
    return _instance;
}

CustomItemReader::CustomItemReader()
{
}

void GetRowContents(const string &rowStr, vector<string> &contents)
{
	boost::char_separator<char> sep(CUSTOM_ITEMS_COL_DELIM.c_str(), "", boost::keep_empty_tokens);
	boost::tokenizer<boost::char_separator<char> > tokens(rowStr,sep);
	BOOST_FOREACH(string token, tokens)
    {
		contents.push_back(token);
    }
}

//read in the new custom items for the given map, using the given template.
namespace fs = boost::filesystem;
bool CustomItemReader::ReadCustomItems(const fs::path &customItemsPath,
    vector<ReadCustomItemT *> &readItems)
{
    if(!fs::exists(customItemsPath))
    {
        ErrorLogger::Log("ERROR: CustomItemReader::ReadCustomItems: no custom items "
            "file exists at path: " + customItemsPath.string() + ".");
        return false;
    }
	const char *customItemsCStr = customItemsPath.string().c_str();
    readItems.clear();
	ifstream fileReader(customItemsCStr, ios::in);
	if(!fileReader.is_open())
    {
        cout << endl;
		ErrorLogger::Log(string("ERROR: ReadCustomItems: failed to open ") 
            + customItemsCStr);
        return false;
    }
	else if(fileReader.eof())
    {
        //cout << endl;
		//ErrorLogger::Log(string("ERROR: ReadCustomItems: ") + customItemsCStr 
        //    + " was empty.");
        //return false;
        return true;
    }
	//cout << "reading " << customItemsCStr << "..." << endl;
    string currentLine;
	std::getline(fileReader, currentLine);
	if(fileReader.eof())
    {
        //cout << endl;
		//ErrorLogger::Log(string("ERROR: ReadCustomItems: ") + customItemsCStr + 
        //    " was empty.");
		//return false;
        return true;
    }
	vector<string> headerVars;
	GetRowContents(currentLine, headerVars);
	int headerVarsSize = headerVars.size();
	while(!fileReader.eof())
	{
		std::getline(fileReader, currentLine);
		if(currentLine.empty())
        {
            continue;
        }
		vector<string> rowContents;
		GetRowContents(currentLine, rowContents);
		ReadCustomItemT *newItem = new ReadCustomItemT;
		for(int i = 0; i < headerVarsSize; ++i)
        {
			newItem->varNameToValue[headerVars[i]] = rowContents[i];
        }
		readItems.push_back(newItem);
	}
	return true; 
}
