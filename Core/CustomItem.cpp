#include "CustomItem.h"
#include "boost/foreach.hpp"
#include <sstream>
#include <iostream>
#include "CommonConstants.h"
#include "NodeMatch.h"
#include "MapManager.h"
#include "ErrorLogger.h"


enum ObjectRequiredAgeT
{
    OLD = 0,
    NEW,
    EITHER
};

enum ObjectOldAgeActionT
{
    MODIFY = 0,
    OVERWRITE,
    DO_NOTHING
};

static const ObjectRequiredAgeT DEFAULT_OBJECT_REQUIRED_AGE = NEW;
static const ObjectOldAgeActionT DEFAULT_OBJECT_OLD_AGE_ACTION = DO_NOTHING;

typedef pair<string, xml_document *> stringXMLDocPair;
bool CustomItem::Create(const Template &baseItemTemplate, const map<string,string> &varNameToValue)
{
    return baseItemTemplate.Instantiate(varNameToValue, _itemData);
}

string CustomItem::GetId() const
{ 
    return _itemData._id;	
}

void CustomItem::GetVariableData(VariableDataMap &varNameToVarData) const
{
    BOOST_FOREACH(StringVarDataPair varNameAndData, _itemData._variableNameToVariableData)
    {
        varNameToVarData[varNameAndData.first] = varNameAndData.second;
        varNameToVarData[varNameAndData.first].forEachNode = xml_node();
    }
}

ObjectRequiredAgeT ObjectGetRequiredAge(xml_node object)
{
    string requiredAgeStr = object.attribute(OBJECT_REQUIRED_AGE_ATTR_NAME.c_str()).value();
    //the attr is only useful to this application. remove it now that we've read it.
    object.remove_attribute(OBJECT_REQUIRED_AGE_ATTR_NAME.c_str());
    for(size_t i = 0; i < sizeof(OBJECT_REQUIRED_AGE_ATTR_VALUES)/sizeof(char *); ++i)
    {
        if(requiredAgeStr == OBJECT_REQUIRED_AGE_ATTR_VALUES[i])
        {
            return (ObjectRequiredAgeT) i;
        }
    }
    return DEFAULT_OBJECT_REQUIRED_AGE;
}

ObjectOldAgeActionT ObjectGetOldAgeAction(xml_node object)
{
    string oldAgeActionStr = object.attribute(OBJECT_OLD_AGE_ACTION_ATTR_NAME.c_str()).value();
    //the attr is only useful to this application. remove it now that we've read it.
    object.remove_attribute(OBJECT_OLD_AGE_ACTION_ATTR_NAME.c_str());
    for(size_t i = 0; i < sizeof(OBJECT_OLD_AGE_ACTION_ATTR_VALUES)/sizeof(char *); ++i)
    {
        if(oldAgeActionStr == OBJECT_OLD_AGE_ACTION_ATTR_VALUES[i])
        {
            return (ObjectOldAgeActionT) i;
        }
    }
    return DEFAULT_OBJECT_OLD_AGE_ACTION;
}

void ModifyNodeUsingValuesFromNewNode(xml_node node, const xml_node &newNode)
{
    //replace all of node's attributes with newNode's attributes.
    while(node.first_attribute())
    {
        node.remove_attribute(node.first_attribute());
    }
    for(xml_attribute newAttr = newNode.first_attribute(); newAttr; newAttr = 
        newAttr.next_attribute())
    {
        node.append_copy(newAttr);
    }

    //recursively modify children.
    for(xml_node newChildNode = newNode.first_child(); newChildNode; newChildNode = 
        newChildNode.next_sibling())
    {
        xml_node matchingOldChildNode = GetMatchingNode(newChildNode, node);
        if(matchingOldChildNode)
        {
            ModifyNodeUsingValuesFromNewNode(matchingOldChildNode, newChildNode);
        }
        else
        {
            //since the existing node does not have newChildNode, we need to add it.
            node.append_copy(newChildNode);
        }
    }
}

//TODO: templates need to be able to specify which objects are adding onto existing objects, and which should be new objects.
bool CustomItem::AddToMap(MapManager &mapManager) const
{
    cout << "Merging map " << mapManager.mapPath.filename() << " with CustomItem " << _itemData._id << "...";
    const map<string, xml_document *> &customItemFilenameToDoc = _itemData._itemFilenameToDoc;
    unordered_map<string, xml_document *> &mapFilenameToDoc = mapManager.mapFilenameToDoc;
    unordered_map<string, bool> &mapFilenameToWasEdited = mapManager.mapFilenameToWasEdited;

    BOOST_FOREACH(stringXMLDocPair filenameAndDoc, customItemFilenameToDoc)
    {
        string currentFilename = filenameAndDoc.first;
        if(mapFilenameToDoc.find(currentFilename) == mapFilenameToDoc.end())
        {
            xml_document *mapDoc = new xml_document();
            mapFilenameToDoc[currentFilename] = mapDoc;
            mapFilenameToWasEdited[currentFilename] = true;
        }
        xml_node mapCatalog = mapFilenameToDoc[currentFilename]->child(CATALOG_NAME.c_str());
        if(!mapCatalog)
        {
            mapCatalog = mapFilenameToDoc[currentFilename]->append_child(CATALOG_NAME.c_str());
            mapFilenameToWasEdited[currentFilename] = true;
        }
        xml_node customItemCatalog = filenameAndDoc.second->child(CATALOG_NAME.c_str());
        for(xml_node customItemObject = customItemCatalog.first_child(); customItemObject;
            customItemObject = customItemObject.next_sibling())
        {
            ObjectRequiredAgeT requiredMapObjectAge = ObjectGetRequiredAge(customItemObject);
            ObjectOldAgeActionT whatToDoIfMapObjectExists = ObjectGetOldAgeAction(customItemObject);

            xml_node mapObject = GetMatchingNode(customItemObject, mapCatalog);
            if(mapObject)
            {
                if(requiredMapObjectAge == NEW)
                {
                    cout << endl;
                    ErrorLogger::Log(string("ERROR: CustomItem::AddToMap: object \"")
                         + mapObject.name() + " " + OBJECT_ID_NAME + "=" 
                         + mapObject.attribute(OBJECT_ID_NAME.c_str()).value() 
                         + "\" in file \"" + currentFilename + "\" already exists "
                         "in the map, which is a problem since \"" 
                         + OBJECT_REQUIRED_AGE_ATTR_NAME + "="
                         + OBJECT_REQUIRED_AGE_ATTR_VALUES[requiredMapObjectAge]
                         + "\".");
                    return false;
                }
                if(whatToDoIfMapObjectExists == MODIFY)
                {
                    ModifyNodeUsingValuesFromNewNode(mapObject, customItemObject);
                    mapFilenameToWasEdited[currentFilename] = true;
                }
                else if(whatToDoIfMapObjectExists == OVERWRITE)
                {
                    mapCatalog.remove_child(mapObject);
                    mapCatalog.append_copy(customItemObject);
                    mapFilenameToWasEdited[currentFilename] = true;
                }
            }
            else
            {
                if(requiredMapObjectAge == OLD)
                {
                    cout << endl;
                    ErrorLogger::Log(string("ERROR: CustomItem::AddToMap: object \"")
                         + customItemObject.name() + " " + OBJECT_ID_NAME + "=" 
                         + customItemObject.attribute(OBJECT_ID_NAME.c_str()).value() 
                         + "\" in file \"" + currentFilename + "\" does not exist in the map,"
                         + " which is a problem since \"" + OBJECT_REQUIRED_AGE_ATTR_NAME + "="
                         + OBJECT_REQUIRED_AGE_ATTR_VALUES[requiredMapObjectAge] + "\".");
                    return false;
                }
                //object is new. add it to the map.
                mapCatalog.append_copy(customItemObject);
                mapFilenameToWasEdited[filenameAndDoc.first] = true;
            }
        }
    }
    cout << " done." << endl << endl;
    return true;
}

//make sure this is called AFTER performing all other CustomItem actions.
bool CustomItem::Output()
{
    try
    {
        BOOST_FOREACH(stringXMLDocPair filenameAndDoc, _itemData._itemFilenameToDoc)
        {
            string currentFilename = filenameAndDoc.first;
            xml_node customItemCatalog = filenameAndDoc.second->child(CATALOG_NAME.c_str());

            if(!customItemCatalog)
            {
                continue;
            }
            ofstream fileWriter((OUTPUT_FOLDER/currentFilename).string().c_str(),
                ios_base::app);

            for(xml_node customItemObject = customItemCatalog.first_child(); customItemObject;
                customItemObject = customItemObject.next_sibling())
            {
                //before we output, delete attributes that are just notes to SC2DM.
                customItemObject.remove_attribute(OBJECT_REQUIRED_AGE_ATTR_NAME.c_str());
                customItemObject.remove_attribute(OBJECT_OLD_AGE_ACTION_ATTR_NAME.c_str());

                //output the object
                customItemObject.print(fileWriter);
            }

            fileWriter.close();
        }
    }
    catch(std::exception &e)
    {
        ErrorLogger::Log(string("ERROR: CustomItem::Output: ") + e.what());
        return false;
    }

    return true;
}