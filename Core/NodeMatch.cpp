#include "NodeMatch.h"
#include "CommonConstants.h"
#include <vector>
#include <stack>

//--------static functions-------
//checks if the two element nodes have the same value. if both their first attributes have names
// equal to firstAttrName, this also checks if both attributes have the same value.
/*bool NodeMatch(const xml_node &node0, const xml_node &node1, const string &toMatchAttrName)
{
	if(string(node0.name()) == string(node1.name())){
		if(toMatchAttrName == node0.first_attribute().name()){
			if(toMatchAttrName == node1.first_attribute().name()){
				return string(node0.first_attribute().value()) == string(node1.first_attribute().value());
			}
			return false;
		}
		return true;
	}
	return false;
}*/

string GetNodeNamePlusAttrXPath(const xml_node &node, const xml_attribute &attr)
{
    return string(node.name())+"[@"+attr.name()+"='"+attr.value()+"']";
}

string GetNodeFullXPath(const xml_node &node)
{
    string xpath(node.name());
    if(!node.first_attribute())
    {
        return xpath;
    }
    string stuffInBrackets;
    for(xml_attribute attr = node.first_attribute(); attr;
        attr = attr.next_attribute())
    {
        if(attr != node.first_attribute())
        {
            stuffInBrackets.append(" and ");
        }
        stuffInBrackets.append(string("@")+attr.name()+"='"+attr.value()+"'");
    }
    xpath.append("["+stuffInBrackets+"]");
    return xpath;
}

string GetNodeXPath(const xml_node &node)
{
    string nodeName(node.name());

    //has attr with an "index" name?
    for(xml_attribute attr = node.first_attribute(); attr; attr = attr.next_attribute())
    {
        string attrName(attr.name());
        for(size_t i = 0; i < sizeof(ATTR_POSSIBLE_INDEX_NAMES)/sizeof(char *); ++i)
        {
            if(attrName == ATTR_POSSIBLE_INDEX_NAMES[i])
            {
                return GetNodeNamePlusAttrXPath(node, attr);
            }
        }
    }

    //node name indicates it's an Array?
    for(size_t i = 0; i < sizeof(NODE_POSSIBLE_ARRAY_SUBSTRS)/sizeof(char *); ++i)
    {
        if(nodeName.find(NODE_POSSIBLE_ARRAY_SUBSTRS[i]) != string::npos)
        {
            return GetNodeFullXPath(node);
        }
    }
    for(size_t i = 0; i < sizeof(NODE_POSSIBLE_ARRAY_NAMES)/sizeof(char *); ++i)
    {
        if(nodeName == NODE_POSSIBLE_ARRAY_NAMES[i])
        {
            return GetNodeFullXPath(node);
        }
    }

    //how many attrs?
    /*xml_attribute firstAttr = node.first_attribute();
    if(firstAttr)
    {
        if(firstAttr != node.last_attribute())
        {
            //>1 attrs
            return GetNodeNamePlusAttrXPath(node, firstAttr);
        }
        else
        {
            //1 attr
            if(node.first_child())
            {
                //has children.
                return GetNodeNamePlusAttrXPath(node, firstAttr);
            }
        }
    }
    //0 attributes OR (1 attr and no children)
    return node.name();
    */
    xml_attribute firstAttr = node.first_attribute();
    if(firstAttr)
    {
        //1 attr
        if(node.first_child())
        {
            //if has children, firstAttr is probably an index.
            return GetNodeNamePlusAttrXPath(node, firstAttr);
        }
    }
    //0 attributes OR (1 attr and no children)
    return node.name();
}

/*xml_node GetMatchingNode(const xml_node &toMatch, const xml_node &otherRoot)//, const string &toMatchAttrName)
{
	xml_node curr = toMatch;
    string fullQuery = "";
	while(curr.parent())
    {
		xml_node currParent = curr.parent();
        string query = GetNodeXPath(curr);
        if(query == "")
        {
            return xml_node();
        }
        fullQuery = string("/") + query + fullQuery; 
		curr = currParent;
	}
	//ensure otherRoot is actually the root by calling root() method
    return otherRoot.root().select_single_node( fullQuery.c_str() ).node();
}*/
xml_node GetMatchingNode(const xml_node &toMatch, const xml_node &otherParent)
{
	xml_node curr = toMatch;
    string fullQuery = "";
    string query = GetNodeXPath(curr);
    if(query == "")
    {
        return xml_node();
    }
    return otherParent.select_single_node( query.c_str() ).node();
}