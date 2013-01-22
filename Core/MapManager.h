// MapManager.h
#ifndef __MAPMANAGER_H__
#define __MAPMANAGER_H__


#include "pugixml.hpp"
#include "boost/unordered_map.hpp"
#include <string>
#include <map>
#include "boost/filesystem.hpp"
#include "CustomItem.h"
using namespace std;
using namespace pugi;
using namespace boost::filesystem;

class MapManager
{
    friend class CustomItem;
public:
	//--------DUMMY CONSTRUCTOR (does nothing) -----
    MapManager();

	//-------ACTUAL CONSTRUCTOR --------
	//There are a number of ways that the MapManager can fail to initialize.
	// It's not possible to return anything from the constructor, nor will
	// throwing errors work as desired. For more control, we use the Create
	// function in place of the normal constructor.

    //Read the map from the specified path
    bool Create(const path &mapPath );

	//Destroys all state held by the map manager.
	~MapManager();

    //--------------- SETTERS/MODIFIERS -----------

    enum ObjectAddingErrorT
    {
        ObjectIsEmpty,
        ObjectNotAppendable,
        ObjectAlreadyExists,
        NoError
    };
	/*
	 * Adds object to the catalog of file fileName. If fileName does not
	 * exist, it is created. If overwriteExisting is true, any existing object
     * that conflicts with the given object is overwritten.
	 */
    ObjectAddingErrorT AddObjectToDataFile(const xml_node &object, const string &fileName,
        bool overwriteExisting=false);

    //Merge the XML trees of the map with those of the CustomItem.
    //string MergeWithCustomItem(const CustomItem &item);

    bool Save();

    //---------------- GETTERS -----------------
    // Fill "fileNames" vector with the names of all the files
    // that this MapManager instance manages.
    void GetDataFileNames(vector<string> &fileNames) const;

    // Fill "objects" vector with all the xml_nodes contained in
    // the catalog of the given "fileName". i.e. the nodes on the
    // top of the hierarchy.
    void GetObjectsInDataFile(string fileName, vector<const xml_node> &objects) const;

    path GetPath() const
    {
        return mapPath;
    }

private /*dummy methods*/:
    //non-copyable semantics
    MapManager(const MapManager &other);
    const MapManager& operator=(const MapManager&);

private /*methods*/:
	
private /*variables*/:
    path mapPath;
    unordered_map<string, xml_document *> mapFilenameToDoc;
    unordered_map<string, bool> mapFilenameToWasEdited;
	bool hasCreated;
};


#endif // __MAPMANAGER_H__
