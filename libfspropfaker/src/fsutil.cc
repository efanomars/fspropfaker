/*
 * Copyright © 2020  Stefano Marsili, <stemars@gmx.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>
 */
/*
 * File:   fsutil.cc
 */

#include "fsutil.h"

#include <iostream>
//#include <cassert>
#include <string>
#include <array>

#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <ios>

namespace fspf
{

bool dirExists(const std::string& sPath) noexcept
{
	struct ::stat oInfo;
	if (::stat(sPath.c_str(), &oInfo) != 0) {
		return false;
	}
	return ((oInfo.st_mode & S_IFDIR) != 0);
}

std::string realPath(const std::string& sPath) noexcept
{
	char aResolvedPath[PATH_MAX];
	char* p0Res = ::realpath(sPath.c_str(), aResolvedPath);
//std::cout << "realPath 1 " << sPath.c_str() << '\n';
	if (p0Res == nullptr) {
		return "";
	}
//std::cout << "realPath 2 " << '\n';
	return std::string{p0Res};
}

std::string getStatVFS(const std::string& sPath, struct ::statvfs& oStatFs) noexcept
{
	const int nRetStat = ::statvfs(sPath.c_str(), &oStatFs);
	if (nRetStat == 0) {
		return "";
	}
	return ::strerror(errno);
}

} // namespace fspf
