// Template.cpp
#include "Template.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>

#include "boost/foreach.hpp"
#include "boost/regex.hpp"
#include "boost/functional/hash.hpp"
#include "boost/lexical_cast.hpp"
#include "muParser.h"

#include "CommonConstants.h"
#include "NodeMatch.h"
#include "LoadXML.h"
#include "ErrorLogger.h"
using namespace std;
using namespace pugi;
using namespace boost;

//typedefs
typedef pair<string, xml_document *> stringXMLDocPair;

//globals
static bool hasInited = false;      //whether or not InitTemplates was called.
static boost::regex REQ_VAR_REGEX;
static boost::regex OPT_VAR_REGEX;
static boost::regex FORMULA_REGEX;

//--------------- ENUMERATION/STRUCT DEFINITIONS -----------------
enum AttrTokenType
{
    REQUIRED_VAR_TOK=REQUIRED_VAR,
    OPTIONAL_VAR_TOK=OPTIONAL_VAR,
    FORMULA_TOK,             /* Token representing a formula (i.e. =2*#damage#=) */
    REGULAR_TEXT_TOK         /* Token representing everything else. */
};

struct AttrToken
{
    AttrTokenType type;
    string tokenText;

    //only used by variables:
    string varName;
    string defaultVal; //only used by optional vars

    //only used by formulas:
    string formulaContents;

    AttrToken(const string &a_tokenText) 
        : type(REGULAR_TEXT_TOK)
        , tokenText(a_tokenText)
    {
    }

    AttrToken(AttrTokenType a_type, const string &a_tokenText)
        : type(a_type)
        , tokenText(a_tokenText)
    {
    }
};

//----------------- FUNCTION PROTOTYPES -------------------
bool IsNodeDescendantOfNode( xml_node node, xml_node possibleAncestor );
bool GetFormulaTokensInStr(const string &str, vector<AttrToken> &tokens);
bool GetVariableTokensInStr(const string &str, vector<AttrToken> &tokens);

//------------------ INITIALIZATION ------------------------
bool TryToAssignRegEx(boost::regex &regEx, const string &format)
{
    try
    {
        regEx.assign(format);
    }
    catch(boost::regex_error &e)
    {
        ErrorLogger::Log("ERROR: TryToAssignRegEx: " + format +
            " is not a valid regular expression: \"" + e.what() + "\"");
        return false;
    }
    return true;
}

bool Template::InitTemplates()
{
    for(int i = 0; i < 2; ++i)
    {
        bool isRequired = (i==0);

        string regExFormat( isRequired ? REQ_VAR_REGEX_FORMAT : OPT_VAR_REGEX_FORMAT );
        boost::regex &varRegEx( isRequired ? REQ_VAR_REGEX : OPT_VAR_REGEX );
        if(!TryToAssignRegEx( varRegEx, regExFormat ))
        {
            ErrorLogger::Log("ERROR: Template::InitTemplates: failed to assign a regex.");
            return false;
        }
    }
    if(!TryToAssignRegEx( FORMULA_REGEX, FORMULA_REGEX_FORMAT ))
    {
        ErrorLogger::Log("ERROR: Template::InitTemplates: failed to assign a regex.");
        return false;
    }
    hasInited = true;
    return true;
}

//-------------------------START PUBLIC MEMBER FUNCTIONS----------------------------

//---TEMPLATE------
bool Template::Create(const path &templatePath)
{
    cout << "Creating template from path " << templatePath << "...";
    if(!hasInited)
    {
        cout << endl;
        ErrorLogger::Log("ERROR: Template::Create: Template system has"
            " not been initialized! Please call InitTemplates.");
        return false;
    }
    _numItemsCreated = 0;
    _name = templatePath.filename();
    if(!_itemData.Create(templatePath))
    {
        cout << endl;
        ErrorLogger::Log("ERROR: Template::Create: could not load Template "
            "from file: " + templatePath.string() + ".");
        return false;
    }
    cout << " done." << endl << endl;
    return true;
}

Template::~Template ()
{
}

void Template::GetVariableData(VariableDataMap &varNameToVarData) const
{
    BOOST_FOREACH(StringVarDataPair varNameAndData, _itemData._variableNameToVariableData)
    {
        varNameToVarData[varNameAndData.first] = varNameAndData.second;
        varNameToVarData[varNameAndData.first].forEachNode = xml_node();
    }
}

const string &Template::GetName() const
{
    return _name;
}

/* Returns a copy of the Template's ItemData, by reference. */
void Template::GetItemData(ItemData &itemData) const
{
    itemData = _itemData;
    itemData._itemFilenameToDoc.clear();

    BOOST_FOREACH(stringXMLDocPair filenameAndDoc, _itemData._itemFilenameToDoc)
    {
        xml_document *currDoc = filenameAndDoc.second;
        xml_document *currDocCopy = new xml_document();
        currDocCopy->reset(*currDoc);
        itemData._itemFilenameToDoc[filenameAndDoc.first] = currDocCopy;
    }
}

bool Template::Instantiate(const map<string, string> &varNameToValue, ItemData& itemData) const
{
    //first, get a copy of our ItemData.
    GetItemData(itemData);
    map<string,string>::const_iterator itr = varNameToValue.find("_id");
    if(itr != varNameToValue.end())
    {
        itemData._id += ":"+itr->second;
    }
    else
    {
        itemData._id += ":"+lexical_cast<string>(_numItemsCreated);
    }
    cout << "Creating CustomItem \"" << itemData._id << "\"...";

    //ignore const-ness of this method, since this really doesn't change any state that the 
    //caller knows about.
    ++((Template *)(this))->_numItemsCreated;

    //fill in all the variables.
    bool success = itemData.SetVariables(varNameToValue);

    cout << (success ? "done." : "failed.") << endl;
    return success;
}


//----ITEMDATA----
bool ItemData::Create (const path &itemDataPath)
{
    if(!boost::filesystem::exists(itemDataPath))
    {
        cout << endl;
        ErrorLogger::Log("ERROR: ItemData::Create: " + itemDataPath.string() 
             + " does not exist!");
        return false;
    }
    _id = itemDataPath.filename();
    directory_iterator end;
    for(directory_iterator iter(itemDataPath); iter != end; ++iter)
    {
        path currentDataFilePath = iter->path();
        if(currentDataFilePath.extension() != ".xml")
        {
            continue;
        }
        xml_document *baseItemDoc = new xml_document();
        string currentFilename = currentDataFilePath.filename();
        _itemFilenameToDoc[currentFilename] = baseItemDoc;
        string error = LoadXMLFile(baseItemDoc, currentDataFilePath.string().c_str());
        if(error != "")
        {
            cout << endl;
            ErrorLogger::Log("ERROR: Template::Create: " + error);
            return false;
        }
    }
    if (!FindVariables())
    {
        return false;
    }
    return true;
}

ItemData::~ItemData()
{
    BOOST_FOREACH (stringXMLDocPair filenameAndDoc, _itemFilenameToDoc)
    {
        delete filenameAndDoc.second;
    }
}

bool ItemData::FindVariables ()
{
    BOOST_FOREACH (stringXMLDocPair filenameAndXMLDoc, _itemFilenameToDoc)
    {
        string currFilename = filenameAndXMLDoc.first;
        xml_node currRootNode = filenameAndXMLDoc.second->document_element();
        if( !FindVariablesInXMLNodeAndChildren(currRootNode, currFilename))
        {
            return false;
        }
    }
    return true;
}

bool ItemData::SetVariables(const map<string, string> &a_varNameToValue)
{
    map<string, string> varNameToValue(a_varNameToValue);
    bool success = true;
    //make sure there are no invalid variable names
    typedef pair<string,string> stringPair;
    BOOST_FOREACH(stringPair varNameAndValue, varNameToValue)
    {
        const string &varName = varNameAndValue.first;
        if(_variableNameToVariableData.count(varName) == 0)
        {
            ErrorLogger::Log("ERROR: ItemData::SetVariable: custom item \"" + _id + 
                "\" cannot set variable \"" + varName + "\" because it does "
                "not exist in the template.");
            success = false;
        }
    }
    //make sure all required variables exist
    typedef pair<string, VariableData> variableDataPair;
    BOOST_FOREACH(variableDataPair varNameAndData, _variableNameToVariableData)
    {
        const string &varName = varNameAndData.first;
        const VariableData &varData = varNameAndData.second;
        if(varData.type == REQUIRED_VAR && varNameToValue.count(varName) == 0)
        {
            ErrorLogger::Log("ERROR: ItemData::SetVariable: custom item \"" + _id +
                "\" is missing value for required variable \"" + varName + "\".");
            success = false;
        }
    }
    if(!success)
    {
        return false;
    }

    BOOST_FOREACH(stringXMLDocPair filenameAndDoc, _itemFilenameToDoc)
    {
        xml_document *doc = filenameAndDoc.second;
        if(!SetVariablesInXMLNodeAndChildren(doc->document_element(), 
            filenameAndDoc.first, varNameToValue))
        {
            return false;
        }
    }

    //update the VariableData to reflect that the variable values were set correctly.
    BOOST_FOREACH(StringVarDataPair varNameAndVarData, _variableNameToVariableData)
    {
        string varName (varNameAndVarData.first);
        _variableNameToVariableData[varName].varValue = varNameToValue[varName];
    }
    return true;
}

//--------------------END PUBLIC FUNCTIONS ----------------------


//------------------START MEMBER HELPER FUNCTIONS ----------------
/* Parses all variables in "node" and its children. Puts their data into 
   _variableNameToVariableData. */
bool ItemData::FindVariablesInXMLNodeAndChildren(const xml_node &node, const string &filename)
{
    bool isForEachNode = (string(node.name()) == FOREACH_NODE_NAME);
    string forEachVarName;
    if(isForEachNode)
    {
        //validate foreach nodes.
        if(!EnterForEachLoop(node, filename, forEachVarName))
        {
            return false;
        }
    }
    if(!FindVariablesInXMLNode(node, filename))
    {
        return false;
    }
    for(xml_node child = node.first_child(); child; child = child.next_sibling())
    {
        if(!FindVariablesInXMLNodeAndChildren(child, filename))
        {
            return false;
        }
    }
    if(isForEachNode)
    {
        if(!ExitForEachLoop(forEachVarName))
        {
            return false;
        }
    }
    return true;
}

/* Expands foreach loops that are at "node" or children of "node". Recursively assigns variables 
   their corresponding values, then evaluates all formulas. Loop variables are 
   automatically assigned values, whereas other variables receive the values given
   in "varNameToValue". 
 */
bool ItemData::SetVariablesInXMLNodeAndChildren(xml_node &node, const string &filename, 
    const map<string, string> &varNameToValue)
{
    bool isForEachNode = (string(node.name()) == FOREACH_NODE_NAME);
    string forEachVarName;
    if(isForEachNode)
    {
        if(!EnterForEachLoop(node, filename, forEachVarName))
        {
            return false;
        }
    }

    //set the variables in this node's attributes
    if(!SetVariablesInXMLNode(node, filename, varNameToValue))
    {
        return false;
    }
    if(!EvaluateFormulasInXMLNode(node, filename))
    {
        return false;
    }

    if(isForEachNode)
    {
        //expand foreach loop, and set variables in expanded children.
        if(!ExpandForEachLoop(node, filename, varNameToValue, forEachVarName))
        {
            return false;
        }
    }
    else
    {
        //do normal recursion. Iterate backwards over children, so that we don't traverse
        // any nodes that get appended to "node" by child recursive calls. (These nodes
        // would be expansions of foreach loops).
        for(xml_node child = node.last_child(); child; child = child.previous_sibling())
        {
            if( !SetVariablesInXMLNodeAndChildren(child, filename, varNameToValue))
            {
                return false;
            }
        }
    }
    if(isForEachNode)
    {
        if(!ExitForEachLoop(forEachVarName))
        {
            return false;
        }
    }
    return true;
}

/* Parses each variable in the attributes of "node", storing information about each in
   _variableNameToVariableData.*/
bool ItemData::FindVariablesInXMLNode(const xml_node &node, const string &/*filename*/)
{
    for(pugi::xml_attribute attr = node.first_attribute(); attr; attr = attr.next_attribute())
    {
        vector<AttrToken> tokens;
        if(!GetVariableTokensInStr( attr.value(), tokens ))
        {
            ErrorLogger::Log("ERROR: Template::FindVariablesInXMLNode: "
                "had trouble tokenizing variables.");
            return false;
        }
        BOOST_FOREACH(AttrToken &token, tokens)
        {
            if(token.type != REGULAR_TEXT_TOK)
            {
                //try to add var to _variableNameToVariableData.
                if(_variableNameToVariableData.count(token.varName) == 1)
                {
                    const VariableData &tokenVarData = _variableNameToVariableData[token.varName];
                    //existing variable. check to see if the new instance is valid in the context
                    // of the existing instance.
                    if(tokenVarData.type == LOOP_VAR)
                    {
                        //if existing variable is a loop variable for foreach, make sure this
                        // instance is a descendant of it.
                        if(!IsNodeDescendantOfNode(node, tokenVarData.forEachNode))
                        {
                            ErrorLogger::Log("ERROR: Template::FindVariablesInXMLNode: variable \""
                                + token.varName + "\" cannot be used as both a required"
                                " variable and a " + FOREACH_NODE_NAME + " variable.");
                            return false;
                        }
                        //loop variables cannot have default values.
                        if(!token.defaultVal.empty())
                    {
                        ErrorLogger::Log("ERROR: Template::FindVariablesInXMLNode: variable \""
                            + token.varName + "\" cannot have a default value, since "
                            " it is a " + FOREACH_NODE_NAME + " variable.");
                        return false;
                    }
                }
                else if( (tokenVarData.type == OPTIONAL_VAR && token.type == REQUIRED_VAR_TOK)
                        ||   (tokenVarData.type == REQUIRED_VAR && token.type == OPTIONAL_VAR_TOK))
                    {
                        ErrorLogger::Log("ERROR: Template::FindVariablesInXMLNode: cannot have variable "
                            "with name " + token.varName + " that is both required and optional.");
                        return false;
                    }
                }
                else
                {
                    //new (non-loop) variable. add to collection of variables.
                    _variableNameToVariableData[token.varName] = 
                        VariableData(token.varName, (token.type == REQUIRED_VAR_TOK ? REQUIRED_VAR : OPTIONAL_VAR));
                }
            }
        }
    }
    return true;
}

/* Assigns values to variables located in the attributes of "node". "varNameToValue" contains
   the value to assign to each variable. */
bool ItemData::SetVariablesInXMLNode(xml_node &node, const string &/*filename*/, 
    const map<string, string> &varNameToValue)
{
    for(pugi::xml_attribute attr = node.first_attribute(); attr; attr = attr.next_attribute())
    {
        vector<AttrToken> tokens;
        if(!GetVariableTokensInStr( attr.value(), tokens ))
        {
            ErrorLogger::Log("ERROR: Template::SetVariablesInXMLNode: "
                "had trouble tokenizing variables.");
            return false;
        }
        string newAttrValue("");
        BOOST_FOREACH(AttrToken &token, tokens)
        {
            if(token.type != REGULAR_TEXT_TOK)
            {
                string varValue;
                if(varNameToValue.count(token.varName))
                {
                    //variable was given a value. set it.
                    varValue = varNameToValue.at(token.varName);
                }
                else
                {
                    if(token.type == REQUIRED_VAR_TOK)
                    {
                        //required variable not assigned a value.
                        ErrorLogger::Log("ERROR: ItemData::SetVariablesInXMLNode: required"
                            " variable \"" + token.varName + "\" was not assigned a value.");
                        return false;
                    }
                    else
                    {
                        //optional variable not given value. uses default value.
                        varValue = token.defaultVal;
                    }
                }
                token.tokenText = varValue;
            }
            newAttrValue.append(token.tokenText);
        }
        //set the new attribute value.
        attr.set_value(newAttrValue.c_str());
    }
    return true;
}

/* Evaluates all formulas located in "node" 's attributes' values. Replaces each
   formula with the result of its computation. Note that the expressions for formulas are
   not validated until this function is called. 
   @param filename: name of the file that contains "node".
 */
bool ItemData::EvaluateFormulasInXMLNode(xml_node &node, const string &filename)
{
    //iterate over every attribute
    for(pugi::xml_attribute attr = node.first_attribute(); attr; attr = attr.next_attribute())
    {
        vector<AttrToken> tokens;
        if(!GetFormulaTokensInStr( attr.value(), tokens ))
        {
            ErrorLogger::Log("ERROR: Template::EvaluateFormulasInXMLNode: "
                "had trouble tokenizing formulas.");
            return false;
        }
        string newAttrValue("");
        BOOST_FOREACH(AttrToken &token, tokens)
        {
            if(token.type == FORMULA_TOK)
            {
                double formulaResult = 0;
                try
                {
                    mu::Parser exprParser;
                    exprParser.SetExpr(token.formulaContents);
                    formulaResult = exprParser.Eval();
                }
                catch (mu::Parser::exception_type &e)
                {
                    ErrorLogger::Log("ERROR: ItemData::EvaluateFormulasInXMLNode: failed "
                        "to parse expression \"" + token.formulaContents + "\" in node \"" 
                        + node.name() + "\" in file \"" + filename + "\". details: " 
                        + e.GetMsg());
                    return false;
                }
                token.tokenText = lexical_cast<string>(formulaResult);
            }
            newAttrValue.append(token.tokenText);
        }
        attr.set_value(newAttrValue.c_str());
    }
    return true;
}

/* Validates the "forEachNode" and sets up state in preparation for calling ExpandForEachLoop. */
bool ItemData::EnterForEachLoop(const xml_node &forEachNode, const string &/*filename*/, string &forEachVarName)
{
    if( !forEachNode.attribute(FOREACH_NODE_VARNAME_ATTR_NAME.c_str())
        || !forEachNode.attribute(FOREACH_NODE_FROM_ATTR_NAME.c_str())
        || !forEachNode.attribute(FOREACH_NODE_TO_ATTR_NAME.c_str()))
    {
        ErrorLogger::Log("ERROR: ItemData::EnterForEachLoop: \"" 
            + FOREACH_NODE_NAME + "\" nodes must have three attributes: "
            + FOREACH_NODE_VARNAME_ATTR_NAME + ", "
            + FOREACH_NODE_FROM_ATTR_NAME + ", "
            + "and " + FOREACH_NODE_TO_ATTR_NAME + ".");
        return false;
    }
    forEachVarName = forEachNode.attribute(FOREACH_NODE_VARNAME_ATTR_NAME.c_str()).value();
    vector<AttrToken> varNameTokens;
    if(!GetVariableTokensInStr( forEachVarName, varNameTokens ))
    {
        ErrorLogger::Log("ERROR: Template::EnterForEachLoop: "
            "had trouble tokenizing variables in \"" + forEachVarName + "\".");
        return false;
    }
    //if forEachVarName is not just regular text, return an error.
    if(varNameTokens.size() != 1 || varNameTokens[0].type != REGULAR_TEXT_TOK)
    {
        ErrorLogger::Log("ERROR: ItemData::EnterForEachLoop: the \"" 
            + FOREACH_NODE_VARNAME_ATTR_NAME + "\" attribute must have a "
            "plain-text value (cannot contain variables or formulas).");
        return false;
    }
    if(_variableNameToVariableData.count(forEachVarName))
    {
        const VariableData &existingVarData = _variableNameToVariableData[forEachVarName];
        if(existingVarData.type != LOOP_VAR || existingVarData.forEachNode != forEachNode)
        {
            //fail if the loop variable is either used as a global required/optional variable,
            // or if the same loop variable name is used in two nested foreach loops.
            ErrorLogger::Log("ERROR: Template::EnterForEachLoop: foreach "
                "variable " + forEachVarName + " must be a unique variable "
                "name.");
            return false;
        }
    }
    //add foreach forEachNode to the map, so that this forEachNode's children can use the loop
    //variable.
    _variableNameToVariableData[forEachVarName] = VariableData(forEachVarName, LOOP_VAR, forEachNode);
    return true;
}

/* Undoes the state changes made by EnterForEachLoop. */
bool ItemData::ExitForEachLoop (const string &forEachVarName)
{
    //a foreach node's data is only relevant to its children. erase it, since we
    //already traversed the children.
    return _variableNameToVariableData.erase(forEachVarName) == 1;
}

/* Unrolls the foreach loop, and recursively sets variables values in the unrolled nodes. */
bool ItemData::ExpandForEachLoop(xml_node &node, const string &filename, 
    const map<string, string> &varNameToValue, const string &forEachVarName)
{
    string fromStr = node.attribute(FOREACH_NODE_FROM_ATTR_NAME.c_str()).value();
    string toStr = node.attribute(FOREACH_NODE_TO_ATTR_NAME.c_str()).value();

    if(varNameToValue.count(forEachVarName) > 0)
    {
        ErrorLogger::Log("ERROR: ItemData::ExpandForEachLoop: " + forEachVarName
            + " is the loop variable of a " + FOREACH_NODE_NAME + " loop. It cannot "
            "be assigned the value \"" + varNameToValue.at(forEachVarName) + "\".");
    }
    int fromValue;
    int toValue;
    try
    {
        fromValue = lexical_cast<int>(fromStr);
        toValue = lexical_cast<int>(toStr);
    }
    catch (boost::bad_lexical_cast &e)
    {
        ErrorLogger::Log("ERROR: ItemData::ExpandForEachLoop: "
            + FOREACH_NODE_FROM_ATTR_NAME + " and "
            + FOREACH_NODE_TO_ATTR_NAME + " must have integral values. Details: "
            + e.what() + ".");
        return false;
    }
    for(int value = fromValue; value <= toValue; ++value)
    {
        string valueStr( lexical_cast<string>(value) );
        for(xml_node childOfForeach = node.first_child(); childOfForeach; 
            childOfForeach = childOfForeach.next_sibling())
        {
            xml_node childCopy = node.parent().append_copy(childOfForeach);
            map<string, string> childVarNameToValue(varNameToValue);
            childVarNameToValue[forEachVarName] = valueStr;

            //recurse on childCopy, using "value" as the current value of
            // the loop variable.
            if(!SetVariablesInXMLNodeAndChildren(childCopy, filename, childVarNameToValue))
            {
                return false;
            }
        }
    }
    //Delete the original foreach node, now that we've expanded it.
    node.parent().remove_child(node);
    return true;
}

//------------------END MEMBER HELPER FUNCTIONS --------------------



//-----------------START NON-MEMBER HELPER FUNCTIONS ---------------

/* Tokenizes the given attribute value string into tokens that are either plain
   text or variable declarations. Treats formula declaration like plain text. */
bool GetVariableTokensInStr(const string &str, vector<AttrToken> &tokens)
{
    string::const_iterator begin = str.begin();
    string::const_iterator end = str.end();

    boost::match_results<string::const_iterator> reqVarMatches;
    boost::match_results<string::const_iterator> optVarMatches;
    while(true)
    {
        bool matchedReq = boost::regex_search(begin, end, reqVarMatches, REQ_VAR_REGEX);
        bool matchedOpt = boost::regex_search(begin, end, optVarMatches, OPT_VAR_REGEX);
        if(!matchedReq && !matchedOpt)
        {
            break;
        }
        bool isRequired;
        if(matchedReq && !matchedOpt)
        {
            isRequired = true;
        }
        else if(matchedOpt && !matchedReq)
        {
            isRequired = false;
        }
        else
        {
            //if attribute still has both an optional var and a required var, pick
            // whichever one starts first.
            isRequired = ( reqVarMatches[0].first < optVarMatches[0].first );
        }
        boost::match_results<string::const_iterator> &matches = 
            ( isRequired ? reqVarMatches : optVarMatches );

        string nonVarTokenStr(begin, matches[0].first);
        if(!nonVarTokenStr.empty())
        {
            tokens.push_back( AttrToken(REGULAR_TEXT_TOK, nonVarTokenStr) );
        }
        string varTokenStr(matches[0].first, matches[0].second);
        AttrToken varToken( (isRequired ? REQUIRED_VAR_TOK : OPTIONAL_VAR_TOK), varTokenStr );
        varToken.varName = string(matches[1].first, matches[1].second);
        if(!isRequired)
        {
            varToken.defaultVal = string(matches[2].first, matches[2].second);
        }
        tokens.push_back(varToken);

        begin = matches[0].second;
    }
    //account for extra plain text after the last variable declaration.
    if(begin != end)
    {
        string nonVarTokenStr(begin, end);
        tokens.push_back( AttrToken(REGULAR_TEXT_TOK, nonVarTokenStr) );
    }
    return true;
}

/* Tokenizes attrContents into a series of plain text tokens and formula declarations. */
bool GetFormulaTokensInStr(const string &str, vector<AttrToken> &tokens)
{
    string::const_iterator begin = str.begin();
    string::const_iterator end = str.end();

    boost::match_results<string::const_iterator> matches;
    while( boost::regex_search(begin, end, matches, FORMULA_REGEX) )
    {
        string nonFormulaTokenStr(begin, matches[0].first);
        if(!nonFormulaTokenStr.empty())
        {
            tokens.push_back( AttrToken(REGULAR_TEXT_TOK, nonFormulaTokenStr) );
        }
        string formulaTokenStr(matches[0].first, matches[0].second);
        AttrToken formulaToken( FORMULA_TOK, formulaTokenStr );
        formulaToken.formulaContents = string(matches[1].first, matches[1].second); 
        tokens.push_back(formulaToken);

        begin = matches[0].second;
    }
    if(begin != end)
    {
        string nonFormulaTokenStr(begin, end);
        tokens.push_back( AttrToken(REGULAR_TEXT_TOK, nonFormulaTokenStr) );
    }
    return true;
}

/* Returns whether or not "node" is a descendant of "possibleAncestor". */
bool IsNodeDescendantOfNode( xml_node node, xml_node possibleAncestor )
{
    if(!node.parent() || !possibleAncestor)
    {
        return false;
    }
    for(xml_node descendant = node.parent(); descendant != descendant.root();
        descendant = descendant.parent())
    {
        if(descendant == possibleAncestor)
        {
            return true;
        }
    }
    return false;
}
//-----------------END NON-MEMBER HELPER FUNCTIONS ---------------