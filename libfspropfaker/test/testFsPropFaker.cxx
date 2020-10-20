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
 * File:   testFsPropFaker.cxx
 */

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "fspropfaker.h"
#include "fsutil.h"

#include "testutil.h"

#include <iostream>

namespace fspf
{

using std::shared_ptr;
using std::unique_ptr;
using std::make_unique;

namespace testing
{


TEST_CASE("PropFaker, testConstruct")
{
	const std::string sMountName = "fspf-iniz";
	const std::string sFsFolderPath = "/tmp/fspropfaker-iniz/iniz-base";
	const std::string sMountPath = "/tmp/fspropfaker-iniz/iniz-mount";
	const std::string sLogFilePath = "/tmp/fspropfaker-iniz/iniz.log";
	std::string sResult;
	std::string sError;
	bool bOk = execCmd("rm -rf /tmp/fspropfaker-iniz", sResult, sError);
	if (! bOk) {
		std::cout << "Could not remove folder: " << sError << '\n';
	}
	REQUIRE(bOk);
	makePath(sFsFolderPath);
	std::string sCmd = std::string{"touch "} + sFsFolderPath + "/some.txt";
	bOk = execCmd(sCmd.c_str(), sResult, sError);
	REQUIRE(bOk);
// std::cout << "1111 " << '\n';
	if (! sMountPath.empty()) {
		makePath(sMountPath);
	}

// std::cout << "2222 " << '\n';
	auto oResult = FsPropFaker::create(sMountName, sFsFolderPath, sMountPath, sLogFilePath);
	auto& refFaker = oResult.m_refFaker;
	sError = std::move(oResult.m_sError);
	REQUIRE(refFaker);
	REQUIRE(sError.empty());
//std::cout << "Mount path is " << refFaker->getMountPath() << '\n';

	REQUIRE(fileExists(refFaker->getMountPath() + "/some.txt"));

	sCmd = std::string{"touch "} + sFsFolderPath + "/other.txt";
	bOk = execCmd(sCmd.c_str(), sResult, sError);
	REQUIRE(bOk);

	REQUIRE(fileExists(refFaker->getMountPath() + "/other.txt"));

	sError = refFaker->unmount();
	REQUIRE(sError.empty());

	::sleep(1);

	REQUIRE(! fileExists(refFaker->getMountPath() + "/some.txt"));

	REQUIRE(! fileExists(refFaker->getMountPath() + "/other.txt"));
}


TEST_CASE("PropFaker, testFakeSize")
{
	const std::string sMountName = "fspf-size";
	const std::string sFsFolderPath = "/tmp/fspropfaker-size/size-base";
	const std::string sMountPath = "/tmp/fspropfaker-size/size-mount";
	const std::string sLogFilePath = "/tmp/fspropfaker-size/size.log";
	std::string sResult;
	std::string sError;
	bool bOk = execCmd("rm -rf /tmp/fspropfaker-size", sResult, sError);
	if (! bOk) {
		std::cout << "Could not remove folder: " << sError << '\n';
	}
	REQUIRE(bOk);
	makePath(sFsFolderPath);
	std::string sCmd = std::string{"touch "} + sFsFolderPath + "/some.txt";
	bOk = execCmd(sCmd.c_str(), sResult, sError);
	REQUIRE(bOk);
// std::cout << "1111 " << '\n';
	if (! sMountPath.empty()) {
		makePath(sMountPath);
	}

	auto oResult = FsPropFaker::create(sMountName, sFsFolderPath, sMountPath, sLogFilePath);
	auto& refFaker = oResult.m_refFaker;
	sError = std::move(oResult.m_sError);
	REQUIRE(refFaker);
	REQUIRE(sError.empty());
	std::cout << "Mount path is " << refFaker->getMountPath() << '\n';

	const auto nBlockSize = refFaker->getBlockSize();

	refFaker->setFakeDiskSizeInBlocks(100);

	::sleep(1);

	struct ::statvfs oStatFs;
	sError = getStatVFS(refFaker->getMountPath(), oStatFs);
	if (! sError.empty()) {
		std::cout << "getStatVFS error " << sError << '\n';
	}
	REQUIRE(sError.empty());

	REQUIRE(oStatFs.f_frsize == nBlockSize);
	REQUIRE(oStatFs.f_blocks == 100);

	// The following only makes sense on a non busy disk.
	// In the root partition services continuously create and remove files.
	// So this test would likely fail.
	//const auto nRealSizeInBlocks = refFaker->getRealDiskSizeInBlocks();
	//refFaker->setFakeDiskSizeDiffInBlocks(0);
	//
	//sError = getStatVFS(refFaker->getMountPath(), oStatFs);
	//if (! sError.empty()) {
	//	std::cout << "getStatVFS error " << sError << '\n';
	//}
	//REQUIRE(sError.empty());
	//
	//REQUIRE(oStatFs.f_blocks == nRealSizeInBlocks);
}

TEST_CASE("PropFaker, testFakeFree")
{
	const std::string sMountName = "fspf-size";
	const std::string sFsFolderPath = "/tmp/fspropfaker-free/free-base";
	const std::string sMountPath = "/tmp/fspropfaker-free/free-mount";
	const std::string sLogFilePath = "/tmp/fspropfaker-free/free.log";
	std::string sResult;
	std::string sError;
	bool bOk = execCmd("rm -rf /tmp/fspropfaker-free", sResult, sError);
	if (! bOk) {
		std::cout << "Could not remove folder: " << sError << '\n';
	}
	REQUIRE(bOk);
	makePath(sFsFolderPath);
	std::string sCmd = std::string{"touch "} + sFsFolderPath + "/some.txt";
	bOk = execCmd(sCmd.c_str(), sResult, sError);
	REQUIRE(bOk);
// std::cout << "1111 " << '\n';
	if (! sMountPath.empty()) {
		makePath(sMountPath);
	}

	auto oResult = FsPropFaker::create(sMountName, sFsFolderPath, sMountPath, sLogFilePath);
	auto& refFaker = oResult.m_refFaker;
	sError = std::move(oResult.m_sError);
	REQUIRE(refFaker);
	REQUIRE(sError.empty());
	std::cout << "Mount path is " << refFaker->getMountPath() << '\n';

	const auto nBlockSize = refFaker->getBlockSize();

	refFaker->setFakeDiskFreeSizeInBlocks(10);

	::sleep(1);

	struct ::statvfs oStatFs;
	sError = getStatVFS(refFaker->getMountPath(), oStatFs);
	if (! sError.empty()) {
		std::cout << "getStatVFS error " << sError << '\n';
	}
	REQUIRE(sError.empty());

	REQUIRE(oStatFs.f_frsize == nBlockSize);
	REQUIRE(oStatFs.f_bavail == 10);
}



} // namespace testing

} // namespace fspf
