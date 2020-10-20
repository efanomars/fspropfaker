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
 * File:   overfs.cc
 */
/* This code is mostly copied (and modified) from the:
 *
 *  Big Brother File System
 *  Copyright (C) 2012 Joseph J. Pfeiffer, Jr., Ph.D. <pfeiffer@cs.nmsu.edu>
 *
  Which is distributed under the terms of the GNU GPLv3.
 */

#include "overfs.h"

#include "fspropfaker.h"
#include "fsutil.h"

#include <iostream>
#include <cassert>
#include <memory>
#include <string>
#include <cstdlib>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/xattr.h>


using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;

namespace fspf
{

static std::string getFullPath(const char* p0Path) noexcept
{
	OverFs* p0OverFs = OverFs::this_();
	FsPropFaker* p0FsPropFaker = p0OverFs->getFsPropFaker();
	return p0FsPropFaker->getRootPath() + p0Path;
}

std::pair<shared_ptr<OverFs>, std::string> OverFs::createInstance(FsPropFaker* p0FsPropFaker
																, std::function<void()>&& oCallback) noexcept
{
	assert(p0FsPropFaker != nullptr);
	auto refFs = shared_ptr<OverFs>(new OverFs(p0FsPropFaker, std::move(oCallback)));
	std::string sErr = refFs->initInstance();
	if (! sErr.empty()) {
		return std::make_pair(shared_ptr<OverFs>{}, sErr);
	}
	return std::make_pair(refFs, "");
}

OverFs::OverFs(FsPropFaker* p0FsPropFaker, std::function<void()>&& oCallback) noexcept
: Fusepp::Fuse<OverFs>()
, m_p0FsPropFaker(p0FsPropFaker)
, m_sMountName(p0FsPropFaker->getMountName())
, m_sRootPath(p0FsPropFaker->getRootPath())
, m_sLogFilePath(p0FsPropFaker->getLogFilePath())
, m_nBlockSize(p0FsPropFaker->getBlockSize())
, m_oCallback(std::move(oCallback))
{
}
std::string OverFs::initInstance() noexcept
{
	auto oPair = FsLogger::create(m_sMountName, m_sLogFilePath);
	if (! oPair.second.empty()) {
		return oPair.second;
	}
	m_refLogger = std::move(oPair.first);
	return "";
}

FsPropFaker* OverFs::getFsPropFaker() const noexcept
{
	return m_p0FsPropFaker;
}
FsLogger* OverFs::getFsLogger() const noexcept
{
	return m_refLogger.get();
}

std::string OverFs::getStatVFS(struct ::statvfs& oStatFs) noexcept
{
	const std::string sFullPath{getFullPath(m_sRootPath.c_str())};

	return fspf::getStatVFS(sFullPath, oStatFs);
}

int64_t OverFs::getRealDiskSizeInBlocks() noexcept
{
	struct ::statvfs oStatFs;
	const std::string sErr = getStatVFS(oStatFs);
	if (! sErr.empty()) {
		int64_t nRet;
		{
			std::lock_guard<std::mutex> oLock(m_oFsMutex);
			nRet = m_nRealDiskSizeInBlocks;
		}
		return nRet; //---------------------------------------------------------
	}

	const int64_t nFsSizeInFragments = static_cast<int64_t>(oStatFs.f_blocks);
	//int64_t nFreeFragments = static_cast<int64_t>(oStatFs.f_bavail);

	{
		std::lock_guard<std::mutex> oLock(m_oFsMutex);
		m_nRealDiskSizeInBlocks = nFsSizeInFragments;
	}
	return nFsSizeInFragments;
}
int64_t OverFs::getRealFreeSizeInBlocks() noexcept
{
	struct ::statvfs oStatFs;
	const std::string sErr = getStatVFS(oStatFs);
	if (! sErr.empty()) {
		int64_t nRet;
		{
			std::lock_guard<std::mutex> oLock(m_oFsMutex);
			nRet = m_nRealFreeSizeInBlocks;
		}
		return nRet; //---------------------------------------------------------
	}
	//const int64_t nFsSizeInFragments = static_cast<int64_t>(oStatFs.f_blocks);
	const int64_t nFreeFragments = static_cast<int64_t>(oStatFs.f_bavail);
	{
		std::lock_guard<std::mutex> oLock(m_oFsMutex);
		m_nRealFreeSizeInBlocks = nFreeFragments;
	}
	return nFreeFragments;
}

void OverFs::setFakeDiskSizeInBlocks(int64_t nSizeBlocks) noexcept
{
	{
		std::lock_guard<std::mutex> oLock(m_oFsMutex);
		m_bUseFakeFixedDiskSize = true;
		m_nFakeDiskSizeInBlocks = nSizeBlocks;
	}
}
void OverFs::setFakeDiskSizeDiffInBlocks(int64_t nSizeBlocks) noexcept
{
	{
		std::lock_guard<std::mutex> oLock(m_oFsMutex);
		m_bUseFakeFixedDiskSize = false;
		m_nFakeDiskSizeInBlocks = nSizeBlocks;
	}
}
void OverFs::setFakeFreeSizeInBlocks(int64_t nSizeBlocks) noexcept
{
	{
		std::lock_guard<std::mutex> oLock(m_oFsMutex);
		m_bUseFakeFixedFreeSize = true;
		m_nFakeFreeSizeInBlocks = nSizeBlocks;
	}
}
void OverFs::setFakeFreeSizeDiffInBlocks(int64_t nSizeBlocks) noexcept
{
	{
		std::lock_guard<std::mutex> oLock(m_oFsMutex);
		m_bUseFakeFixedFreeSize = false;
		m_nFakeFreeSizeInBlocks = nSizeBlocks;
	}
}


void* OverFs::init(struct fuse_conn_info * p0Conn
					#if FUSE_USE_VERSION < 35
					#else
					, struct fuse_config *
					#endif
					)
{
	OverFs* p0OverFs = OverFs::this_();

	auto& oLog = *(p0OverFs->m_refLogger);
	oLog.log_msg("\nover:init()\n");

	oLog.log_conn(p0Conn);
	oLog.log_fuse_context(::fuse_get_context());

	// inform main thread that the file system has been initialized
	p0OverFs->m_oCallback();

	return p0OverFs;
}

int OverFs::getattr(const char* p0Path, struct ::stat* p0StatBuf
					#if FUSE_USE_VERSION < 35
					#else
					, struct ::fuse_file_info *
					#endif
					)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:getattr(path=\"%s\", statbuf=0x%08x)\n", p0Path, p0StatBuf);

	const std::string sFullPath{getFullPath(p0Path)};
	const int nRetStat = oLog.log_syscall("lstat", ::lstat(sFullPath.c_str(), p0StatBuf), 0);

	oLog.log_stat(p0StatBuf);

	return nRetStat;
}

int OverFs::readlink(const char* p0Path, char* p0Link, size_t nSize)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:readlink(path=\"%s\", link=\"%s\", size=%d)\n", p0Path, p0Link, nSize);

	const std::string sFullPath{getFullPath(p0Path)};
	int nRetStat = oLog.log_syscall("readlink", ::readlink(sFullPath.c_str(), p0Link, nSize - 1), 0);

	if (nRetStat >= 0) {
		p0Link[nRetStat] = '\0';
		nRetStat = 0;
		oLog.log_msg("    link=\"%s\"\n", p0Link);
	}

	return nRetStat;
}

int OverFs::mknod(const char* p0Path, mode_t nMode, dev_t nDev)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	int nRetStat;

	oLog.log_msg("\nover:mknod(path=\"%s\", mode=0%3o, dev=%lld)\n", p0Path, nMode, nDev);

	const std::string sFullPath{getFullPath(p0Path)};
	// On Linux this could just be 'mknod(path, mode, dev)' but this
	// tries to be be more portable by honoring the quote in the Linux
	// mknod man page stating the only portable use of mknod() is to
	// make a fifo, but saying it should never actually be used for
	// that.
	if (S_ISREG(nMode)) {
		nRetStat = oLog.log_syscall("open", ::open(sFullPath.c_str(), O_CREAT | O_EXCL | O_WRONLY, nMode), 0);
		if (nRetStat >= 0) {
			nRetStat = oLog.log_syscall("close", ::close(nRetStat), 0);
		}
	} else if (S_ISFIFO(nMode)) {
		nRetStat = oLog.log_syscall("mkfifo", ::mkfifo(sFullPath.c_str(), nMode), 0);
	} else {
		nRetStat = oLog.log_syscall("mknod", ::mknod(sFullPath.c_str(), nMode, nDev), 0);
	}

	return nRetStat;
}

int OverFs::mkdir(const char* p0Path, mode_t nMode)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:mkdir(path=\"%s\", mode=0%3o)\n", p0Path, nMode);

	const std::string sFullPath{getFullPath(p0Path)};

	return oLog.log_syscall("mkdir", ::mkdir(sFullPath.c_str(), nMode), 0);
}

int OverFs::unlink(const char* p0Path)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("over:unlink(path=\"%s\")\n", p0Path);

	const std::string sFullPath{getFullPath(p0Path)};

	return oLog.log_syscall("unlink", ::unlink(sFullPath.c_str()), 0);
}

int OverFs::rmdir(const char* p0Path)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("over:rmdir(path=\"%s\")\n", p0Path);

	const std::string sFullPath{getFullPath(p0Path)};

	return oLog.log_syscall("rmdir", ::rmdir(sFullPath.c_str()), 0);
}

int OverFs::symlink(const char* p0Path, const char* p0Link)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:symlink(path=\"%s\", link=\"%s\")\n", p0Path, p0Link);

	const std::string sFullLink{getFullPath(p0Link)};

	return oLog.log_syscall("symlink", ::symlink(p0Path, sFullLink.c_str()), 0);
}

int OverFs::rename(const char* p0Path, const char* p0NewPath
					#if FUSE_USE_VERSION < 35
					#else
					, unsigned int
					#endif
)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:rename(fpath=\"%s\", newpath=\"%s\")\n", p0Path, p0NewPath);

	const std::string sFullPath{getFullPath(p0Path)};
	const std::string sNewFullPath{getFullPath(p0NewPath)};

	return oLog.log_syscall("rename", ::rename(sFullPath.c_str(), sNewFullPath.c_str()), 0);
}

int OverFs::link(const char* p0Path, const char* p0NewPath)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:link(path=\"%s\", newpath=\"%s\")\n", p0Path, p0NewPath);

	const std::string sFullPath{getFullPath(p0Path)};
	const std::string sNewFullPath{getFullPath(p0NewPath)};

	return oLog.log_syscall("link", ::link(sFullPath.c_str(), sNewFullPath.c_str()), 0);
}

int OverFs::chmod(const char* p0Path, mode_t nMode
				#if FUSE_USE_VERSION < 35
				#else
				, struct fuse_file_info *
				#endif
)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:chmod(fpath=\"%s\", mode=0%03o)\n", p0Path, nMode);

	const std::string sFullPath{getFullPath(p0Path)};

	return oLog.log_syscall("chmod", ::chmod(sFullPath.c_str(), nMode), 0);
}

int OverFs::chown(const char* p0Path, uid_t nUId, gid_t nGId
				#if FUSE_USE_VERSION < 35
				#else
				, fuse_file_info *
				#endif
				)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:chown(path=\"%s\", uid=%d, gid=%d)\n", p0Path, nUId, nGId);

	const std::string sFullPath{getFullPath(p0Path)};

	return oLog.log_syscall("chown", ::chown(sFullPath.c_str(), nUId, nGId), 0);
}

int OverFs::truncate(const char* p0Path, off_t nNewSize
					#if FUSE_USE_VERSION < 35
					#else
					, fuse_file_info *
					#endif
					)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:truncate(path=\"%s\", newsize=%lld)\n", p0Path, nNewSize);

	const std::string sFullPath{getFullPath(p0Path)};

	return oLog.log_syscall("truncate", ::truncate(sFullPath.c_str(), nNewSize), 0);
}

int OverFs::utime(const char* p0Path, struct utimbuf *ubuf)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:utime(path=\"%s\", ubuf=0x%08x)\n", p0Path, ubuf);

	const std::string sFullPath{getFullPath(p0Path)};

	return oLog.log_syscall("utime", ::utime(sFullPath.c_str(), ubuf), 0);
}

int OverFs::open(const char* p0Path, struct fuse_file_info* p0FI)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:open(path\"%s\", fi=0x%08x)\n", p0Path, p0FI);

	const std::string sFullPath{getFullPath(p0Path)};

	int nRetStat = 0;

	// if the open call succeeds, my nRetStat is the file descriptor,
	// else it's -errno.  I'm making sure that in that case the saved
	// file descriptor is exactly -1.
	int fd = oLog.log_syscall("open", ::open(sFullPath.c_str(), p0FI->flags), 0);
	if (fd < 0) {
		nRetStat = oLog.log_error("open");
	}

	p0FI->fh = fd;

	oLog.log_fi(p0FI);

	return nRetStat;
}

int OverFs::read(const char* p0Path, char* p0Buf, size_t nSize, off_t nOffset, struct fuse_file_info* p0FI)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n"
				, p0Path, p0Buf, nSize, nOffset, p0FI);
	// no need to get fpath on this one, since I work from p0FI->fh not the path
	oLog.log_fi(p0FI);

	return oLog.log_syscall("pread", ::pread(p0FI->fh, p0Buf, nSize, nOffset), 0);
}

int OverFs::write(const char* p0Path, const char* p0Buf, size_t nSize, off_t nOffset
				, struct fuse_file_info* p0FI)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n"
				, p0Path, p0Buf, nSize, nOffset, p0FI);
	// no need to get fpath on this one, since I work from p0FI->fh not the path
	oLog.log_fi(p0FI);

	return oLog.log_syscall("pwrite", ::pwrite(p0FI->fh, p0Buf, nSize, nOffset), 0);
}

int OverFs::statfs(const char* p0Path, struct ::statvfs* p0StatFs)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:statfs(path=\"%s\", statv=0x%08x)\n", p0Path, p0StatFs);

	const std::string sFullPath{getFullPath(p0Path)};

	// get stats for underlying filesystem
	const int nRetStat = oLog.log_syscall("statvfs", ::statvfs(sFullPath.c_str(), p0StatFs), 0);

	if (nRetStat == -1) {
		oLog.log_retstat("statvfs", nRetStat);
		return -1; //-----------------------------------------------------------
	}
	oLog.log_statvfs(p0StatFs);

	const unsigned long nFragmentSize = p0StatFs->f_frsize;
	if (p0OverFs->m_nBlockSize != static_cast<int64_t>(nFragmentSize)) {
		oLog.log_msg("\ninternal error block size\n");
		errno = EOVERFLOW;
		return -1; //-----------------------------------------------------------
	}

	int64_t nFsSizeInFragments = static_cast<int64_t>(p0StatFs->f_blocks);
	int64_t nFreeFragments = static_cast<int64_t>(p0StatFs->f_bavail);
	int64_t nFreeishFragments = static_cast<int64_t>(p0StatFs->f_bfree);
	const int64_t nDeltaFree = nFreeishFragments - nFreeFragments;

	bool bUseFakeFixedDiskSize;
	bool bUseFakeFixedFreeSize;
	int64_t nFakeDiskSizeInBlocks;
	int64_t nFakeFreeSizeInBlocks;
	{
		std::lock_guard<std::mutex> oLock(p0OverFs->m_oFsMutex);
		// store real data
		p0OverFs->m_nRealDiskSizeInBlocks = nFsSizeInFragments;
		p0OverFs->m_nRealFreeSizeInBlocks = nFreeFragments;
		// modifying data
		bUseFakeFixedDiskSize = p0OverFs->m_bUseFakeFixedDiskSize;
		bUseFakeFixedFreeSize = p0OverFs->m_bUseFakeFixedFreeSize;
		nFakeDiskSizeInBlocks = p0OverFs->m_nFakeDiskSizeInBlocks;
		nFakeFreeSizeInBlocks = p0OverFs->m_nFakeFreeSizeInBlocks;
	}
	if (bUseFakeFixedDiskSize) {
		nFsSizeInFragments = nFakeDiskSizeInBlocks;
	} else if (nFakeDiskSizeInBlocks != 0) {
		nFsSizeInFragments += nFakeDiskSizeInBlocks;
		if (nFsSizeInFragments < 0) {
			nFsSizeInFragments = 0;
		}
	}
	if (bUseFakeFixedFreeSize) {
		nFreeFragments = std::min(nFakeFreeSizeInBlocks, nFsSizeInFragments);
	} else if (nFakeFreeSizeInBlocks != 0) {
		nFreeFragments += nFakeFreeSizeInBlocks;
		if (nFreeFragments < 0) {
			nFreeFragments= 0;
		} else if (nFreeFragments > nFsSizeInFragments) {
			nFreeFragments = nFsSizeInFragments;
		}
	}

	p0StatFs->f_blocks = static_cast<fsblkcnt_t>(nFsSizeInFragments);
	p0StatFs->f_bavail = static_cast<fsblkcnt_t>(nFreeFragments);
	p0StatFs->f_bfree = static_cast<fsblkcnt_t>(nFreeFragments + nDeltaFree);
	oLog.log_msg("\n     new f_blocks %lld \n", p0StatFs->f_blocks);
	oLog.log_msg("     new f_bavail %lld \n", p0StatFs->f_bavail);
	oLog.log_msg("     new f_bfree %lld \n", p0StatFs->f_bfree);
	oLog.log_msg("     nRetStat %d \n", nRetStat);

	return nRetStat;
}

int OverFs::flush(const char* p0Path, struct fuse_file_info* p0FI)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:flush(path=\"%s\", fi=0x%08x)\n", p0Path, p0FI);
	// no need to get fpath on this one, since I work from p0FI->fh not the path
	oLog.log_fi(p0FI);

	return 0;
}

int OverFs::release(const char* p0Path, struct fuse_file_info* p0FI)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:release(path=\"%s\", fi=0x%08x)\n", p0Path, p0FI);
	oLog.log_fi(p0FI);

	// We need to close the file.  Had we allocated any resources
	// (buffers etc) we'd need to free them here as well.
	return oLog.log_syscall("close", ::close(p0FI->fh), 0);
}

int OverFs::fsync(const char* p0Path, int nDataSync, struct fuse_file_info* p0FI)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:fsync(path=\"%s\", datasync=%d, fi=0x%08x)\n", p0Path, nDataSync, p0FI);
	oLog.log_fi(p0FI);

	// some unix-like systems (notably freebsd) don't have a datasync call
	//#ifdef HAVE_FDATASYNC
	if (nDataSync) {
		return oLog.log_syscall("fdatasync", ::fdatasync(p0FI->fh), 0);
	}
	//#endif	
	return oLog.log_syscall("fsync", ::fsync(p0FI->fh), 0);
}

int OverFs::setxattr(const char* p0Path, const char* p0Name, const char* p0Value, size_t nSize, int nFlags)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:setxattr(path=\"%s\", name=\"%s\", value=\"%s\", size=%d, flags=0x%08x)\n"
				, p0Path, p0Name, p0Value, nSize, nFlags);

	const std::string sFullPath{getFullPath(p0Path)};

	return oLog.log_syscall("lsetxattr", ::lsetxattr(sFullPath.c_str(), p0Name, p0Value, nSize, nFlags), 0);
}

int OverFs::getxattr(const char* p0Path, const char* p0Name, char* p0Value, size_t nSize)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:getxattr(path = \"%s\", name = \"%s\", value = 0x%08x, size = %d)\n"
				, p0Path, p0Name, p0Value, nSize);

	const std::string sFullPath{getFullPath(p0Path)};

	const int nRetStat = oLog.log_syscall("lgetxattr", ::lgetxattr(sFullPath.c_str(), p0Name, p0Value, nSize), 0);
	if (nRetStat >= 0) {
		oLog.log_msg("    value = \"%s\"\n", p0Value);
	}

	return nRetStat;
}

int OverFs::listxattr(const char* p0Path, char* p0List, size_t nSize)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:listxattr(path=\"%s\", list=0x%08x, size=%d)\n"
				, p0Path, p0List, nSize);

	const std::string sFullPath{getFullPath(p0Path)};

	const int nRetStat = oLog.log_syscall("llistxattr", ::llistxattr(sFullPath.c_str(), p0List, nSize), 0);
	if (nRetStat >= 0) {
		oLog.log_msg("    returned attributes (length %d):\n", nRetStat);
		if (p0List != nullptr) {
			for (char* p0El = p0List; p0El < p0List + nRetStat; p0El += ::strlen(p0El)+1) {
				oLog.log_msg("    \"%s\"\n", p0El);
			}
		} else {
			oLog.log_msg("    (null)\n");
		}
	}

	return nRetStat;
}

int OverFs::removexattr(const char* p0Path, const char* p0Name)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:removexattr(path=\"%s\", name=\"%s\")\n", p0Path, p0Name);

	const std::string sFullPath{getFullPath(p0Path)};

	return oLog.log_syscall("lremovexattr", ::lremovexattr(sFullPath.c_str(), p0Name), 0);
}

int OverFs::opendir(const char* p0Path, struct fuse_file_info* p0FI)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:opendir(path=\"%s\", fi=0x%08x)\n", p0Path, p0FI);

	const std::string sFullPath{getFullPath(p0Path)};

	int nRetStat = 0;

	DIR* p0DirStream = ::opendir(sFullPath.c_str());
	oLog.log_msg("    opendir returned 0x%p\n", p0DirStream);
	if (p0DirStream == nullptr) {
		nRetStat = oLog.log_error("over:opendir opendir");
	}

	p0FI->fh = reinterpret_cast<uint64_t>(p0DirStream);

	oLog.log_fi(p0FI);

	return nRetStat;
}

int OverFs::readdir(const char* p0Path, void* p0Buf, fuse_fill_dir_t filler, off_t nOffset
					, struct fuse_file_info* p0FI
					#if FUSE_USE_VERSION < 35
					#else
					, enum fuse_readdir_flags
					#endif
					)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:readdir(path=\"%s\", buf=0x%08x, filler=0x%08x, offset=%lld, fi=0x%08x)\n"
				, p0Path, p0Buf, filler, nOffset, p0FI);

	int nRetStat = 0;
	// once again, no need for fullpath -- but note that I need to cast p0FI->fh
	DIR* p0DirStream = reinterpret_cast<DIR *>(static_cast<uintptr_t>(p0FI->fh));

	// Every directory contains at least two entries: . and ..  If my
	// first call to the system readdir() returns nullptr I've got an
	// error; near as I can tell, that's the only condition under
	// which I can get an error from readdir()
	struct dirent* p0DirEntry = ::readdir(p0DirStream);
	oLog.log_msg("    readdir returned 0x%p\n", p0DirEntry);
	if (p0DirEntry == nullptr) {
		nRetStat = oLog.log_error("over:readdir readdir");
		return nRetStat;
	}

	// This will copy the entire directory into the buffer.  The loop exits
	// when either the system readdir() returns nullptr, or filler()
	// returns something non-zero.  The first case just means I've
	// read the whole directory; the second means the buffer is full.
	do {
		oLog.log_msg("calling filler with name %s\n", p0DirEntry->d_name);
		if (filler(p0Buf, p0DirEntry->d_name, nullptr, 0) != 0) {
			oLog.log_msg("    ERROR over:readdir filler:  buffer full");
			return -ENOMEM;
		}
	} while ((p0DirEntry = ::readdir(p0DirStream)) != nullptr);

	oLog.log_fi(p0FI);

	return nRetStat;
}

int OverFs::releasedir(const char* p0Path, struct fuse_file_info* p0FI)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:releasedir(path=\"%s\", fi=0x%08x)\n", p0Path, p0FI);

	oLog.log_fi(p0FI);

	DIR* p0DirStream = reinterpret_cast<DIR *>(static_cast<uintptr_t>(p0FI->fh));

	::closedir(p0DirStream);

	int nRetStat = 0;
	return nRetStat;
}

int OverFs::fsyncdir(const char* p0Path, int nDataSync, struct fuse_file_info* p0FI)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:fsyncdir(path=\"%s\", datasync=%d, fi=0x%08x)\n", p0Path, nDataSync, p0FI);
	oLog.log_fi(p0FI);

	int nRetStat = 0;
	return nRetStat;
}

void OverFs::destroy(void* p0Userdata)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:destroy(userdata=0x%08x)\n", p0Userdata);
}

int OverFs::access(const char* p0Path, int nMask)
{
	OverFs* p0OverFs = OverFs::this_();
	auto& oLog = *(p0OverFs->m_refLogger);

	oLog.log_msg("\nover:access(path=\"%s\", mask=0%o)\n", p0Path, nMask);

	const std::string sFullPath{getFullPath(p0Path)};

	int nRetStat = ::access(sFullPath.c_str(), nMask);

	if (nRetStat < 0) {
		nRetStat = oLog.log_error("over:access access");
	}

	return nRetStat;
}

} // namespace fspf

