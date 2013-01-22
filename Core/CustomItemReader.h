#ifndef _CUSTOM_ITEM_READER_H_
#define _CUSTOM_ITEM_READER_H_

#include <string>
#include <vector>
#include <map>
#include "boost/filesystem.hpp"
using namespace std;

struct ReadCustomItemT
{
    std::map<string, string> varNameToValue;
};

class CustomItemReader
{
public:
    static CustomItemReader* GetInstance();
    bool ReadCustomItems(const boost::filesystem::path &customItemsPath,
        vector<ReadCustomItemT *> &readItems);

private:
    //This is a singleton class. disallow these operations.
    CustomItemReader();
    CustomItemReader(CustomItemReader const&);
    CustomItemReader& operator=(CustomItemReader const&);

    //if there is i.e. a mapping with name "name" and value "{1-6}", 
    //it is replaced by 6 name/value mappings: "name/1", "name/2", ... , "name/6". 
    //bool ExpandVariableRanges( std::vector<ReadCustomItemT *> &readItems );

    static CustomItemReader *_instance;
};

#endif //_CUSTOM_ITEM_READER_H_