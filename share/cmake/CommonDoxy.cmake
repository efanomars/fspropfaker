# Copyright © 2018-2020  Stefano Marsili, <stemars@gmx.ch>
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

# File:   CommonDoxy.cmake

# See first three params of CreateLibDoxyDetailed
function(CreateLibDoxy STMMI_TARGET_LIB  STMMI_TARGET_LIB_VERSION  STMMI_INCLUDE_LIBS)

    set(STMMI_DOC_DIRS_AND_FILES)
    list(APPEND STMMI_DOC_DIRS_AND_FILES
            "${PROJECT_SOURCE_DIR}/include"
            "${PROJECT_SOURCE_DIR}/doc"
            "${PROJECT_SOURCE_DIR}/README.md"
            )
    CreateLibDoxyDetailed("${STMMI_TARGET_LIB}" "${STMMI_TARGET_LIB_VERSION}" "${STMMI_INCLUDE_LIBS}" "${STMMI_DOC_DIRS_AND_FILES}" 
                          "html" "docu-config.doxygen" "OFF")

endfunction()

# STMMI_TARGET_LIB              The library name. Example: stmm-input-gtk
# STMMI_TARGET_LIB_VERSION      The library version. Example: 0.1.0
#                               The value is used by the configure_file command
# STMMI_INCLUDE_LIBS            The other libraries that should be added to the docu.
#                               If one is not from this source package it looks in system paths
#                               or an installed one (which hopefully has commented headers).
#                               Example: "stmm-input-ev;stmm-input"
# STMMI_DOC_DIRS_AND_FILES_IN   The additional files and directories to include in the documentation.
# STMMI_TARGET_OUTPUT_DIR       Either html or htmlall
# STMMI_DOXYGEN_CONFIG          Either docu-config.doxygen or docu-config-all.doxygen
# STMMI_FORCE_BUILD_DOCS        Override the BUILD_DOC variable (either ON or OFF)
function(CreateLibDoxyDetailed STMMI_TARGET_LIB  STMMI_TARGET_LIB_VERSION  STMMI_INCLUDE_LIBS
                               STMMI_DOC_DIRS_AND_FILES_IN  STMMI_TARGET_OUTPUT_DIR
                               STMMI_DOXYGEN_CONFIG  STMMI_FORCE_BUILD_DOCS)

    # Add an option for the user to enable or not the documentation generation every time we compile.
    # Either set it on the command line or use cmake-gui. It is an advanced option.
    option(BUILD_DOCS "Build doxygen documentation for ${STMMI_TARGET_LIB}" OFF)
    mark_as_advanced(BUILD_DOCS)

    option(BUILD_DOCS_WARNINGS_TO_LOG_FILE "Doxygen warnings for ${STMMI_TARGET_LIB} written to log file" OFF)
    mark_as_advanced(BUILD_DOCS_WARNINGS_TO_LOG_FILE)

if ($ENV{STMM_CMAKE_COMMENTS})
message(STATUS "CreateLibDoxyDetailed:")
message(STATUS "    BUILD_DOCS                    ${BUILD_DOCS}")
message(STATUS "    STMMI_TARGET_LIB              ${STMMI_TARGET_LIB}")
message(STATUS "    STMMI_TARGET_LIB_VERSION      ${STMMI_TARGET_LIB_VERSION}")
message(STATUS "    STMMI_INCLUDE_LIBS            ${STMMI_INCLUDE_LIBS}")
message(STATUS "    STMMI_DOC_DIRS_AND_FILES_IN   ${STMMI_DOC_DIRS_AND_FILES_IN}")
message(STATUS "    STMMI_TARGET_OUTPUT_DIR       ${STMMI_TARGET_OUTPUT_DIR}")
message(STATUS "    STMMI_DOXYGEN_CONFIG          ${STMMI_DOXYGEN_CONFIG}")
message(STATUS "    STMMI_FORCE_BUILD_DOCS        ${STMMI_FORCE_BUILD_DOCS}")
endif()

    set(STMMI_DOC_DIRS_AND_FILES ${STMMI_DOC_DIRS_AND_FILES_IN})
    list(LENGTH STMMI_INCLUDE_LIBS STMMI_INCLUDE_LIBS_LEN)
#message(STATUS "CreateLibDoxyDetailed STMMI_INCLUDE_LIBS_LEN      ${STMMI_INCLUDE_LIBS_LEN}")

    if (STMMI_INCLUDE_LIBS_LEN GREATER 0)
        set(STMMI_INCLUDE_LIBS_TEMP ${STMMI_INCLUDE_LIBS})
        string(REPLACE ";" ", " STMMI_INCLUDE_LIBS_TEMP "${STMMI_INCLUDE_LIBS_TEMP}")
        #
        option(BUILD_DOCS_INCLUDE_LIBS "Include the ${STMMI_INCLUDE_LIBS_TEMP} headers into doxygen documentation" ON)
        mark_as_advanced(BUILD_DOCS_INCLUDE_LIBS)

        if ((BUILD_DOCS OR STMMI_FORCE_BUILD_DOCS) AND BUILD_DOCS_INCLUDE_LIBS)
            foreach (STMMI_INCLUDE_CUR_LIB  ${STMMI_INCLUDE_LIBS})
                #
                set(STMMI_INCLUDE_CUR_LIB_PATH "${PROJECT_SOURCE_DIR}/../lib${STMMI_INCLUDE_CUR_LIB}/include/${STMMI_INCLUDE_CUR_LIB}")
                if (EXISTS ${STMMI_INCLUDE_CUR_LIB_PATH})
                    if (IS_DIRECTORY ${STMMI_INCLUDE_CUR_LIB_PATH})
                        list(APPEND STMMI_DOC_DIRS_AND_FILES   ${STMMI_INCLUDE_CUR_LIB_PATH})
                    endif()
                else()
                    # ensure the search is really repeated for the current libraries
                    # by setting the result variable to -NOTFOUND
                    set(STMMITEMPCURLIB "STMMITEMPCURLIB-NOTFOUND")
                    find_file(STMMITEMPCURLIB  "${STMMI_INCLUDE_CUR_LIB}"
                                NO_CMAKE_PATH  NO_CMAKE_ENVIRONMENT_PATH)
                    if ("${STMMITEMPCURLIB}" STREQUAL "STMMITEMPCURLIB-NOTFOUND")
                        message(FATAL_ERROR "Couldn't find include directory '${STMMI_INCLUDE_CUR_LIB}'")
                    else()

                        list(APPEND STMMI_DOC_DIRS_AND_FILES   ${STMMITEMPCURLIB})

                    endif()
                    mark_as_advanced(STMMITEMPCURLIB)
                endif()
            endforeach()
        endif()
    endif()

    if (BUILD_DOCS  OR  STMMI_FORCE_BUILD_DOCS)
        string(REPLACE ";" " " STMMI_DOC_DIRS_AND_FILES "${STMMI_DOC_DIRS_AND_FILES}")

        if (BUILD_DOCS_WARNINGS_TO_LOG_FILE)
            set(STMMI_DOXY_WARNING_LOG_FILE "${CMAKE_BINARY_DIR}/lib${STMMI_TARGET_LIB}_doxy.log")
        else (BUILD_DOCS_WARNINGS_TO_LOG_FILE)
            set(STMMI_DOXY_WARNING_LOG_FILE)
        endif (BUILD_DOCS_WARNINGS_TO_LOG_FILE)

        # Configure the doxygen config file with current settings:
        configure_file("${PROJECT_SOURCE_DIR}/../share/doc/docu-config.doxygen.in"  "${CMAKE_BINARY_DIR}/${STMMI_DOXYGEN_CONFIG}" @ONLY)

        set(STMMI_DOC_TARGET "doc")
        if (TARGET doc)
            set(STMMI_DOC_TARGET "doc-all")
        endif()

        add_custom_target(${STMMI_DOC_TARGET}
            ${DOXYGEN_EXECUTABLE} "${CMAKE_BINARY_DIR}/${STMMI_DOXYGEN_CONFIG}"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Doxygen: generating API documentation for ${STMMI_TARGET_LIB}" VERBATIM)

        if ("${STMMI_DOC_TARGET}" STREQUAL "doc-all")
            add_dependencies(doc doc-all)
        endif()

        make_directory(${CMAKE_BINARY_DIR}/${STMMI_TARGET_OUTPUT_DIR})

        install(DIRECTORY "${CMAKE_BINARY_DIR}/${STMMI_TARGET_OUTPUT_DIR}/html"
                DESTINATION "share/doc/lib${STMMI_TARGET_LIB}"
                PATTERN "*.md5" EXCLUDE)
    endif()

endfunction()
