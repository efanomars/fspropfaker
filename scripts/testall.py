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

# File:   testall.py

# - compiles and installs all the projects (binaries) contained in this source package
#   for both Debug and Release configurations and for both shared and static libs
# - compiles the examples
# - creates documentation and checks it is ok
# - runs all tests
# - makes sure no line in the code starts with std::cout
# - optionally calls clang-llvm sanitizer
# - optionally calls clang-tidy

import sys
import os
import subprocess


def testAll(sBuildType, sInstallDir, sSudo, bBuildStatic, bSanitize):
	sStatic = ""
	if bBuildStatic:
		sStatic = "-s On"
	else:
		sStatic = "-s Off"

	if bSanitize:
		sSanitize = "--sanitize"
		sSanitizeOptions = "ASAN_OPTIONS=check_initialization_order=1:strict_init_order=1:detect_odr_violation=1"
		# ":alloc_dealloc_mismatch=1:detect_leaks=1:allow_user_segv_handler=1:new_delete_type_mismatch=1
		sSanitizeIgnoreOptions = ":detect_leaks=0:new_delete_type_mismatch=0"
	else:
		sSanitize = ""
		sSanitizeOptions = ""
		sSanitizeIgnoreOptions = ""

	subprocess.check_call("./scripts/uninstall_fspropfaker-all.py -y --installdir {}  {}".format(sInstallDir, sSudo).split())

	subprocess.check_call("./scripts/install_fspropfaker-all.py -b {}  -t On  -d On --docs-to-log  --installdir {}  {} {} {}"\
			.format(sBuildType, sInstallDir, sSudo, sStatic, sSanitize).split())


	os.chdir("libfspropfaker/build")
	subprocess.check_call(sSanitizeOptions + " make test", shell=True)
	if os.path.getsize("libfspropfaker_doxy.log") > 0:
		raise RuntimeError("Error: libfspropfaker_doxy.log not empty")
	os.chdir("../..")

	if not bSanitize:
		pass



def checkTidy():
	#
	print("-------checkTidy ")
	subprocess.check_call("./scripts/priv/checktidy.py".split())
	

def main():

	import argparse
	oParser = argparse.ArgumentParser(description="Uninstall, compile, document, reinstall and test all projects.\n"\
						+ "")
	oParser.add_argument("-y", "--no-prompt", help="no prompt comfirmation", action="store_true"\
						, default=False, dest="bNoPrompt")
	oParser.add_argument("--installdir", help="install dir (default=/usr/local)", metavar='INSTALLDIR'\
						, default="/usr/local", dest="sInstallDir")
	oParser.add_argument("--no-sudo", help="don't use sudo to (un)install", action="store_true"\
						, default=False, dest="bDontSudo")
	oParser.add_argument("-b", "--buildtype", help="build type (default=Both)", choices=['Debug', 'Release', 'Both']\
						, default="Both", dest="sBuildType")
	oParser.add_argument("-l", "--link", help="build static library or shared", choices=['Static', 'Shared', 'Both']\
						, default="Both", dest="sLinkType")
	oParser.add_argument("--sanitize", help="execute tests with llvm address sanitize checks (Debug+Static only)", action="store_true"\
						, default=False, dest="bSanitize")
	oParser.add_argument("--tidy", help="apply clang-tidy to source code", action="store_true"\
						, default=False, dest="bTidy")
	oArgs = oParser.parse_args()

	while not oArgs.bNoPrompt:
		sAnswer = input("Are you sure? (yes/no) >").strip()
		if sAnswer.casefold() == "yes":
			break
		elif sAnswer.casefold() == "no":
			return #-----------------------------------------------------------

	print("-----------------------------------")
	print("testall.py (fspropfaker) started!")
	print("-----------------------------------")

	sInstallDir = os.path.abspath(os.path.expanduser(oArgs.sInstallDir))

	sScriptDir = os.path.dirname(os.path.abspath(__file__))
	os.chdir(sScriptDir)
	os.chdir("..")

	subprocess.check_call("./share/python/checkcode.py".split())

	if oArgs.bDontSudo:
		sSudo = "--no-sudo"
	else:
		sSudo = ""

	bTidyDone = False  # tidy should be applied only once for the same build type

	if (oArgs.sLinkType == "Both") or (oArgs.sLinkType == "Static"):
		bBuildStatic = True
		if oArgs.sBuildType == "Both":
			if oArgs.bSanitize:
				testAll("Debug", sInstallDir, sSudo, bBuildStatic, True)
			#
			testAll("Debug", sInstallDir, sSudo, bBuildStatic, False)
			if oArgs.bTidy:
				checkTidy()
			#
			testAll("Release", sInstallDir, sSudo, bBuildStatic, False)
			if oArgs.bTidy:
				checkTidy()
				bTidyDone = True
		else:
			if oArgs.bSanitize and (oArgs.sBuildType == "Debug"):
				testAll("Debug", sInstallDir, sSudo, bBuildStatic, True)
			#
			testAll(oArgs.sBuildType, sInstallDir, sSudo, bBuildStatic, False)
			if oArgs.bTidy:
				checkTidy()
				bTidyDone = True

	if (oArgs.sLinkType == "Both") or (oArgs.sLinkType == "Shared"):
		bBuildStatic = False
		if oArgs.sBuildType == "Both":
			testAll("Debug", sInstallDir, sSudo, bBuildStatic, False)
			if oArgs.bTidy and not bTidyDone:
				checkTidy()
			#
			testAll("Release", sInstallDir, sSudo, bBuildStatic, False)
			if oArgs.bTidy and not bTidyDone:
				checkTidy()
		else:
			testAll(oArgs.sBuildType, sInstallDir, sSudo, bBuildStatic, False)
			if oArgs.bTidy and not bTidyDone:
				checkTidy()

	print("-------------------------------------------------")
	print("testall.py (fspropfaker) finished successfully!")
	print("-------------------------------------------------")


if __name__ == "__main__":
	main()

