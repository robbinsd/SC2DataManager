#ifndef __LOAD_XML_H___
#define __LOAD_XML_H___

#include <string>
#include "pugixml.hpp"
using namespace std;
using namespace pugi;

string LoadXMLFile(xml_document *xmlDoc, const char *filePath);

#endif //__LOAD_XML_H___