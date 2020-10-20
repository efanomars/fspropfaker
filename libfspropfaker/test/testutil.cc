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
 * File:   testutil.cc
 */

#include "testutil.h"

#include <string>
#include <iostream>
#include <cassert>
#include <array>

#include <string.h>
#include <stdlib.h>

#include <errno.h>
#include <sys/stat.h>

namespace fspf
{

// Modified from https://stackoverflow.com/questions/675039/how-can-i-create-directory-tree-in-c-linux
typedef struct stat Stat;

namespace Private
{
static std::string makeDir(const char* p0Dir) noexcept
{
	Stat oStat;

	if (::stat(p0Dir, &oStat) != 0) {
		// Directory does not exist. EEXIST for race condition.
		const auto nRet = ::mkdir(p0Dir, 0755); // rwx r-x r-x
		if (nRet != 0) {
			if (errno != EEXIST) {
				return std::string{"Error: Could not create directory " + std::string(p0Dir)};
			}
		}
	} else if (!S_ISDIR(oStat.st_mode)) {
		return std::string{"Error: " + std::string(p0Dir) + " not a directory"};
	}
	return "";
}
}
std::string makePath(const std::string& sPath) noexcept
{
	std::string sWorkPath = sPath;
	std::string::size_type nBasePos = 0;
	do {
		const auto nNewPos = sWorkPath.find('/', nBasePos);
		if (nNewPos == std::string::npos) {
			auto sError = Private::makeDir(sWorkPath.c_str());
			if (! sError.empty()) {
				return sError;
			}
			break; // do ------
		} else if (nBasePos != nNewPos) {
			// not root or double slash
			sWorkPath[nNewPos] = '\0';
			auto sError = Private::makeDir(sWorkPath.c_str());
			if (! sError.empty()) {
				return sError;
			}
			sWorkPath[nNewPos] = '/';
		}
		nBasePos = nNewPos + 1;
	} while (true);
	return "";
}

bool execCmd(const char* sCmd, std::string& sResult, std::string& sError) noexcept
{
	::fflush(nullptr);
	sError.clear();
	sResult.clear();
	std::array<char, 128> aBuffer;
	FILE* pFile = ::popen(sCmd, "r");
	if (pFile == nullptr) {
		sError = std::string("Error: popen() failed: ") + ::strerror(errno) + "(" + std::to_string(errno) + ")";
		return false; //--------------------------------------------------------
	}
	while (!::feof(pFile)) {
		if (::fgets(aBuffer.data(), sizeof(aBuffer), pFile) != nullptr) {
			sResult += aBuffer.data();
		}
	}
	const auto nRet = ::pclose(pFile);
	const bool bOk = (nRet != -1);
	if (! bOk) {
		sError = std::string("Error: pclose() failed: ") + ::strerror(errno) + "(" + std::to_string(errno) + ")";
	}
	return bOk;
}

bool fileExists(const std::string& sPath) noexcept
{
	struct ::stat oInfo;
	if (::stat(sPath.c_str(), &oInfo) != 0) {
		return false;
	}
	return ((oInfo.st_mode & S_IFREG) != 0);
}

} // namespace fspf
