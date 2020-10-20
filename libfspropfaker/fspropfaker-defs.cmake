# Copyright © 2020  Stefano Marsili, <stemars@gmx.ch>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with this program; if not, see <http://www.gnu.org/licenses/>

# File:   fspropfaker-defs.cmake

# Libtool CURRENT/REVISION/AGE: here
#   MAJOR is CURRENT interface
#   MINOR is REVISION (implementation of interface)
#   AGE is always 0
set(FSPROPFAKER_MAJOR_VERSION 0)
set(FSPROPFAKER_MINOR_VERSION 1) # !-U-!
set(FSPROPFAKER_VERSION "${FSPROPFAKER_MAJOR_VERSION}.${FSPROPFAKER_MINOR_VERSION}.0")

set(FSPROPFAKER_REQ_FUSE_VERSION "2.9.7")

if ("${CMAKE_SCRIPT_MODE_FILE}" STREQUAL "")
    include(FindPkgConfig)
    if (NOT PKG_CONFIG_FOUND)
        message(FATAL_ERROR "Mandatory 'pkg-config' not found!")
    endif()
    # Beware! The prefix passed to pkg_check_modules(PREFIX ...) shouldn't contain underscores!
    pkg_check_modules(FUSE    REQUIRED  fuse>=${FSPROPFAKER_REQ_FUSE_VERSION})
endif()

# include dirs
list(APPEND FSPROPFAKER_EXTRA_INCLUDE_DIRS  "${FUSE_INCLUDE_DIRS}")

set(STMMI_TEMP_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/../libfspropfaker/include")

list(APPEND FSPROPFAKER_INCLUDE_DIRS  "${STMMI_TEMP_INCLUDE_DIR}")
list(APPEND FSPROPFAKER_INCLUDE_DIRS  "${FSPROPFAKER_EXTRA_INCLUDE_DIRS}")

# libs
set(        STMMI_TEMP_EXTERNAL_LIBRARIES    "")
list(APPEND STMMI_TEMP_EXTERNAL_LIBRARIES    "${FUSE_LIBRARIES}")

set(        FSPROPFAKER_EXTRA_LIBRARIES      "")
list(APPEND FSPROPFAKER_EXTRA_LIBRARIES      "${STMMI_TEMP_EXTERNAL_LIBRARIES}")

if (BUILD_SHARED_LIBS)
    set(STMMI_LIB_FILE "${PROJECT_SOURCE_DIR}/../libfspropfaker/build/libfspropfaker.so")
else()
    set(STMMI_LIB_FILE "${PROJECT_SOURCE_DIR}/../libfspropfaker/build/libfspropfaker.a")
endif()

list(APPEND FSPROPFAKER_LIBRARIES "${STMMI_LIB_FILE}")
list(APPEND FSPROPFAKER_LIBRARIES "${FSPROPFAKER_EXTRA_LIBRARIES}")

if ("${CMAKE_SCRIPT_MODE_FILE}" STREQUAL "")
    DefineAsSecondaryTarget(fspropfaker  ${STMMI_LIB_FILE}  "${FSPROPFAKER_INCLUDE_DIRS}"  "" "${STMMI_TEMP_EXTERNAL_LIBRARIES}")
endif()
