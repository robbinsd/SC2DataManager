// CustomItem.h
#ifndef __CUSTOMITEM_H__
#define __CUSTOMITEM_H__


#include "pugixml.hpp"
#include "boost/unordered_map.hpp"
#include "Template.h"
using namespace std;
using namespace pugi;
using namespace boost;


static const string OBJECT_REQUIRED_AGE_ATTR_NAME ("SC2DM_shouldAlreadyExist");
static const char *OBJECT_REQUIRED_AGE_ATTR_VALUES[] = { "yes", "no", "idc" }; 
static const string OBJECT_OLD_AGE_ACTION_ATTR_NAME ("SC2DM_whatToDoIfExists");
static const char *OBJECT_OLD_AGE_ACTION_ATTR_VALUES[] = { "modify", "overwrite", "doNothing" };

class MapManager;

class CustomItem
{
public:
	CustomItem(){}

	//@param baseItemTemplate: template for this custom item
	//@param varNameToValue: sets of values that should be filled in 
							//for each variable in the template
	//@return : error message
	bool Create(const Template &baseItemTemplate, const map<string, string> &varNameToValue);

    bool AddToMap(MapManager &mapManager) const;

	string GetId() const;

    void GetVariableData(VariableDataMap &varNameToVarData) const;

    bool Output();
private:
	//non-copyable semantics
	CustomItem(const CustomItem &other);
	const CustomItem& operator=(const CustomItem&);

    ItemData _itemData;
};

#endif // __CUSTOMITEM_H__