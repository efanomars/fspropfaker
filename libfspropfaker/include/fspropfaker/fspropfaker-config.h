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
 * File:   fspropfaker-config.h
 */

#ifndef FSPROPFAKER_LIB_CONFIG_H
#define FSPROPFAKER_LIB_CONFIG_H

namespace stmi
{

namespace libconfig
{

namespace au
{

/** The fspropfaker library version.
 * @return The version string. Cannot be empty.
 */
const char* getVersion() noexcept;

} // namespace au

} // namespace libconfig

} // namespace stmi

#endif /* FSPROPFAKER_LIB_CONFIG_H */

