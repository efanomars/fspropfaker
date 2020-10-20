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

# File:   libfspropfaker/CMakeLists.txt

cmake_minimum_required(VERSION 3.0)

project(fspropfaker CXX)

set(LIBRARY_OUTPUT_DIRECTORY "build")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../share/cmake")

include(CommonUtil)

CheckBinaryNotSourceTree()
CheckBuildType()
DefineSharedLibOption()
DefineCommonOptions()
DefineCommonCompileOptions("c++14")

# Headers dir
set(STMMI_INCLUDE_DIR  "${PROJECT_SOURCE_DIR}/include")
set(STMMI_HEADERS_DIR  "${STMMI_INCLUDE_DIR}/fspropfaker")

set(STMMI_HEADERS
        "${STMMI_HEADERS_DIR}/fspropfaker.h"
        "${STMMI_HEADERS_DIR}/fspropfaker-config.h"
        )
#
# Sources dir
set(STMMI_SOURCES_DIR  "${PROJECT_SOURCE_DIR}/src")
# Source files (and headers only used for building)
set(STMMI_SOURCES
        "${STMMI_SOURCES_DIR}/fslogger.h"
        "${STMMI_SOURCES_DIR}/fslogger.cc"
        "${STMMI_SOURCES_DIR}/fspropfaker.cc"
        "${STMMI_SOURCES_DIR}/fsutil.h"
        "${STMMI_SOURCES_DIR}/fsutil.cc"
        "${STMMI_SOURCES_DIR}/overfs.h"
        "${STMMI_SOURCES_DIR}/overfs.cc"
        )

# Define library
add_library(fspropfaker ${STMMI_SOURCES} "${PROJECT_BINARY_DIR}/fspropfaker-config.cc")

include("fspropfaker-defs.cmake")

target_include_directories(fspropfaker SYSTEM PUBLIC ${FSPROPFAKER_EXTRA_INCLUDE_DIRS})
target_include_directories(fspropfaker        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../share/thirdparty")
target_include_directories(fspropfaker BEFORE PUBLIC
  $<BUILD_INTERFACE:${STMMI_HEADERS_DIR}>
  $<INSTALL_INTERFACE:include/fspropfaker>  # <prefix>/include/fspropfaker
)

find_package(Threads REQUIRED)
target_link_libraries(fspropfaker ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries(fspropfaker ${FSPROPFAKER_EXTRA_LIBRARIES})

target_sources(fspropfaker PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../share/thirdparty/fusepp/Fuse.cpp")

set_target_properties(fspropfaker PROPERTIES  ${CMAKE_BUILD_TYPE}_POSTFIX "")
set_target_properties(fspropfaker PROPERTIES
        VERSION ${FSPROPFAKER_VERSION}
        SOVERSION ${FSPROPFAKER_MAJOR_VERSION}
#        OUTPUT_NAME "fspropfaker-${FSPROPFAKER_VERSION}"
        )

target_compile_definitions(fspropfaker PRIVATE  "_FILE_OFFSET_BITS=64")

DefineTargetPublicCompileOptions(fspropfaker)

# Set version for fspropfaker-config.cc.in
set(STMMI_PKG_VERSION "${FSPROPFAKER_VERSION}")
# Create config file for library
configure_file("${PROJECT_SOURCE_DIR}/src/fspropfaker-config.cc.in"
               "${PROJECT_BINARY_DIR}/fspropfaker-config.cc" @ONLY)

# Set install dir for pkg-config file
set(STMMI_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
# Create pkg-config file for the library
configure_file("${PROJECT_SOURCE_DIR}/fspropfaker.pc.in"
               "${PROJECT_BINARY_DIR}/fspropfaker.pc" @ONLY)


if ($ENV{STMM_CMAKE_COMMENTS})
message(STATUS "")
message(STATUS "fspropfaker was configured with the following options:")
message(STATUS " STMMI_HEADERS:                  ${STMMI_HEADERS}")
message(STATUS " STMMI_SOURCES:                  ${STMMI_SOURCES}")
message(STATUS " FSPROPFAKER_EXTRA_INCLUDE_DIRS: ${FSPROPFAKER_EXTRA_INCLUDE_DIRS}")
message(STATUS " FSPROPFAKER_EXTRA_LIBS:         ${FSPROPFAKER_EXTRA_LIBS}")
message(STATUS " CMAKE_BUILD_TYPE:               ${CMAKE_BUILD_TYPE}")
message(STATUS " CMAKE_CXX_COMPILER_ID:          ${CMAKE_CXX_COMPILER_ID}")
message(STATUS " CMAKE_CXX_COMPILER_VERSION:     ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS " CMAKE_CXX_FLAGS:                ${CMAKE_CXX_FLAGS}")
message(STATUS " install prefix:                 ${STMMI_INSTALL_PREFIX}")
message(STATUS " BUILD_DOCS:                     ${BUILD_DOCS}")
message(STATUS " BUILD_TESTING:                  ${BUILD_TESTING}")
endif()

# Documentation
add_subdirectory(doc)
if (BUILD_DOCS) #  OR ("$ENV{STMM_DEBIAN_PACKAGING}" STREQUAL "ON")
    # build automatically, otherwise call `make doc`
    add_dependencies(fspropfaker doc)
endif()

# Testing
enable_testing()
add_subdirectory(test)

install(TARGETS fspropfaker LIBRARY DESTINATION "lib"  ARCHIVE DESTINATION "lib")

install(FILES ${STMMI_HEADERS}   DESTINATION "include/fspropfaker")

install(FILES  "${PROJECT_BINARY_DIR}/fspropfaker.pc"   DESTINATION "lib/pkgconfig")

