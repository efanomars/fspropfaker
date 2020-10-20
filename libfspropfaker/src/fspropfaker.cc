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
 * File:   fspropfaker.cc
 */

#include "fspropfaker.h"

#include "overfs.h"
#include "fsutil.h"

#include <iostream>
#include <cassert>
#include <memory>
#include <string>
#include <mutex>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <signal.h>
#include <sys/statvfs.h>


using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;

namespace fspf
{

static constexpr int32_t s_nMaxMountNameSize = 20;

FsPropFaker::CreateResult FsPropFaker::create(const std::string& sMountName
											, const std::string& sFsFolderPath
											, const std::string& sMountPath
											, const std::string& sLogFilePath) noexcept
{
	assert(! sFsFolderPath.empty());
	CreateResult oResult;
    if ((::getuid() == 0) || (::geteuid() == 0)) {
		oResult.m_sError = "Running as root not permitted";
		return oResult;
    }
	auto refFaker = std::unique_ptr<FsPropFaker>(new FsPropFaker());
	oResult.m_sError = refFaker->init(sMountName, sFsFolderPath, sMountPath, sLogFilePath);
	if (! oResult.m_sError.empty()) {
		return oResult;
	}
	oResult.m_sError = refFaker->createThread();
	if (! oResult.m_sError.empty()) {
		return oResult;
	}
	oResult.m_refFaker = std::move(refFaker);
	return oResult;
}

FsPropFaker::FsPropFaker() noexcept
{
}

FsPropFaker::~FsPropFaker() noexcept
{
	if (m_refFsThread) {
		unmount();
		m_refFsThread->join();
	}
}

const std::string& FsPropFaker::getMountName() const noexcept
{
	return m_sMountName;
}
const std::string& FsPropFaker::getRootPath() const noexcept
{
	return m_sRootPath;
}
const std::string& FsPropFaker::getMountPath() const noexcept
{
	return m_sMountPath;
}
const std::string& FsPropFaker::getLogFilePath() const noexcept
{
	return m_sLogFilePath;
}
int64_t FsPropFaker::getBlockSize() const noexcept
{
	return m_nBlockSize;
}

std::string FsPropFaker::init(const std::string& sMountName
							, const std::string& sFsFolderPath
							, const std::string& sMountPath
							, const std::string& sLogFilePath) noexcept
{
	m_sRootPath = realPath(sFsFolderPath);
	if (! dirExists(m_sRootPath)) {
		return std::string("Folder ") + m_sRootPath + " not found"; //----------
	}
	struct ::statvfs oStatFs;
	const std::string sErr = getStatVFS(m_sRootPath, oStatFs);
	if (! sErr.empty()) {
		return sErr; //---------------------------------------------------------
	}
//std::cout << "FsPropFaker::init bbb" << '\n';
	m_nBlockSize = static_cast<int64_t>(oStatFs.f_frsize);
	if (m_nBlockSize <= 0) {
		return std::string("Block size must be positive!"); //------------------
	}
	const auto nMountNameSize = sMountName.size();
	if (nMountNameSize == 0) {
		m_sMountName = "propfaker";
	} else if (static_cast<int32_t>(nMountNameSize) > s_nMaxMountNameSize) {
		return std::string("Mount name too long!"); //--------------------------
	} else {
		m_sMountName = sMountName;
	}
	if (sMountPath.empty()) {
		static const char* s_p0Template = "/tmp/fspropfakerXXXXXX";
		char sDirPath[PATH_MAX];
		::strcpy(sDirPath, s_p0Template);
		char* p0DirPath = ::mkdtemp(sDirPath);
		if (p0DirPath == nullptr) {
			return std::string("Temporary folder ") + p0DirPath + " could not be created"; //------
		}
		m_sMountPath = std::string{p0DirPath};
	} else {
		m_sMountPath = realPath(sMountPath);
		if (m_sMountPath.empty()) {
			return sMountPath + ": " + std::string("Error ") + std::string(::strerror(errno));
		}
		if (! dirExists(m_sMountPath)) {
			return std::string("Folder ") + m_sMountPath + " not found";
		}
	}
	if (! sLogFilePath.empty()) {
		m_sLogFilePath = sLogFilePath;
	}
//std::cout << "m_sRootPath " << m_sRootPath << '\n';
//std::cout << "m_sMountPath " << m_sMountPath << '\n';
	return "";
}

std::string FsPropFaker::createThread() noexcept
{
//std::cout << "FsPropFaker::createThread  starting thread" << '\n';
	auto oPair = OverFs::createInstance(this, [&](){ fuseInitialized(); });
	if (! oPair.second.empty()) {
		return oPair.second;
	}
	m_refFs = std::move(oPair.first);
	m_refFsThread = std::make_unique<std::thread>([&]()
	{
		// creates fuse filesystem
		const int nArgC = 3;
		char aPrg[s_nMaxMountNameSize + 1];
		::strcpy(aPrg, m_sMountName.c_str());
		char aMountPath[PATH_MAX];
		::strcpy(aMountPath, m_sMountPath.c_str());
		char aOptionF[2 + 1];
		::strcpy(aOptionF, "-f"); // so that run() blocks until unmounted
		//
		char* aArgV[3] = {aPrg, aOptionF, aMountPath};

		m_refFs->run(nArgC, aArgV);
		// now thread is ready to join in the destructor
	});
//std::cout << "FsPropFaker::createThread  waiting for fuse initialization" << '\n';
	// Wait for m_oFsThread to initialize file system
	{
		std::unique_lock<std::mutex> oLock(m_oFsStartedMutex);
		m_oFileSystemStarted.wait(oLock, [&](){ return (m_bFileSystemStarted != false); });
	}
//std::cout << "FsPropFaker::createThread  before sleep" << '\n';
	::sleep(2); // wait for the fuse system to be set up!

	return "";
}
void FsPropFaker::fuseInitialized() noexcept
{
	// signal main thread started
	{
		std::lock_guard<std::mutex> oLock(m_oFsStartedMutex);
		m_bFileSystemStarted = true;
	}
	m_oFileSystemStarted.notify_one();
}
// from https://code.i-harness.com/en/q/ea7f34
template <typename T>
using stripwhat = typename std::remove_pointer<typename std::decay<T>::type>::type;
std::string FsPropFaker::unmount() noexcept
{
	static_assert(std::is_same<stripwhat<std::thread::native_handle_type>, pthread_t>::value
				, "libstdc++ doesn't use pthread_t");
	const auto nRet = ::pthread_kill(m_refFsThread->native_handle(), SIGHUP);
	if (nRet == 0) {
		return "";
	}
	return ::strerror(nRet);
}

int64_t FsPropFaker::getRealDiskSizeInBlocks() noexcept
{
	return m_refFs->getRealDiskSizeInBlocks();
}
int64_t FsPropFaker::getRealDiskSizeInMB() noexcept
{
	return getRealDiskSizeInBlocks() * m_nBlockSize / s_nMegaByteBytes;
}
int64_t FsPropFaker::getRealFreeSizeInBlocks() noexcept
{
	return m_refFs->getRealFreeSizeInBlocks();
}
int64_t FsPropFaker::getRealFreeSizeInMB() noexcept
{
	return getRealFreeSizeInBlocks() * m_nBlockSize / s_nMegaByteBytes;
}

void FsPropFaker::setFakeDiskSizeInBlocks(int64_t nSizeBlocks) noexcept
{
	assert(nSizeBlocks > 0);
	m_refFs->setFakeDiskSizeInBlocks(nSizeBlocks);
}
int64_t FsPropFaker::setFakeDiskSizeInMB(int64_t nSizeMB) noexcept
{
	const int64_t nSizeBytes = nSizeMB * s_nMegaByteBytes;
	const int64_t nSizeBlocks = nSizeBytes / m_nBlockSize + ((nSizeBytes % m_nBlockSize) != 0 ? 1 : 0);
	setFakeDiskSizeInBlocks(nSizeBlocks);
	return nSizeBlocks;
}
void FsPropFaker::setFakeDiskSizeDiffInBlocks(int64_t nSizeBlocks) noexcept
{
	m_refFs->setFakeDiskSizeDiffInBlocks(nSizeBlocks);
}
int64_t FsPropFaker::setFakeDiskSizeDiffInMB(int64_t nSizeMB) noexcept
{
	const int64_t nSizeBytes = nSizeMB * s_nMegaByteBytes;
	const int32_t nCorr = (nSizeMB > 0) ? 1 : -1;
	const int64_t nSizeBlocks = nSizeBytes / m_nBlockSize + ((nSizeBytes % m_nBlockSize) != 0 ? nCorr : 0);
	setFakeDiskSizeDiffInBlocks(nSizeBlocks);
	return nSizeBlocks;
}

void FsPropFaker::setFakeDiskFreeSizeInBlocks(int64_t nFreeSizeBlocks) noexcept
{
	assert(nFreeSizeBlocks > 0);
	m_refFs->setFakeFreeSizeInBlocks(nFreeSizeBlocks);
}
int64_t FsPropFaker::setFakeDiskFreeSizeInMB(int64_t nFreeSizeMB) noexcept
{
	const int64_t nFreeSizeBytes = nFreeSizeMB * s_nMegaByteBytes;
	const int64_t nFreeSizeBlocks = nFreeSizeBytes / m_nBlockSize + ((nFreeSizeBytes % m_nBlockSize) != 0 ? 1 : 0);
	setFakeDiskFreeSizeInBlocks(nFreeSizeBlocks);
	return nFreeSizeBlocks;
}
void FsPropFaker::setFakeDiskFreeSizeDiffInBlocks(int64_t nFreeSizeBlocks) noexcept
{
	m_refFs->setFakeFreeSizeDiffInBlocks(nFreeSizeBlocks);
}
int64_t FsPropFaker::setFakeDiskFreeSizeDiffInMB(int64_t nFreeSizeMB) noexcept
{
	const int64_t nFreeSizeBytes = nFreeSizeMB * s_nMegaByteBytes;
	const int32_t nCorr = (nFreeSizeMB > 0) ? 1 : -1;
	const int64_t nFreeSizeBlocks = nFreeSizeBytes / m_nBlockSize + ((nFreeSizeBytes % m_nBlockSize) != 0 ? nCorr : 0);
	setFakeDiskFreeSizeDiffInBlocks(nFreeSizeBlocks);
	return nFreeSizeBlocks;
}


} // namespace fspf

