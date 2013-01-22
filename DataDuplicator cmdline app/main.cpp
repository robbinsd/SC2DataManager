/*
	Starcraft 2 Data Duplicator
	Independently developed by Daniel Robbins
	1/21/11

	--Description--
	A desktop application which makes it much less tedious to make mods for the popular
	computer game "StarCraft 2". Each mod (A.K.A. "map") contains a set of XML
	files which define how the map's objects differ from the original game. 
	This application essentially provides an intuitive interface for modifying/adding
	objects to the aforementiond XML files. The user defines a "template", which stores
	XML objects. The user specifies variables in certain XML attributes. Then, the user
	can essentially instantiate an instance following that template, giving values for
	each of the variables. The new instance is added to the map files.
	
	Whenever the user wants to create a bunch of gameplay elements that are similar to 
	an existing	gameplay element, he creates a template from the existing element and
	specifies how each new element differs from the existing one, cutting down on tedious
	error-prone work.

	--Current Libraries Used--
	Boost: filesystem (portable filesystem ops), tokenizer (split string into smaller pieces),
		foreach (easy iteration--java style), regex (perl style regular expression parser).
	PugiXML (fast and easy XML manipulator, using DOM model)

	--Future Plans--
	I eventually plan to release this application as a cross-platform GUI application for
	Windows, Mac, and (hopefully) Linux.
	I will use QT for portable GUI development in C++.
	I also plan on using an expression parsing library, such that templates can contain
	formulas using variables. This would be useful, for example, in calculating a character's
	"Damage per Second" based on his "Damage" variable and his "Attack Speed" variable.
*/

#include <iostream>
#include <windows.h>
#include <direct.h> 
#include "boost/filesystem.hpp"
#include "boost/foreach.hpp"
#include "boost/regex.hpp"
#include "pugixml.hpp"
#include "Template.h"
#include "CustomItem.h"
#include "MapManager.h"
#include "CommonConstants.h"
#include "CustomItemReader.h"
#include "FilesystemUtils.h"
#include "ErrorLogger.h"
using namespace std;
using namespace boost;
namespace fs = boost::filesystem;

boost::regex ARG_REGEX(ARG_FORMAT);

bool ReadArgs(path &mapPath)
{
    mapPath = "";
	const string argsStr = ARGS_FILE.string();
	cout << "Reading program parameters from " << argsStr << endl
        << "{" << endl;
    bool canReadArgs = true;
    if(!exists(ARGS_FILE))
    {
        cout << "Parameters file \"" << ARGS_FILE.string() <<
            "\" does not exist. " << endl;
        canReadArgs = false;
    }
	ifstream fileReader(argsStr.c_str());
	if(canReadArgs && !fileReader.is_open())
    {
        cout << "Parameters file \"" << ARGS_FILE.string() <<
            "\" could not be opened. Output will NOT be copied to a map."
            << endl;
        canReadArgs = false;
    }

    if(canReadArgs && fileReader.eof())
    {
        cout << "Parameters file \"" << ARGS_FILE.string() <<
            "\" is empty. To specify a map to copy into, type \""
            << ARG_MAPPATH_NAME << "=C:/Path/To/Map\"." << endl;
        canReadArgs = false;
    }
    if(canReadArgs)
    {
        while(!fileReader.eof())
        {
            string currentLine;
            std::getline(fileReader, currentLine);
            if(!currentLine.empty())
            {
                boost::match_results<string::const_iterator> matches;
                if(!boost::regex_match(currentLine, matches, ARG_REGEX))
                {
                    cout << "Invalid map path parameter. A valid map path has the form "
                        "\"" << ARG_MAPPATH_NAME << "=Path/To/Map\"." << endl;
                    continue;
                }
                mapPath = string(matches[2].first, matches[2].second);
                if(!mapPath.empty())
                {
                    break;
                }
            }
        }
    }
    if(!mapPath.empty())
    {
        try
        {
            if(!exists(mapPath))
            {
                if(!mapPath.is_complete())
                {
                    mapPath = MAPS_FOLDER/mapPath;
                    if(!exists(mapPath))
                    {
                        ErrorLogger::Log("ERROR: ReadArgs: map path does "
                            "not exist: " + mapPath.string() + ".");
                        return false;
                    }
                }
                else
                {
                    ErrorLogger::Log("ERROR: ReadArgs: map path does not exist: "
                        + mapPath.string() + ".");
                    return false;
                }
            }
        }
        catch(std::exception &e)
        {
            ErrorLogger::Log(string("ERROR: ReadArgs: ") + e.what());
        }
    }
        
    if(mapPath.empty())
    {
        cout << ARG_MAPPATH_NAME << ": NONE. Output will NOT be copied to a map." << endl;
    }
    else
    {
        cout << ARG_MAPPATH_NAME << ": " << mapPath.string() << ". Output will be"
            " copied here." << endl;
    }

    cout << "}" << endl << endl;
	return true;
}

bool BackupMap(const path &mapPath)
{
    try
    {
        if(!exists(BACKUP_FILES_FOLDER))
        {
            create_directory(BACKUP_FILES_FOLDER);
        }
        path backupPath = BACKUP_FILES_FOLDER/mapPath.filename();
        if(exists(backupPath))
        {
            remove_all(backupPath);
        }
        if(!CopyDirectoryAndContents( mapPath, backupPath ))
        {
            ErrorLogger::Log("ERROR: BackupMap: could not backup map.");
            return false;
        }
    }
    catch(std::exception &e)
    {
        ErrorLogger::Log(string("ERROR: BackupMap: ") + e.what() + ".");
        return false;
    }
    return true;
}

bool ClearOutputDirectory ()
{
    try
    {
        if(!exists(OUTPUT_FOLDER))
        {
            create_directory(OUTPUT_FOLDER);
        }
        directory_iterator end_itr;
        for(directory_iterator itr(OUTPUT_FOLDER); itr != end_itr; ++itr)
        {
            if(is_regular_file(itr->path()) && itr->path().extension() == 
                ".xml")
            {
                remove(itr->path());
            }
        }
    }
    catch(std::exception &e)
    {
        ErrorLogger::Log(string("ERROR: ClearOutputDirectory: ") + 
            e.what() + ".");
        return false;
    }
    return true;
}

bool ReadAndCreateCustomItems(MapManager *map=NULL)
{
    CustomItemReader *customItemReader = CustomItemReader::GetInstance();
    if(!customItemReader)
    {
        return false;
    }
    if(fs::is_empty(CUSTOM_ITEMS_FOLDER))
    {
        ErrorLogger::Log("ERROR: ReadAndCreateCustomItems: folder \""
             + CUSTOM_ITEMS_FOLDER.string() + "\" is empty. No items were "
             "created.");
        return false;
    }
    size_t totalNumItemsCreated = 0;
    for(fs::directory_iterator it(CUSTOM_ITEMS_FOLDER); 
        it != fs::directory_iterator(); it++)
    {
        path customItemsPath(it->path());
        vector<ReadCustomItemT *> readCustomItems;
        if(!customItemReader->ReadCustomItems(customItemsPath, readCustomItems))
        {
            return false;
        }
        if(readCustomItems.empty())
        {
            continue;
        }
        string templateForCustomItem(customItemsPath.stem());
        Template templateToUse;
        if(!templateToUse.Create(TEMPLATES_FOLDER/templateForCustomItem))
        {
            return false;
        }
        for(size_t i = 0; i < readCustomItems.size(); i ++)
        {
            CustomItem currentItem;
            if(!currentItem.Create(templateToUse, 
                readCustomItems[i]->varNameToValue))
            {
                return false;
            }
            if(map)
            {
                if(!currentItem.AddToMap(*map))
                {
                    return false;
                }
            }
            if(!currentItem.Output())
            {
                return false;
            }
            ++totalNumItemsCreated;
        }
    }
    if(totalNumItemsCreated > 0)
    {
        cout << endl << "Total number of items created: " 
            << totalNumItemsCreated << endl;
    }
    else
    {
        cout << "WARNING: No items created! Define items in the csv files"
            " of the Custom Items directory." << endl;
    }
    return true;
}

bool Execute()
{
    if(!Template::InitTemplates())
    {
        return false;
    }
    path mapPath;
    if(!ReadArgs(mapPath))
    {
        return false;
    }
    MapManager map;
    if(!mapPath.empty())
    {
        if(!BackupMap(mapPath))
        {
            return false;
        }
        if(!map.Create(mapPath))
        {
            return false;
        }
    }
    if(!ClearOutputDirectory())
    {
        return false;
    }
    if(!mapPath.empty())
    {
        if(!ReadAndCreateCustomItems(&map))
        {
            return false;
        }
        if(!map.Save())
        {
            return false;
        }
    }
    else
    {
        if(!ReadAndCreateCustomItems())
        {
            return false;
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    if(!ErrorLogger::Init())
    {
        return 1;
    }
	if(!Execute())
    {
		cout << endl << "Failed." << endl;
    }
	else 
    {
		cout << endl << "Success!" << endl;
    }
	Sleep(600000);	//give user a chance to see errors or Success message
	return 0;
}