#!/usr/bin/env python3

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

# File:   dd_install_fspropfaker-all.py

# Compiles and installs all the projects (binaries) contained in this source package
# taking into account the ordering of their dependencies.
# In addition to the pk_install_fspropfaker-all.py parameters it sets the DESTDIR
# environment variable

import sys
import os
import subprocess

def main():
	sImpossibleDir = "//////////"

	import argparse
	oParser = argparse.ArgumentParser(description="Install all fspropfaker projects.\n"
						"  Same as pk_install_fspropfaker-all.py but sets DESTDIR environment variable.\n"
						"  Used to build Debian binary packages."
						, formatter_class=argparse.RawDescriptionHelpFormatter)

	oParser.add_argument("--installdir", help="install dir (default=/usr/local)", metavar='INSTALLDIR'\
						, default="/usr/local", dest="sInstallDir")
	oParser.add_argument("--destdir", help="sets DESTDIR used by make install (default=${pwd}/debian/tmp)", metavar='DESTDIR'\
						, default=sImpossibleDir, dest="sDestDir")

	oParser.add_argument("-s", "--staticlib", help="build static libraries (instead of shared)", choices=['On', 'Off', 'Cache']\
						, default="Cache", dest="sBuildStaticLib")
	oParser.add_argument("-b", "--buildtype", help="build type (default=Release)"\
						, choices=['Debug', 'Release', 'MinSizeRel', 'RelWithDebInfo']\
						, default="Release", dest="sBuildType")
	oParser.add_argument("-t", "--tests", help="build tests (default=Cache)", choices=['On', 'Off', 'Cache']\
						, default="Cache", dest="sBuildTests")
	oParser.add_argument("-d", "--docs", help="build documentation (default=Cache)", choices=['On', 'Off', 'Cache']\
						, default="Cache", dest="sBuildDocs")
	oParser.add_argument("--docs-to-log", help="--docs warnings to log file", action="store_true"\
						, default=False, dest="bDocsWarningsToLog")
	#oParser.add_argument("--omit-plugins", help="do not build stmm-input-dl project", action="store_true"\
						#, default=False, dest="bOmitPlugins")
	oParser.add_argument("--no-configure", help="don't configure", action="store_true"\
						, default=False, dest="bDontConfigure")
	oParser.add_argument("--no-make", help="don't make", action="store_true"\
						, default=False, dest="bDontMake")
	oParser.add_argument("--no-install", help="don't install", action="store_true"\
						, default=False, dest="bDontInstall")
	oParser.add_argument("--no-sudo", help="don't use sudo to install", action="store_true"\
						, default=False, dest="bDontSudo")
	oParser.add_argument("--sanitize", help="compile libraries with -fsanitize=address (Debug only)", action="store_true"\
						, default=False, dest="bSanitize")
	oArgs = oParser.parse_args()

	sDestDir = oArgs.sDestDir
	if sDestDir == sImpossibleDir:
		sCallerDir = os.path.abspath(os.getcwd())
		sDestDir = sCallerDir + "/debian/tmp"
	else:
		sDestDir = os.path.abspath(os.path.expanduser(sDestDir))

	try:
		os.environ["DESTDIR"] = sDestDir
	except KeyError:
		raise RuntimeError("Couldn't set environment variable DESTDIR to: " + sDestDir)

	sInstallDir = os.path.abspath(os.path.expanduser(oArgs.sInstallDir))

	sPrivateScriptDir = os.path.dirname(os.path.abspath(__file__))
	#print("sScriptDir:" + sScriptDir)
	os.chdir(sPrivateScriptDir)

	#
	sBuildStaticLib = "-s " + oArgs.sBuildStaticLib
	#print("sBuildStaticLib:" + sBuildStaticLib)
	#
	sBuildTests = "-t " + oArgs.sBuildTests
	#print("sBuildTests:" + sBuildTests)
	#
	sBuildDocs = "-d " + oArgs.sBuildDocs
	#print("sBuildDocs:" + sBuildDocs)
	#
	if oArgs.bDocsWarningsToLog:
		sDocsWarningsToLog = "--docs-to-log"
	else:
		sDocsWarningsToLog = ""
	#
	sInstallDir = "--installdir " + sInstallDir
	#print("sInstallDir:" + sInstallDir)
	#
	sBuildType = "-b " + oArgs.sBuildType
	#print("sBuildType:" + sBuildType)
	#
	if oArgs.bSanitize:
		sSanitize = "--sanitize"
	else:
		sSanitize = ""
	#print("sSanitize:" + sSanitize)

	#
	if oArgs.bDontConfigure:
		sNoConfigure = "--no-configure"
	else:
		sNoConfigure = ""
	#
	if oArgs.bDontMake:
		sNoMake = "--no-make"
	else:
		sNoMake = ""
	#
	if oArgs.bDontInstall:
		sNoInstall = "--no-install"
	else:
		sNoInstall = ""
	#
	if oArgs.bDontSudo:
		sSudo = "--no-sudo"
	else:
		sSudo = ""

	subprocess.check_call("./pk_install_fspropfaker-all.py {} {} {} {} {} {} {} {} {} {} {}".format(\
			sBuildStaticLib, sBuildTests, sBuildDocs, sDocsWarningsToLog, sBuildType, sInstallDir\
			, sNoConfigure, sNoMake, sNoInstall, sSudo, sSanitize).split())



if __name__ == "__main__":
	main()

