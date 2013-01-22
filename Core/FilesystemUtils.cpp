#include "FilesystemUtils.h"
#include <iostream>
#include "ErrorLogger.h"

using namespace std;

//Adapted from nijansen's response to
//http://stackoverflow.com/questions/8593608/how-can-i-copy-a-directory-using-boost-filesystem

bool CopyDirectoryAndContents(  const boost::filesystem::path &source,
                                const boost::filesystem::path &dest )
{
    bool success = true;
    namespace fs = boost::filesystem;
    try
    {
        // Check whether the function call is valid
        if(!fs::exists(source) || !fs::is_directory(source))
        {
            ErrorLogger::Log("ERROR: CopyDirectoryAndContents: Source directory " 
                + source.string() + " does not exist or is not a directory.");
            return false;
        }
        if(fs::exists(dest))
        {
            ErrorLogger::Log("ERROR: CopyDirectoryAndContents: Destination directory " 
                + dest.string() + " already exists.");
            return false;
        }
        // Create the destination directory
        if(!fs::create_directory(dest))
        {
            ErrorLogger::Log("ERROR: CopyDirectoryAndContents: Unable to create "
                "destination directory " + dest.string());
            return false;
        }
    }
    catch(fs::filesystem_error& e)
    {
        ErrorLogger::Log(string("ERROR: CopyDirectoryAndContents: ") + e.what());
        return false;
    }
    // Iterate through the source directory
    for(fs::directory_iterator it(source); it != fs::directory_iterator(); it++)
    {
        try
        {
            fs::path pathInSource(it->path());
            fs::path pathInDest(dest/pathInSource.filename());
            if(fs::is_directory(pathInSource))
            {
                // Found directory: Recurse.
                if(!CopyDirectoryAndContents(pathInSource, pathInDest))
                {
                    success = false;
                }
            }
            else
            {
                // Found file: Copy
                fs::copy_file(pathInSource, pathInDest);
            }
        }
        catch(fs::filesystem_error& e)
        {
            ErrorLogger::Log(string("ERROR: CopyDirectoryAndContents: ") + e.what());
            success = false;
        }
    }
    return success;
}