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
 * File:   fslogger.cc
 */
/* This code is mostly copied (and modified) from the:
 *
 *  Big Brother File System
 *  Copyright (C) 2012 Joseph J. Pfeiffer, Jr., Ph.D. <pfeiffer@cs.nmsu.edu>
 *
  Which is distributed under the terms of the GNU GPLv3.
 */

// To undo the define:
// #define log_struct(st, field, format, typecast) 
//   log_msg("    " #field " = " #format "\n", typecast st->field)
//
// In kwrite:
// Search : log_struct\(([a-z0-9_]+), ([a-z0-9_]+), ([a-z0-9%]+),[ ]+\)
// Replace: log_msg("     \2 = \3\\n", \1->\2)

#include "fslogger.h"

#include "fspropfaker.h"

#include "fsutil.h"

#include <iostream>
#include <cassert>
#include <memory>
#include <string>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/statvfs.h>


using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;

namespace fspf
{

std::pair<unique_ptr<FsLogger>, std::string> FsLogger::create(const std::string& sMountName, const std::string& sLogFilePath) noexcept
{
	auto refLogger = std::unique_ptr<FsLogger>(new FsLogger(sMountName, sLogFilePath));
	std::string sErr = refLogger->init();
	if (! sErr.empty()) {
		return std::make_pair(unique_ptr<FsLogger>{}, std::move(sErr));
	}
	return std::make_pair(std::move(refLogger), "");
}
FsLogger::FsLogger(const std::string& sMountName, const std::string& sLogFilePath) noexcept
: m_sMountName(sMountName)
, m_sLogFilePath(sLogFilePath)
{
}
std::string FsLogger::init() noexcept
{
	if (m_sLogFilePath.empty()) {
		// no logging
		return "";
	}
	std::string sLogFilePath = m_sLogFilePath;
	if (dirExists(m_sLogFilePath)) {
		// it's a directory
		const std::string sLogFileName = m_sMountName + ".log";
		sLogFilePath += "/" + sLogFileName;
	}
	m_p0LogFile = ::fopen(sLogFilePath.c_str(), "w");
	if (m_p0LogFile == nullptr) {
		return std::string{"Could not open log file "} + sLogFilePath;
	}
	// set logfile to line buffering
	::setvbuf(m_p0LogFile, NULL, _IOLBF, 0);
	return "";
}
FsLogger::~FsLogger() noexcept
{
	if (m_p0LogFile != nullptr) {
		::fclose(m_p0LogFile);
	}
}

	//void FsLogger::log_function(const char* p0FuncName, const char* p0Format, ...)
	//{
	//	::vfprintf(m_p0LogFile, p0Format, oAP);
	//}

void FsLogger::log_msg(const char* p0Format, ...)
{
	if (m_p0LogFile == nullptr) {
		return;
	}
	::va_list oAP;
	::va_start(oAP, p0Format);

	::vfprintf(m_p0LogFile, p0Format, oAP);
}
void FsLogger::log_conn(struct fuse_conn_info* p0Conn)
{
	if (m_p0LogFile == nullptr) {
		return;
	}

	log_msg("    conn:\n");

	/** Major version of the protocol (read-only) */
	// unsigned proto_major;
	log_msg("     proto_major = %d\n", p0Conn->proto_major);

	/** Minor version of the protocol (read-only) */
	// unsigned proto_minor;
	log_msg("     proto_minor = %d\n", p0Conn->proto_minor);

	/** Is asynchronous read supported (read-write) */
	// unsigned async_read;
	log_msg("     async_read = %d\n", p0Conn->async_read);

	/** Maximum size of the write buffer */
	// unsigned max_write;
	log_msg("     max_write = %d\n", p0Conn->max_write);

	/** Maximum readahead */
	// unsigned max_readahead;
	log_msg("     max_readahead = %d\n", p0Conn->max_readahead);

	/** Capability flags, that the kernel supports */
	// unsigned capable;
	log_msg("     capable = %08x\n", p0Conn->capable);

	/** Capability flags, that the filesystem wants to enable */
	// unsigned want;
	log_msg("     want = %08x\n", p0Conn->want);

	/** Maximum number of backgrounded requests */
	// unsigned max_background;
	log_msg("     max_background = %d\n", p0Conn->max_background);

	/** Kernel congestion threshold parameter */
	// unsigned congestion_threshold;
	log_msg("     congestion_threshold = %d\n", p0Conn->congestion_threshold);

	/** For future use. */
	// unsigned reserved[23];
}
int FsLogger::log_error(const char* p0Func)
{
	int nRet = -errno;

	log_msg("    ERROR %s: %s\n", p0Func, ::strerror(errno));

	return nRet;
}
void FsLogger::log_fi(struct fuse_file_info* p0FI)
{
	log_msg("    fi:\n");

	/** Open flags.  Available in open() and release() */
	//	int flags;
	log_msg("     flags = 0x%08x\n", p0FI->flags);

	/** Old file handle, don't use */
	//	unsigned long fh_old;	
	log_msg("     fh_old = 0x%08lx\n", p0FI->fh_old);

	/** In case of a write operation indicates if this was caused by a
	    writepage */
	//	int writepage;
	log_msg("     writepage = %d\n", p0FI->writepage);

	/** Can be filled in by open, to use direct I/O on this file.
	    Introduced in version 2.4 */
	//	unsigned int keep_cache : 1;
	log_msg("     direct_io = %d\n", p0FI->direct_io);

	/** Can be filled in by open, to indicate, that cached file data
	    need not be invalidated.  Introduced in version 2.4 */
	//	unsigned int flush : 1;
	log_msg("     keep_cache = %d\n", p0FI->keep_cache);

	/** Padding.  Do not use*/
	//	unsigned int padding : 29;

	/** File handle.  May be filled in by filesystem in open().
	    Available in all other file operations */
	//	uint64_t fh;
	log_msg("     fh = 0x%016llx\n", p0FI->fh);

	/** Lock owner id.  Available in locking operations and flush */
	//  uint64_t lock_owner;
	log_msg("     lock_owner = 0x%016llx\n", p0FI->lock_owner);
}
void FsLogger::log_fuse_context(struct fuse_context* p0Context)
{
	log_msg("    context:\n");

	/** Pointer to the fuse object */
	//	struct fuse *fuse;
	log_msg("     fuse = %08x\n", p0Context->fuse);

	/** User ID of the calling process */
	//	uid_t uid;
	log_msg("     uid = %d\n", p0Context->uid);

	/** Group ID of the calling process */
	//	gid_t gid;
	log_msg("     gid = %d\n", p0Context->gid);

	/** Thread ID of the calling process */
	//	pid_t pid;
	log_msg("     pid = %d\n", p0Context->pid);

	// /** Private filesystem data */
	// //	void *private_data;
	// log_msg("     private_data = %08x\n", p0Context->private_data);
	// log_struct(((struct bb_state *)p0Context->private_data), logfile, %08x, );
	// log_struct(((struct bb_state *)p0Context->private_data), rootdir, %s, );

	/** Umask of the calling process (introduced in version 2.8) */
	//	mode_t umask;
	log_msg("     umask = %05o\n", p0Context->umask);

}
void FsLogger::log_retstat(const char* p0func, int nRetStat)
{
	int nErrSave = errno;
	log_msg("    %s returned %d\n", p0func, nRetStat);
	errno = nErrSave;
}
void FsLogger::log_stat(struct stat* p0StatBuf)
{
	log_msg("    si:\n");

	//  dev_t     st_dev;     /* ID of device containing file */
	log_msg("     st_dev = %lld\n", p0StatBuf->st_dev);

	//  ino_t     st_ino;     /* inode number */
	log_msg("     st_ino = %lld\n", p0StatBuf->st_ino);

	//  mode_t    st_mode;    /* protection */
	log_msg("     st_mode = 0%o\n", p0StatBuf->st_mode);

	//  nlink_t   st_nlink;   /* number of hard links */
	log_msg("     st_nlink = %d\n", p0StatBuf->st_nlink);

	//  uid_t     st_uid;     /* user ID of owner */
	log_msg("     st_uid = %d\n", p0StatBuf->st_uid);

	//  gid_t     st_gid;     /* group ID of owner */
	log_msg("     st_gid = %d\n", p0StatBuf->st_gid);

	//  dev_t     st_rdev;    /* device ID (if special file) */
	log_msg("     st_rdev = %lld\n", p0StatBuf->st_rdev);

	//  off_t     st_size;    /* total size, in bytes */
	log_msg("     st_size = %lld\n", p0StatBuf->st_size);

	//  blksize_t st_blksize; /* blocksize for filesystem I/O */
	log_msg("     st_blksize = %ld\n", p0StatBuf->st_blksize);

	//  blkcnt_t  st_blocks;  /* number of blocks allocated */
	log_msg("     st_blocks = %lld\n", p0StatBuf->st_blocks);

	//  time_t    st_atime;   /* time of last access */
	log_msg("     st_atime = 0x%08lx\n", p0StatBuf->st_atime);

	//  time_t    st_mtime;   /* time of last modification */
	log_msg("     st_mtime = 0x%08lx\n", p0StatBuf->st_mtime);

	//  time_t    st_ctime;   /* time of last status change */
	log_msg("     st_ctime = 0x%08lx\n", p0StatBuf->st_ctime);

}
void FsLogger::log_statvfs(struct ::statvfs* p0StatFs)
{
	log_msg("    sv:\n");

	//  unsigned long  f_bsize;    /* file system block size */
	log_msg("     f_bsize = %ld\n", p0StatFs->f_bsize);

	//  unsigned long  f_frsize;   /* fragment size */
	log_msg("     f_frsize = %ld\n", p0StatFs->f_frsize);

	//  fsblkcnt_t     f_blocks;   /* size of fs in f_frsize units */
	log_msg("     f_blocks = %lld\n", p0StatFs->f_blocks);

	//  fsblkcnt_t     f_bfree;    /* # free blocks */
	log_msg("     f_bfree = %lld\n", p0StatFs->f_bfree);

	//  fsblkcnt_t     f_bavail;   /* # free blocks for non-root */
	log_msg("     f_bavail = %lld\n", p0StatFs->f_bavail);

	//  fsfilcnt_t     f_files;    /* # inodes */
	log_msg("     f_files = %lld\n", p0StatFs->f_files);

	//  fsfilcnt_t     f_ffree;    /* # free inodes */
	log_msg("     f_ffree = %lld\n", p0StatFs->f_ffree);

	//  fsfilcnt_t     f_favail;   /* # free inodes for non-root */
	log_msg("     f_favail = %lld\n", p0StatFs->f_favail);

	//  unsigned long  f_fsid;     /* file system ID */
	log_msg("     f_fsid = %ld\n", p0StatFs->f_fsid);

	//  unsigned long  f_flag;     /* mount flags */
	log_msg("     f_flag = 0x%08lx\n", p0StatFs->f_flag);

	//  unsigned long  f_namemax;  /* maximum filename length */
	log_msg("     f_namemax = %ld\n", p0StatFs->f_namemax);
}
int FsLogger::log_syscall(const char* p0func, int nRetStat, int nMinRet)
{
	log_retstat(p0func, nRetStat);

	if (nRetStat < nMinRet) {
		log_error(p0func);
		nRetStat = -errno;
	}

	return nRetStat;

}
void FsLogger::log_utime(struct utimbuf* p0Buf)
{
	log_msg("    buf:\n");

	//    time_t actime;
	log_msg("     actime = 0x%08lx\n", p0Buf->actime);

	//    time_t modtime;
	log_msg("     modtime = 0x%08lx\n", p0Buf->modtime);
}

} // namespace fspf
