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
 * File:   testutil.h
 */

#ifndef FSPF_TEST_UTIL_H
#define FSPF_TEST_UTIL_H

#include <string>
#include <vector>

namespace fspf
{

/* Create the path if it doesn't exist.
 * @param sPath The path.
 * @return Empty string if no error, otherwise error.
 */
std::string makePath(const std::string& sPath) noexcept;

bool execCmd(const char* sCmd, std::string& sResult, std::string& sError) noexcept;

bool fileExists(const std::string& sPath) noexcept;

} // namespace fspf

#endif /* FSPF_TEST_UTIL_H */
