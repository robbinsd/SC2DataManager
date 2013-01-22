#ifndef _COMMON_SC2_CONSTANTS_H_
#define _COMMON_SC2_CONSTANTS_H_

#include "boost/filesystem/path.hpp"
using namespace boost::filesystem;
using namespace std;

static const string OBJECT_ID_NAME ("id");
static const string CATALOG_NAME ("Catalog");
static const path GAME_DATA_PATH ("Base.SC2Data/GameData");
static const string SC2MAP_EXTENSION(".SC2Map");

//for data duplicator
static const path WORKING_DIRECTORY		("");//Data Files");
static const path TEMPLATES_FOLDER		(WORKING_DIRECTORY/"Templates");
static const path CUSTOM_ITEMS_FOLDER	(WORKING_DIRECTORY/"Custom Items");
static const path BACKUP_FILES_FOLDER	(WORKING_DIRECTORY/"Backup Files");
static const path MAPS_FOLDER       	(WORKING_DIRECTORY/"Maps");
static const path OUTPUT_FOLDER         (WORKING_DIRECTORY/"Output");
static const path ARGS_FILE				(WORKING_DIRECTORY/"parameters.txt");
static const path ERROR_LOG_FILE        (WORKING_DIRECTORY/"errorLog.txt");
static const path README_FILE           (WORKING_DIRECTORY/"README.txt");
static const path EXECUTABLE_FILE       ("SC2DataManager.exe");

static const string ARG_MAPPATH_NAME    ("MapPath");
static const string ARG_MAPPATH_DELIM   ("=");
static const string ARG_FORMAT          ("("+ARG_MAPPATH_NAME+")"+
                                            ARG_MAPPATH_DELIM+"(.*)");
static const string CUSTOM_ITEMS_COL_DELIM(",");

#endif