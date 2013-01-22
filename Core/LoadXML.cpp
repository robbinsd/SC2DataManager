#include "LoadXML.h"
#include <sstream>
#include <fstream>

string LoadXMLFile(xml_document *xmlDoc, const char *filePath)
{
	xml_parse_result result = xmlDoc->load_file(filePath);
	stringstream errorMsg("");
	if (!result)
	{
		errorMsg << "XML [" << filePath << "] parsed with errors." << endl
			<< "Error description: " << result.description() << endl;
		ifstream fileReader(filePath);
		fileReader.seekg(result.offset);
		string line;
		std::getline(fileReader, line);
		errorMsg << "Error offset: " << result.offset << " (error at [..." << line << "...]" << endl << endl;
	}
	return errorMsg.str();
}