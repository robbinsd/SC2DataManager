#include "MapManager.h"
#include "NodeMatch.h"
#include <iostream>
#include "boost/foreach.hpp"
#include "LoadXML.h"
#include "CommonConstants.h"
#include "ErrorLogger.h"

typedef pair<string, xml_document *> stringXMLDocPair;

//Struct definitions

//TODO: make a recursive method to do this cleaner and avoid using
//GetMatchingNode.
/*struct modifying_walker: pugi::xml_tree_walker
{
    string nodeAttrName;
    xml_node mapDocNode;

    //NOTE: in the future, may want to handle modifying pcdata with child_value()
    virtual bool for_each(pugi::xml_node& templateNode)
    {*/
        /*if(string(templateNode.name()) == "Amount")
        {
            ;
        }
        xml_node mapNode = GetMatchingNode(templateNode, mapDocNode);//, nodeAttrName);
        //if the node does not already exist in the map, create a copy of the templateNode
        if(!mapNode)
        {
            mapNode = GetMatchingNode(templateNode.parent(), mapDocNode);//, nodeAttrName);
            if(mapNode)
                mapNode.append_copy(templateNode);
            return true;
        }
        //if the node exists in the map, modify it
        //iterate over every attribute
        xml_attribute attr = templateNode.first_attribute();
        if(string(templateNode.first_attribute().name()) == nodeAttrName)
        {
            if(string(mapNode.first_attribute().name()) == nodeAttrName)
            {
                attr = attr.next_attribute();
            }
            else
            {
                return false;
            }
        }
        for(; attr; attr = attr.next_attribute())
        {
            xml_attribute mapAttr = mapNode.attribute(attr.name());
            if(mapAttr)
            {
                mapAttr.set_value(attr.value());
            }
            else
            {
                mapNode.append_copy(attr);
            }
        }
        return true; // continue traversal*/
        //return false;
    //}
//};


MapManager::MapManager()
	: mapPath("")
    , mapFilenameToDoc()
	, mapFilenameToWasEdited()
	, hasCreated(false)
{
}

MapManager::~MapManager()
{
	//free everything that we've allocated.
	BOOST_FOREACH(stringXMLDocPair filenameAndDoc, mapFilenameToDoc)
	{
		delete filenameAndDoc.second;
	}
}

bool MapManager::Create(const path &mapPath)
{
	//make sure Create is only called once per instance of MapManager.
	if(hasCreated)
	{
		ErrorLogger::Log("ERROR: MapManager::Create: MapManager \"" + mapPath.string()
			 + "\" has already been created.");
		return false;
	}
    if(!boost::filesystem::exists(mapPath))
    {
		ErrorLogger::Log("ERROR: MapManager::Create: file path \"" + mapPath.string()
		     + "\" does not exist!");
        return false;
    }
	if( !boost::filesystem::is_directory(mapPath) )
	{
		ErrorLogger::Log("ERROR: MapManager::Create: file path(" + mapPath.string()
			 + ") is not a directory!");
        return false;
	}
	if(mapPath.extension() != SC2MAP_EXTENSION)
	{
        ErrorLogger::Log("ERROR: MapManager::Create: file path's extension("
             + mapPath.extension() + ") does not match expected extension(" 
             + SC2MAP_EXTENSION + ").");
        return false;
	}

    this->mapPath = mapPath;
    path gameDataPath = mapPath/GAME_DATA_PATH;
	//if the game data folder exists, read from it.
    if(boost::filesystem::exists(gameDataPath))
    {
		cout << "Reading from map " << gameDataPath << endl 
            << "{" << endl;
		directory_iterator end;
		for(directory_iterator iter(gameDataPath); iter != end; ++iter)
		{
			path currentDataFilePath = iter->path();
			if(currentDataFilePath.extension() != ".xml")
			{
				continue;
			}
			cout << "Reading from map data file " << currentDataFilePath.filename() << "...";
			xml_document *currentDataDoc = new xml_document();
			string currentFilename = currentDataFilePath.filename();
			mapFilenameToDoc[currentFilename] = currentDataDoc;
			string error = LoadXMLFile(currentDataDoc, currentDataFilePath.string().c_str());
			if(error != "")
			{
                cout << endl;
				ErrorLogger::Log(error);
				return false;
			}
			cout << " done." << endl;
		}
		cout << "}" << endl << endl;
	}
	else
	{
        cout << "Warning: MapManager::Create: map(" << mapPath.string() 
			<< ") does not have a Game Data folder, but that's okay." << endl;
    }
	hasCreated = true;
    return true;
}

MapManager::ObjectAddingErrorT MapManager::AddObjectToDataFile(const xml_node &objectToAdd,
                                       const string &fileName, bool overwriteExisting)
{
    if(objectToAdd.empty())
    {
        ErrorLogger::Log("ERROR: MapManager::AddObjectToDataFile: cannot append"
            " an empty objectToAdd.");
        return MapManager::ObjectIsEmpty;
    }
    xml_document *fileDoc = mapFilenameToDoc[fileName];
    if(!fileDoc)
    {
		//if fileDoc doesn't exist, create it.
		fileDoc = new xml_document();
		mapFilenameToDoc[fileName] = fileDoc;
        mapFilenameToWasEdited[fileName] = true;
        /*cerr << "ERROR: MapManager::AddObjectToDataFile: " << fileName << " does not exist for map("
				<< mapPath.string() << ")." << endl;
                << fileName << endl;
        return false;*/
    }
    xml_node docCatalog = fileDoc->child(CATALOG_NAME.c_str());
    if(docCatalog.empty())
    {
		//if catalog doesn't exist, create it.
		docCatalog = fileDoc->append_child(CATALOG_NAME.c_str());
        mapFilenameToWasEdited[fileName] = true;
    }
    //check if the objectToAdd already exists
    const char *objectToAddName = objectToAdd.name();
    xml_node nextExistingObject;
    for(xml_node existingObject = docCatalog.child(objectToAddName); existingObject; 
        existingObject = nextExistingObject)
    {
        nextExistingObject = existingObject.next_sibling(objectToAddName);
        if(strcmp(existingObject.attribute("id").value(), objectToAdd.attribute("id").value()) == 0)
        {
            if(overwriteExisting)
            {
                docCatalog.remove_child(existingObject);
                mapFilenameToWasEdited[fileName] = true;
            }
            else
            {
                return MapManager::ObjectAlreadyExists;
            }
        }
    }
    if( docCatalog.append_copy(objectToAdd).empty() )
    {
        ErrorLogger::Log(string("ERROR: MapManager::AddObjectToDataFile: problem"
            " adding node(") + objectToAdd.name() + ") to file(" + fileName + ")");
        return MapManager::ObjectNotAppendable;
    }
	mapFilenameToWasEdited[fileName] = true;
    return MapManager::NoError;
}


bool MapManager::Save()
{
	//make sure GameData folder and all of its parents exist.
	path currentPath( mapPath );
	BOOST_FOREACH(string dirThatShouldExist, GAME_DATA_PATH)
	{
		currentPath = currentPath/dirThatShouldExist;
		if( !boost::filesystem::exists(currentPath) )
		{
			if( !boost::filesystem::create_directory(currentPath) )
			{
				ErrorLogger::Log("ERROR: MapManager::Save: something went wrong"
                    " while creating non-existent directory(" + currentPath.string() 
                    + ").");
				return false;
			}
		}
	}

    //write the XML files to the map's GameData directory
    BOOST_FOREACH(stringXMLDocPair filenameAndDoc, mapFilenameToDoc)
    {
        string currentFilename = filenameAndDoc.first;
        path mapDataFilePath = mapPath/GAME_DATA_PATH/(currentFilename);
        string mapName = mapPath.leaf();
        if(mapFilenameToWasEdited[filenameAndDoc.first])
        {
            if(!mapFilenameToDoc[currentFilename]->save_file(mapDataFilePath.string().c_str(), "    "))
            {
                ErrorLogger::Log("ERROR: MapManager::Save: Failed to save changes"
                    " to file(" + currentFilename + ") in map(" + mapName + ").");
                return false;
            }
        }
    }
    mapFilenameToWasEdited.clear();
    return true;
}



/*to create a series of custom items and add them to the xml documents of the map, we need to do the following:
 backup the map's GameData folder
  if there's not a folder for this map in the Backup Files folder
   create one
  remove existing Backup Files/GameData folder (if it exists)
  copy the map's current GameData folder into the directory
 read the xml files of the map into xml_documents
 for each custom item we want to create:
  create a template
   read the template xml files into xml_documents
   traverse the tree and find all the variables
  replace each variable with its value, as specified in the custom item interface
  merge the xml_nodes of the modified template with the xml_nodes of the map
   find xml_nodes (children of Catalog) in the map which have the same id attribute values as the xml_nodes in the template.
   if none exist:
    if template is a Modifier:
     return an error stating "#id# is not the id of an existing item. Please try a different id."
    else if template is a Creator:
     for each xml doc in the template:
      for each child node of template Catalog
       add node to children of map Catalog (hopefully this works. if not, we'll have to do deep copying.)
   else if at least one exists:
    if custom item is supposed to be new (i.e. if user clicks "add" button)
     return an error stating "#id# already exists! You can either modify the existing objects or choose a different id."
    else
     for each xml doc in the template:
      for each child node of template Catalog
       add node to children of map Catalog (hopefully this works. if not, we'll have to do deep copying.)
     for each existing map node:
      traverse corresponding template node
       if map has current traversal node (if tag type matches and index value matches, if they have index attr)
        if traversal node
        loop over attributes of traversal node
         if map has current attribute
          set its value to match template attr value
         else
          add template attr as child of map node corresponding to traversal node
       else
        add traversal node as child of map node corresponding to (traversal node.parent)
 for each of the map's xml_documents
  write the xml_document to its path

WARNING: never modify the value of an attribute of name "index". If no node exists with the index attribute value you're looking for,
you'll need to create a node with that index attribute value.
*/
/*
 typedef pair<string, xml_document *> stringXMLDocPair;
 //parse the map's XML files and find the objects we would want to modify
 BOOST_FOREACH(stringXMLDocPair filenameAndDoc, baseItemFilenameToDoc){
  string currentFilename = filenameAndDoc.first;
  path mapDataFilePath = mapPath/GAME_DATA_PATH/currentFilename;
  if(exists(mapDataFilePath)){
   const char *mapDataFilePathCStr = mapDataFilePath.string().c_str();
   xml_document *mapDoc = new xml_document();
   mapFilenameToDoc[currentFilename] = mapDoc;
   pugi::xml_parse_result result = mapDoc->load_file(mapDataFilePathCStr);
   if (!result){
    stringstream output("");
    output << mapDataFilePath << " parsed with errors.\n"
     << "Error description: " << result.description() << "\n"
     << "Error offset: " << result.offset << " (error at [..." << (mapDataFilePathCStr + result.offset) << "]\n\n";
    return output.str();
   }
   xml_node catalog = mapDoc->child(CATALOG_NAME);
   for(xml_node object = catalog.first_child(); object; object = object.next_sibling()){
    if(baseItemFilenameToObjects[currentFilename].count(string(object.name())+" "+object.first_attribute().value())){
     mapFilenameToObjectNodes[currentFilename].insert(object);
    }
   }
  }else{
   //create the xml file and fill it with the xml_document element and catalog element
   xml_document *mapDoc = new xml_document();
   mapFilenameToDoc[currentFilename] = mapDoc;
   mapDoc->append_child(CATALOG_NAME);
   //mapFilenameToObjectNodes[currentFilename];
  }
 }*/


//------------------ GETTERS -----------------
void MapManager::GetDataFileNames(vector<string> &fileNames) const
{
    BOOST_FOREACH(stringXMLDocPair fileNameAndDoc, mapFilenameToDoc)
    {
        fileNames.push_back(fileNameAndDoc.first);
    }
}

void MapManager::GetObjectsInDataFile(string fileName, vector<const xml_node> &objects) const
{
    objects.clear();
    unordered_map<string,xml_document*>::const_iterator itr = mapFilenameToDoc.find(fileName);
    if(itr == mapFilenameToDoc.end())
    {
        ErrorLogger::Log("ERROR: MapManager::GetObjectsInDataFile: file(" + fileName
             + ") does not exist in map(" + mapPath.string() + ").");
        return;
    }
    xml_document *doc = itr->second;
    xml_node catalog = doc->child(CATALOG_NAME.c_str());
	if(catalog.empty())
	{
		//cout << "Unable to find catalog. This means no objects are in data file." << endl;
		return;
	}
    for(xml_node object = catalog.first_child(); object; object = object.next_sibling())
    {
        objects.push_back(object);
    }
}
