// NodeMatch.h
#ifndef __NODEMATCH_H__
#define __NODEMATCH_H__


#include "pugixml.hpp"
#include <string>
using namespace std;
using namespace pugi;

//CONSTANTS
//All XML elements used in SC2 (except Arrays) can be identified uniquely. Each element's 
// id contains its name and the value of its "index" attribute. The "index" attribute
// can have many possible names, some of which are listed in ATTR_POSSIBLE_INDEX_NAMES.
// (If you know any additional ones, please add them). Many elements do not contain an
// "index" attribute, and are identified solely by their name.
static const char *ATTR_POSSIBLE_INDEX_NAMES[] = { "id", "index", "Terms", "CardId" };

//If an XML element is an Array, and it does not have an "index" attribute, then it
// cannot be identified uniquely. This means that it cannot be matched to another 
// element that wants to i.e. modify the values of its attributes.
//An element is an Array if its name is one of the following: 
static const char *NODE_POSSIBLE_ARRAY_NAMES[] = { "Cost", "Effect", "LayoutButtons" };
// OR if its name contains one of the following values as substrings:
static const char *NODE_POSSIBLE_ARRAY_SUBSTRS[] = { "Array" };

//checks if the two element nodes have the same value. if both their first attributes have names
// equal to firstAttrName, this also checks if both attributes have the same value.
//bool NodeMatch(const xml_node &node0, const xml_node &node1, const string &firstAttrName);

xml_node GetMatchingNode(const xml_node &toMatch, const xml_node &otherParent);//, const string &firstAttrName);

#endif // __NODEMATCH_H__