#ifndef _FILESYSTEM_UTILS_H_
#define _FILESYSTEM_UTILS_H_

#include "boost/filesystem.hpp"

bool CopyDirectoryAndContents(  const boost::filesystem::path &source,
                                const boost::filesystem::path &dest );

#endif //_FILESYSTEM_UTILS_H_