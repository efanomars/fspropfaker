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
 * File:   overfs.h
 */

#ifndef OVER_FS_H
#define OVER_FS_H

#include "fusepp/Fuse.h"
#include "fusepp/Fuse.cpp"

#include "fslogger.h"

#include <memory>
#include <string>
#include <functional>
#include <mutex>

namespace fspf
{

using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;

class FsPropFaker;

/** File system property faker.
 */
class OverFs : public Fusepp::Fuse<OverFs>
{
public:
	// returns object and empty string or null and error string
	static std::pair<shared_ptr<OverFs>, std::string> createInstance(FsPropFaker* p0FsPropFaker
																	, std::function<void()>&& oCallback) noexcept;

	// callback from fuse when initializing (initialized?)
	#if FUSE_USE_VERSION < 35
	static void* init(struct fuse_conn_info* p0Conn);
	#else
	static void* init(struct fuse_conn_info* p0Conn, struct fuse_config*);
	#endif
	#if FUSE_USE_VERSION < 35
	static int getattr (const char* p0Path, struct ::stat* p0StatBuf);
	#else
	static int getattr (const char* p0Path, struct ::stat* p0StatBuf, struct fuse_file_info*);
	#endif
	static int readlink(const char* p0Path, char* p0Link, size_t nSize);
	static int mknod(const char* p0Path, mode_t nMode, dev_t nDev);
	static int mkdir(const char* p0Path, mode_t nMode);
	static int unlink(const char* p0Path);
	static int rmdir(const char* p0Path);
	static int symlink(const char* p0Path, const char* p0Link);
	#if FUSE_USE_VERSION < 35
	static int rename(const char* p0Path, const char* p0NewPath);
	#else
	static int rename(const char* p0Path, const char* p0NewPath, unsigned int);
	#endif
	static int link(const char* p0Path, const char* p0NewPath);
	#if FUSE_USE_VERSION < 35
	static int chmod(const char* p0Path, mode_t nMode);
	#else
	static int chmod(const char* p0Path, mode_t nMode, struct fuse_file_info*);
	#endif
	#if FUSE_USE_VERSION < 35
	static int chown(const char* p0Path, uid_t nUId, gid_t nGId);
	#else
	static int chown(const char* p0Path, uid_t nUId, gid_t nGId, fuse_file_info*);
	#endif
	#if FUSE_USE_VERSION < 35
	static int truncate(const char* p0Path, off_t nNewSize);
	#else
	static int truncate(const char* p0Path, off_t nNewSize, fuse_file_info *);
	#endif
	static int utime(const char* p0Path, struct utimbuf *ubuf);
	static int open(const char* p0Path, struct fuse_file_info* p0FI);
	static int read(const char* p0Path, char* p0Buf, size_t nSize, off_t nOffset, struct fuse_file_info* p0FI);
	static int write(const char* p0Path, const char* p0Buf, size_t nSize, off_t nOffset
					, struct fuse_file_info* p0FI);
	static int statfs(const char* p0Path, struct ::statvfs* p0StatFs);
	static int flush(const char* p0Path, struct fuse_file_info* p0FI);
	static int release(const char* p0Path, struct fuse_file_info* p0FI);
	static int fsync(const char* p0Path, int nDataSync, struct fuse_file_info* p0FI);
	static int setxattr(const char* p0Path, const char* p0Name, const char* p0Value, size_t nSize, int nFlags);
	static int getxattr(const char* p0Path, const char* p0Name, char* p0Value, size_t nSize);
	static int listxattr(const char* p0Path, char* p0List, size_t nSize);
	static int removexattr(const char* p0Path, const char* p0Name);
	static int opendir(const char* p0Path, struct fuse_file_info* p0FI);
	#if FUSE_USE_VERSION < 35
	static int readdir(const char* p0Path, void* p0Buf, fuse_fill_dir_t filler, off_t nOffset
						, struct fuse_file_info* p0FI);
	#else
	static int readdir(const char* p0Path, void* p0Buf, fuse_fill_dir_t filler, off_t nOffset
						, struct fuse_file_info* p0FI, enum fuse_readdir_flags);
	#endif
	static int releasedir(const char* p0Path, struct fuse_file_info* p0FI);
	static int fsyncdir(const char* p0Path, int nDataSync, struct fuse_file_info* p0FI);
	static void destroy(void* p0UserData);
	static int access(const char* p0Path, int nMask);


	FsPropFaker* getFsPropFaker() const noexcept;
	FsLogger* getFsLogger() const noexcept;

	int64_t getRealDiskSizeInBlocks() noexcept;
	int64_t getRealFreeSizeInBlocks() noexcept;

	void setFakeDiskSizeInBlocks(int64_t nSizeBlocks) noexcept;
	void setFakeDiskSizeDiffInBlocks(int64_t nSizeBlocks) noexcept;
	void setFakeFreeSizeInBlocks(int64_t nSizeBlocks) noexcept;
	void setFakeFreeSizeDiffInBlocks(int64_t nSizeBlocks) noexcept;

protected:
	OverFs(FsPropFaker* p0FsPropFaker, std::function<void()>&& oCallback) noexcept;
	std::string initInstance() noexcept;
private:
	std::string getStatVFS(struct ::statvfs& oStatFs) noexcept;
private:
	//friend class FsPropFaker;
	FsPropFaker* m_p0FsPropFaker;
	const std::string& m_sMountName;
	const std::string& m_sRootPath;
	const std::string& m_sLogFilePath;
	int64_t m_nBlockSize;

	unique_ptr<FsLogger> m_refLogger;

	std::function<void()> m_oCallback;

	std::mutex m_oFsMutex;
		int64_t m_nRealDiskSizeInBlocks = -1; // if negative not determined yet.
		int64_t m_nRealFreeSizeInBlocks = -1; // if negative not determined yet.
		bool m_bUseFakeFixedDiskSize = false; // if false m_nFakeDiskSizeInBlocks must be added to real disk size.
		bool m_bUseFakeFixedFreeSize = false; // if false m_nFakeFreeSizeInBlocks must be added to real free size.
		int64_t m_nFakeDiskSizeInBlocks = 0; // 1000000 bytes.
		int64_t m_nFakeFreeSizeInBlocks = 0; // 1000000 bytes.

private:
	OverFs() = delete;
	OverFs(const OverFs& oSource) = delete;
	OverFs& operator=(const OverFs& oSource) = delete;
};

} // namespace fspf

#endif /* OVER_FS_H */

