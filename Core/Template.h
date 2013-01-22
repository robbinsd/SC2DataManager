#ifndef __TEMPLATE_H__
#define __TEMPLATE_H__

#include <set>
#include <map>
#include <vector>

#include "boost/filesystem.hpp"
#include "pugixml.hpp"
#include <string>
using namespace std;
using namespace boost::filesystem;
using namespace boost;
using namespace pugi;

/* 
A Template represents a bunch of XML data that can be instantiated using a set
of parameters. Each Template contains a bunch of variables, or placeholders,
whose values are specified in the instantiation parameters. 

Many of the methods in this module return booleans. These booleans simply
indicate whether or not the method executed successfully.
*/

//--------CONSTANTS-------
/* character that marks start and end of a variable. */
static const string VAR_DELIM_CHAR("#");
/* character that marks start of a variable's default value. */
static const string DEFAULT_VAL_CHAR("=");
/* character that marks start and end of a formula. */
static const string FORMULA_DELIM_CHAR("=");
/* regular expression for variables names. */
static const string VAR_NAME_REGEX_FORMAT("([\\w\\d\\h]+)");
/* regular expression for default values. */
static const string VAR_DEFAULT_VAL_REGEX_FORMAT("([^"+VAR_DELIM_CHAR+DEFAULT_VAL_CHAR+"]*)");
/* name of XML node used to do "foreach" loops. */
static const string FOREACH_NODE_NAME("SC2DM_foreach");
/* name of XML attribute, used in foreach node, to indicate the loop variable. */
static const string FOREACH_NODE_VARNAME_ATTR_NAME("varName");
/* name of XML attribute, used in foreach node, to indicate the loop variable starting value. */
static const string FOREACH_NODE_FROM_ATTR_NAME("from");
/* name of XML attribute, used in foreach node, to indicate the loop variable ending value. */
static const string FOREACH_NODE_TO_ATTR_NAME("to");

/* regular expression for required variables. Examples of valid required variables:
    #varName#
    #id#
    #experience#
 */
static const string REQ_VAR_REGEX_FORMAT(VAR_DELIM_CHAR + VAR_NAME_REGEX_FORMAT + VAR_DELIM_CHAR);

/* regular expression for optional variables. Should ideally have same format as required variables,
   but with embedded default value. Examples of valid optional variables:
    #varName=defaultValue#
    #impact model=None#
 */
static const string OPT_VAR_REGEX_FORMAT(VAR_DELIM_CHAR + VAR_NAME_REGEX_FORMAT + DEFAULT_VAL_CHAR
    + VAR_DEFAULT_VAL_REGEX_FORMAT + VAR_DELIM_CHAR);

/* regular expression for formulas. Formula validation is not done by regular expression,
   but rather by expression parsing library (muParser). Examples of formulas:
    =2*#level#=
    =3*(#damage#+2)=
 */
static const string FORMULA_REGEX_FORMAT(FORMULA_DELIM_CHAR + "([^"+FORMULA_DELIM_CHAR+"]+)" 
    + FORMULA_DELIM_CHAR);


/* Enumeration containing all variable types. */
enum VariableType
{
    REQUIRED_VAR,       /* required variable. */
    OPTIONAL_VAR,       /* optional variable (has default value). */
    LOOP_VAR            /* loop variable. automatically assigned values. */
};

/* Structure containing data about each variable. */
struct VariableData
{
    string varName;             /* name of the variable. */
    string varValue;            /* value of variable. Only set if located in a CustomItem. */
    VariableType type;          /* Type of the variable. */
    xml_node forEachNode;       /* Used by LOOP variables. Indicates the node that 
                                   defines the foreach loop. */

    VariableData() : varName(), varValue(), type(REQUIRED_VAR), forEachNode(xml_node())
    {
    }

    VariableData(string varName_, VariableType type_, xml_node forEachNode_=xml_node())
        : varName(varName_), type(type_), forEachNode(forEachNode_)
    {
    }
};

typedef map<string, VariableData> VariableDataMap;
typedef pair<string, VariableData> StringVarDataPair;

/* Structure containing data common to both Templates and instantiations of Templates
   (A.K.A. CustomItems). */
class ItemData
{
public:
    ItemData (){}    /* does nothing. */
    ~ItemData ();   /* destructor. */

    /* Read a CustomItem or a preconstructed Template from disk. */
    bool Create(const path &templatePath);

    /* Finds all variables contained in XML data. Populates variableNameToVariableData. */
    bool FindVariables();

    /* Set values of variables.
       @param varNameToValue: contains pairs of mappings from variable names to 
                              variable values.*/
    bool SetVariables(const map<string, string> &varNameToValue);

    //------------(PUBLIC) MEMBER VARIABLES-------------
    /* Identifier containing either the name of the Template, or the name of the CustomItem. */
    string _id;     
    /* access data about each variable, using variable name as key. */
    VariableDataMap _variableNameToVariableData;

    /* contains XML documents from the GameData folder, using filename as key. */
    map<string, xml_document *> _itemFilenameToDoc;
private:

    /* The following two functions are used to both find and set variables. Also used to
       find and expand foreach loops. */ 
    bool FindVariablesInXMLNode(const xml_node &node, const string &filename);
    bool SetVariablesInXMLNode(xml_node &node, const string &filename, 
        const map<string, string> &varNameToValue);
    bool EnterForEachLoop(const xml_node &forEachNode, const string &filename, 
        string &forEachVarName);
    bool ExpandForEachLoop(xml_node &node, const string &filename, 
        const map<string, string> &varNameToValue, const string &forEachVarName);
    bool ExitForEachLoop (const string &forEachVarName);
    bool FindVariablesInXMLNodeAndChildren(const xml_node &node, const string &filename);
    bool SetVariablesInXMLNodeAndChildren(xml_node &node, const string &filename, 
        const map<string, string> &varNameToValue);

    /* Used to evaluate formulas. */
    bool EvaluateFormulasInXMLNode(xml_node &node, const string &filename);
};

/* The Template class is essentially just a wrapper around an ItemData instance.
   Eventually, it will contain functions not only for reading existing templates,
   but also for creating templates from scratch. */
class Template
{
public:
    /* Must be called on program init. Returns whether or not Templating system
       could be initialized. */
    static bool InitTemplates();

    /* Does nothing. Template must be initialized using Create. */
    Template(){}

    ~Template();

    //-----------VIEWING A TEMPLATE--------------

    /* Read a pre-constructed template from disk.*/
    bool Create(const path &templatePath);
    
    /* Instantiates the template, giving values to each variable. Returns the resulting
       ItemData object by reference. */
    bool Instantiate(const map<string, string> &varNameToValue, ItemData& itemData) const;

    void GetVariableData(map<string, VariableData> &varNameToVarData) const;

    const string &GetName() const;

    //-----------CREATING A TEMPLATE---------------
    //TODO: implement methods to create a template from scratch.
    
    //void SaveToDisk(const path &templatePath);
    //-----------------------------------------------


private:
    void GetItemData(ItemData &a_itemData) const;

    //non-copyable semantics
    Template(const Template &other);
    const Template& operator=(const Template&);

    ItemData _itemData;
    string _name;
    size_t _numItemsCreated;
};

#endif // __TEMPLATE_H__